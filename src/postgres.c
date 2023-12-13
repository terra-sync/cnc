#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "libpq-fe.h"

#include "config.h"
#include "db/db.h"
#include "db/postgres.h"
#include "log.h"

/* Write/Read end for pipes */
#define READ_END 0
#define WRITE_END 1

extern config_t *yaml_config;

extern struct db_operations **available_dbs;

static struct db_operations pg_db_ops = {
	.connect = connect_pg,
	.close = close_pg,
	.replicate = replicate,
};

int construct_pg(void)
{
	struct db_t *pg_db_t = (struct db_t *)malloc(sizeof(struct db_t));

	pg_db_t->pg_conf = (postgres_t *)malloc(sizeof(postgres_t));
	memcpy(pg_db_t->pg_conf, yaml_config->postgres_config,
	       sizeof(postgres_t));

	pg_db_ops.db = pg_db_t;
	available_dbs[0] = &pg_db_ops;

	return 0;
}

/*
 * connect_pg
 *
 * Returns: 
 *   0 Success
 *  -1 Failure to connect
 */
int connect_pg(struct db_t *pg_db_t)
{
	int ret = 0;
	/*
	 * We should NULL the connection to avoid using `free` on an
	 * uninitialized connection.
	 */
	pg_db_t->origin_conn = NULL;
	pg_db_t->target_conn = NULL;

	// Initialize the fields that are to be used to connect to postgres.
	const char *keywords[] = { "host", "user",   "password",
				   "port", "dbname", NULL };
	// Construct the array that will hold the values of the fields.
	const char *origin_values[] = { pg_db_t->pg_conf->origin_host,
					pg_db_t->pg_conf->origin_user,
					pg_db_t->pg_conf->origin_password,
					pg_db_t->pg_conf->origin_port,
					pg_db_t->pg_conf->origin_database };
	const char *target_values[] = { pg_db_t->pg_conf->target_host,
					pg_db_t->pg_conf->target_user,
					pg_db_t->pg_conf->target_password,
					pg_db_t->pg_conf->target_port,
					pg_db_t->pg_conf->target_database };

	// Connect to origin-database.
	pg_db_t->origin_conn = PQconnectdbParams(keywords, origin_values, 0);
	if (PQstatus(pg_db_t->origin_conn) != CONNECTION_OK) {
		pr_debug("Postgres origin-database connection: Failed!\n");
		fprintf(stderr, "%s", PQerrorMessage(pg_db_t->origin_conn));
		ret = -1;
		return ret;
	}
	pr_debug("Origin-database connection: Success!\n");

	// Connect to target-database.
	pg_db_t->target_conn = PQconnectdbParams(keywords, target_values, 0);
	if (PQstatus(pg_db_t->target_conn) != CONNECTION_OK) {
		pr_debug("Target-database connection: Failed!\n");
		fprintf(stderr, "%s", PQerrorMessage(pg_db_t->target_conn));
		ret = -1;
		return ret;
	}
	pr_debug("Target-database connection: Success!\n");

	return ret;
}

void close_pg(struct db_t *pg_db_t)
{
	PQfinish(pg_db_t->origin_conn);
	PQfinish(pg_db_t->target_conn);
	free(pg_db_t->pg_conf);
}

/*
 * replicate
 *  Construct the variables and arguments that will be used for the
 * `pg_dump` and `pg_fork`, then call the `execve_binary` to execute
 *  the commands.
 *
 * Returns:
 *   0 Success
 *  -1 Failure of`pg_dump` or `pg_restore`
 *  -2 Failure of creation of fork() or pipe()
 *
 */
