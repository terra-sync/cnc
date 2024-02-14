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
#include "rust/email-bindings.h"
#include "util.h"

char *email_body;

extern config_t *ini_config;

extern struct db_operations **available_dbs;

static struct db_operations pg_db_ops = {
	.connect = connect_pg,
	.close = close_pg,
	.replicate = replicate,
};

int construct_pg(void)
{
	struct db_t *pg_db_t = (struct db_t *)malloc(sizeof(struct db_t));
	email_body = malloc(EMAIL_BODY_LENGTH * sizeof(char));
	pg_db_t->pg_conf = (postgres_t *)malloc(sizeof(postgres_t));

	memcpy(pg_db_t->pg_conf, ini_config->postgres_config,
	       sizeof(postgres_t));
	snprintf(email_body, EMAIL_BODY_LENGTH,
		 "Summary of Postgres Database: `%s`\r\n\r\n",
		 pg_db_t->pg_conf->origin_database);

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
 *  -2 Not enabled
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

	// Check if the database is `enabled` before attempting to connect
	if (!pg_db_t->pg_conf->enabled) {
		ret = -2;
		free(email_body);
		return ret;
	}

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
		pr_error("%s", PQerrorMessage(pg_db_t->origin_conn));
		strncat(email_body, PQerrorMessage(pg_db_t->origin_conn),
			EMAIL_BODY_LENGTH - 1);
		ret = -1;
		return ret;
	}
	pr_debug("Origin-database connection: Success!\n");

	// Connect to target-database.
	pg_db_t->target_conn = PQconnectdbParams(keywords, target_values, 0);
	if (PQstatus(pg_db_t->target_conn) != CONNECTION_OK) {
		pr_debug("Target-database connection: Failed!\n");
		pr_error("%s", PQerrorMessage(pg_db_t->target_conn));
		strncat(email_body, PQerrorMessage(pg_db_t->target_conn),
			EMAIL_BODY_LENGTH - 1);
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

	if (ini_config->email && pg_db_t->pg_conf->email) {
		EmailInfo email_info = {
			.from = ini_config->smtp_config->from,
			.to = (const char *const *)ini_config->smtp_config->to,
			.to_len = ini_config->smtp_config->to_len,
			.cc = (const char *const *)ini_config->smtp_config->cc,
			.cc_len = ini_config->smtp_config->cc_len,
			.body = email_body,
			.smtp_host = ini_config->smtp_config->smtp_host,
			.smtp_username = ini_config->smtp_config->username,
			.smtp_password = ini_config->smtp_config->password,
		};

		int result = send_email(email_info);
		if (result < 0) {
			pr_debug("Unable to send email.\n");
		} else {
			pr_debug("Email was successfully sent!\n");
		}
	}

	free(email_body);
	free(pg_db_t->pg_conf);
}

void setup_command(char **pg_command_path, char **pg_pass, const char *command,
		   const char *password, const char *command_path,
		   int command_path_size, int command_size)
{
	int pg_pass_size = PG_PASS_PREFIX + strlen(password) + 1;

	*pg_pass = realloc(*pg_pass, pg_pass_size * sizeof(char));
	strcpy(*pg_pass, "PGPASSWORD=");
	strcat(*pg_pass, password);

	*pg_command_path =
		realloc(*pg_command_path,
			(command_path_size + command_size + 1) * sizeof(char));
	strncpy(*pg_command_path, command_path,
		command_path_size + command_size + 1);
	strcat(*pg_command_path, command);
}

int exec_command(const char *pg_command_path, char *const args[], char *pg_pass,
		 char *prefixed_command_path)
{
	char const *command_envp[3] = { pg_pass, prefixed_command_path, NULL };
	int ret = execve_binary((char *)pg_command_path, args,
				(char *const *)command_envp);

	if (ret != 0) {
		pr_error("`%s` failed\n", args[0]);
		char temp_buffer[EMAIL_BODY_LENGTH - 1];
		snprintf(temp_buffer, EMAIL_BODY_LENGTH - 1, "%s failed\r\n",
			 args[0]);
		strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);
	}

	return ret;
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
		pr_error("Failure creating the pipe.\n");
		strncat(email_body, "Failure creating the pipe\r\n",
			strlen("Failure creating the pipe\r\n"));
		return -2;
	}

	pg_pid = fork();
	if (pg_pid == 0) {
		close(pipefd[READ_END]);
		dup2(pipefd[WRITE_END], STDERR_FILENO);
		dup2(pipefd[WRITE_END], STDOUT_FILENO);
		close(pipefd[WRITE_END]);
		execve(command_path, command_args, envp);
		pr_error("Error executing `%s`\n", command_path);
		pr_debug("`%s` error: %s\n", command_path, strerror(errno));
		char temp_buffer[EMAIL_BODY_LENGTH - 1];
		snprintf(temp_buffer, EMAIL_BODY_LENGTH - 1,
			 "Error executing `%s`\r\n", command_path);
		strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);
		exit(EXIT_FAILURE);
	} else {
		if (pg_pid == -1) {
			pr_debug(
				"`%s` process failed to start: %s.\n Replication process will be terminated.\n",
				command_path, strerror(errno));
			pr_error("%s\n", strerror(errno));
			char temp_buffer[EMAIL_BODY_LENGTH - 1];
			snprintf(email_body, EMAIL_BODY_LENGTH - 1,
				 "`%s` failed to start: `%s`\r\n", command_path,
				 strerror(errno));
			strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);

			return -2;
		}
	}
	close(pipefd[WRITE_END]);

	if (read_buffer_pipe(pipefd) != 0) {
		pr_error("Error reading from pipe\n");
		pr_debug("`read_buffer_pipe` error: %s\n", strerror(errno));
		char temp_buffer[EMAIL_BODY_LENGTH - 1];
		snprintf(temp_buffer, EMAIL_BODY_LENGTH - 1,
			 "`read_buffer_pipe` error : %s\r\n", strerror(errno));
		strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);
	}

	wait(&wstatus);
	// Check the status of the forked process, anything other than 0 means failure.
	if (wstatus != 0) {
		pr_debug(
			"`%s` was unsuccessful. Replication process will be terminated.\n",
			command_path);
		char temp_buffer[EMAIL_BODY_LENGTH - 1];
		snprintf(temp_buffer, EMAIL_BODY_LENGTH - 1,
			 "`%s` was unsuccessful\r\n", command_path);
		strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);

		return -1;
	}

	return 0;
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
int replicate(struct db_t *pg_db_t)
{
	int ret = 0;
	char *pg_pass = NULL;
	char *pg_command_path = NULL;
	char backup_path[255] = { 0 };
	char *pg_bin, prefixed_command_path[261] = "PATH=";
	char command_path[255] = "/usr/bin/";
	int command_path_size = strlen(command_path) + 1;

	construct_dump_path(backup_path);

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
		pg_db_t->pg_conf->backup_type == SCHEMA ? "-s" : NULL,
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

	pr_debug("Checking if $PG_BIN environmental variable.\n");
	pg_bin = getenv("PG_BIN");
	if (pg_bin) {
		pr_debug("$PG_BIN=%s was found.\n", pg_bin);
		command_path_size = strlen(pg_bin) + 1;
		char temp_buffer[EMAIL_BODY_LENGTH - 1];
		snprintf(temp_buffer, command_path_size, "%s", pg_bin);
		strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);
	} else {
		pr_warn("PG_BIN was not found.\n");
	}
	strncat(prefixed_command_path, command_path, command_path_size + 1);

	setup_command(&pg_command_path, &pg_pass, PG_DUMP_COMMAND,
		      pg_db_t->pg_conf->origin_password, command_path,
		      command_path_size, strlen(PG_DUMP_COMMAND));
	ret = exec_command(pg_command_path, dump_args, pg_pass,
			   prefixed_command_path);
	if (ret != 0)
		goto cleanup;
	strcat(email_body, "\r\n");
	setup_command(&pg_command_path, &pg_pass, PG_RESTORE_COMMAND,
		      pg_db_t->pg_conf->target_password, command_path,
		      command_path_size, strlen(PG_RESTORE_COMMAND));
	ret = exec_command(pg_command_path, restore_args, pg_pass,
			   prefixed_command_path);
	if (ret != 0)
		goto cleanup;

	pr_info("Database Replication was successful!\n");
	char temp_buffer[EMAIL_BODY_LENGTH - 1];
	snprintf(
		temp_buffer, EMAIL_BODY_LENGTH - 1,
		"\r\nReplication of Postgres Database: `%s` was successful\r\n.",
		pg_db_t->pg_conf->origin_database);
	strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);

cleanup:
	free(pg_command_path);
	free(pg_pass);
	return ret;
}

void construct_dump_path(char *path)
{
	char *home_path = getenv("HOME");
	int home_path_size = strlen(home_path) + 1;
	snprintf(path, home_path_size, "%s", home_path);
	strcat(path, "/backup.dump");
}

int read_buffer_pipe(int *pipefd)
{
	char buffer[4096] = { 0 };
	int nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));
	int no_of_new_lines = 0;

	// We keep the sum of the bytes so we can correctly reallocate memory if needed.
	int sum_of_bytes = nbytes;

	if (nbytes < 0) {
		return -1;
	}

	while (nbytes > 0) {
		pr_info("%s", buffer);
		format_buffer(buffer);

		if (sum_of_bytes > 4096) {
			email_body = realloc(email_body,
					     sum_of_bytes * sizeof(char) + 1);
		}

		strncat(email_body, buffer, nbytes + no_of_new_lines + 1);
		memset(buffer, 0, sizeof(buffer));
		nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));
		if (nbytes < 0) {
			return -1;
		}

		sum_of_bytes += nbytes;
	}

	return 0;
}

ADD_FUNC(construct_pg);