int replicate(struct db_t *pg_db_t, struct options *pg_options)
{
	int ret = 0;

	/*
	 * Setting up the password and command sizes as variables to
	 * avoid multiple uses of strlen().
	 */
	int pg_pass_size =
		PG_PASS_PREFIX + strlen(pg_db_t->pg_conf->origin_password) + 1;
	int pg_command_size = strlen(PG_DUMP_COMMAND);

	/*
	 * These will be used as an environmental variable for `pg_dump` and `pg_restore`.
	 * The format of the passed variables is `PGPASSWORD=<password>`.
	 */
	char *pg_pass = calloc(pg_pass_size, sizeof(char));

	/*
	 * The max length of the array is 261 because 255 is the max length of
	 * the acceptable filepath by Linux. Our string is of the format
	 * `PATH=/path/to/pg_command` therefore we also need 5 characters for
	 * `PATH=` + 1 for '\0'.
	 */
	char *pg_bin, prefixed_command_path[261] = "PATH=";

	/*
	 * Set this as the default command for Debian-based distributions. The array
	 * should potentially hold the max length of the filepath because it might be
	 * overwritten if the user has specified the $PG_BIN environmental variable.
	 */
	char command_path[255] = "/usr/bin/";
	int command_path_size = strlen(command_path);
	char *pg_command_path;

	/*
	 * This is `$HOME/backup.dump and will be used as the path for the output
	 * of `pg_dump` and the input of the `pg_restore`.
	 */
	char backup_path[255] = { 0 };

	char *const dump_args[] = {
		PG_DUMP_COMMAND,
		"-h",
		(char *const)pg_db_t->pg_conf->origin_host,
		"-F",
		"custom",
		"-p",
		(char *const)pg_db_t->pg_conf->origin_port,
		"-U",
		(char *const)pg_db_t->pg_conf->origin_user,
		"-d",
		(char *const)pg_db_t->pg_conf->origin_database,
		"-f",
		backup_path,
		"-v",
		NULL
	};
	char *const restore_args[] = {
		PG_RESTORE_COMMAND,
		"-h",
		(char *const)pg_db_t->pg_conf->target_host,
		"-p",
		(char *const)pg_db_t->pg_conf->target_port,
		"-U",
		(char *const)pg_db_t->pg_conf->target_user,
		"-d",
		(char *const)pg_db_t->pg_conf->target_database,
		backup_path,
		"-v",
		NULL
	};

	char const *command_envp[3];

	construct_dump_path(backup_path);

	strcpy(pg_pass, "PGPASSWORD=");
	strcat(pg_pass, (char *)pg_db_t->pg_conf->origin_password);

	pr_debug("Checking if $PG_BIN environmental variable.\n");
	pg_bin = getenv("PG_BIN");
	if (pg_bin) {
		pr_debug("$PG_BIN=%s was found.\n", pg_bin);
		command_path_size = strlen(pg_bin);
		strncat(prefixed_command_path, pg_bin, command_path_size);
		stpncpy(command_path, pg_bin, strlen(prefixed_command_path));
	} else {
		pr_debug(
			"$PG_BIN was not found, default path %s will be used.\n",
			command_path);
		strncat(prefixed_command_path, command_path,
			command_path_size + 1);
	}

	pg_command_path = calloc(command_path_size + pg_command_size + 1,
				 (command_path_size + pg_command_size + 1) *
					 sizeof(char));
	strncpy(pg_command_path, command_path, command_path_size);
	strncat(pg_command_path, PG_DUMP_COMMAND, pg_command_size + 1);

	command_envp[0] = pg_pass;
	command_envp[1] = prefixed_command_path;
	command_envp[2] = NULL;

	/*
	 * The replication process is achieved by using a fork() for `pg_dump`
	 * and another fork() for `pg_restore`.
	 */
	pr_debug("Starting `pg_dump` process.\n");
	ret = execve_binary(pg_command_path, dump_args,
			    (char *const *)command_envp);

	if (ret != 0) {
		printf("`%s` failed\n", PG_DUMP_COMMAND);
		free(pg_command_path);
		free(pg_pass);
		return ret;
	}

	/* Reassign the variables with the values for the `pg_restore` command.
	 * This is the same steps used previously for `pg_dump`.
	 */
	pg_pass_size =
		PG_PASS_PREFIX + strlen(pg_db_t->pg_conf->target_password) + 1;
	pg_command_size = strlen(PG_RESTORE_COMMAND);
	pg_pass = realloc(pg_pass, pg_pass_size * sizeof(char));

	strcpy(pg_pass, "PGPASSWORD=");
	strcat(pg_pass, (char *)pg_db_t->pg_conf->target_password);

	pg_command_path = realloc(pg_command_path,
				  (command_path_size + pg_command_size + 1) *
					  sizeof(char));

	strncpy(pg_command_path, command_path,
		command_path_size + pg_command_size + 1);
	strncat(pg_command_path, PG_RESTORE_COMMAND, pg_command_size + 1);

	command_envp[0] = pg_pass;
	command_envp[1] = prefixed_command_path;
	command_envp[2] = NULL;

	pr_debug("Starting `pg_restore` process.\n");
	ret = execve_binary(pg_command_path, restore_args,
			    (char *const *)command_envp);

	if (ret != 0) {
		printf("`%s` failed\n", PG_RESTORE_COMMAND);
		free(pg_command_path);
		free(pg_pass);
		return ret;
	}

	free(pg_command_path);
	free(pg_pass);
	printf("Database Replication was successful!\n");

	return ret;
}

void construct_dump_path(char *path)
{
	char *home_path = getenv("HOME");
	int home_path_size = strlen(home_path);
	strncpy(path, home_path, home_path_size);
	strcat(path, "/backup.dump");
}

void read_buffer_pipe(int *pipefd)
{
	char buffer[4096] = { 0 };
	int nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));
	while (nbytes > 0) {
		printf("%s", buffer);
		memset(buffer, 0, sizeof(buffer));
		nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));
	}
}

/*
 * execve_binary
 *  This function utilizes a fork for calling `execve` and reads the
 *  output through the usage of `read_buffer_pipe`.
 *
 * Returns:
 *   0 Success
 *  -1 Failure of`pg_dump` or `pg_restore`
 *  -2 Failure of creation of fork() or pipe()
 *
 */
int execve_binary(char *command_path, char *const command_args[],
		  char *const envp[])
{
	// A pipe to read the output from the process.
	int pipefd[2];
	int pg_pid;
	int wstatus;

	if (pipe(pipefd) == -1) {
		printf("Failure creating the pipe.\n");
		return -2;
	}

	pg_pid = fork();
	if (pg_pid == 0) {
		close(pipefd[READ_END]);
		dup2(pipefd[WRITE_END], STDERR_FILENO);
		dup2(pipefd[WRITE_END], STDOUT_FILENO);
		close(pipefd[WRITE_END]);
		execve(command_path, command_args, envp);
		printf("Error executing `pg_dump`\n");
		pr_debug("`pg_dump` error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	} else {
		if (pg_pid == -1) {
			pr_debug(
				"`pg_dump` process failed to start: %s.\n Replication process will be terminated.\n",
				strerror(errno));
			printf("%s\n", strerror(errno));
			return -2;
		}
	}
	close(pipefd[WRITE_END]);
	read_buffer_pipe(pipefd);

	wait(&wstatus);
	// Check the status of the forked process, anything other than 0 means failure.
	if (wstatus != 0) {
		pr_debug(
			"`pg_dump` was unsuccessful. Replication process will be terminated.\n");
		return -1;
	}

	return 0;
}

ADD_FUNC(construct_pg);
