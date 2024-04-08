#include <string.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/error.h>
#else
#include <error.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include "libpq-fe.h"

#include "config.h"
#include "db/db.h"
#include "db/postgres.h"
#include "log.h"
#include "rust/email-bindings.h"
#include "util.h"

extern FILE *log_file;
extern config_t *ini_config;

extern struct db_operations **available_dbs;
static struct db_operations pg_db_ops = {
	.connect = connect_pg,
	.close = close_pg,
	.replicate = replicate,
};

int construct_pg(void)
{
	struct db_t *pg_db_t = CNC_MALLOC(sizeof(struct db_t));

	pg_db_t->pg_conf = CNC_MALLOC(sizeof(postgres_t));
	memcpy(pg_db_t->pg_conf, ini_config->postgres_config,
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
		return ret;
	}
	pr_info_fd("Summary of Postgres Database: `%s`\n\n",
		   pg_db_t->pg_conf->database.origin);

	// Initialize the fields that are to be used to connect to postgres.
	const char *keywords[] = { "host", "user",   "password",
				   "port", "dbname", NULL };
	// Construct the array that will hold the values of the fields.
	const char *origin_values[] = { pg_db_t->pg_conf->host.origin,
					pg_db_t->pg_conf->user.origin,
					pg_db_t->pg_conf->password.origin,
					pg_db_t->pg_conf->port.origin,
					pg_db_t->pg_conf->database.origin };
	const char *target_values[] = { pg_db_t->pg_conf->host.target,
					pg_db_t->pg_conf->user.target,
					pg_db_t->pg_conf->password.target,
					pg_db_t->pg_conf->port.target,
					pg_db_t->pg_conf->database.target };

	// Connect to origin-database.
	pg_db_t->origin_conn = PQconnectdbParams(keywords, origin_values, 0);
	if (PQstatus(pg_db_t->origin_conn) != CONNECTION_OK) {
		pr_debug_fd("Postgres origin-database connection: Failed!\n");
		pr_error_fd("%s", PQerrorMessage(pg_db_t->origin_conn));

		ret = -1;
		return ret;
	}
	pr_debug("Origin-database connection: Success!\n");

	// Connect to target-database.
	pg_db_t->target_conn = PQconnectdbParams(keywords, target_values, 0);
	if (PQstatus(pg_db_t->target_conn) != CONNECTION_OK) {
		pr_debug_fd("Target-database connection: Failed!\n");
		pr_error_fd("%s", PQerrorMessage(pg_db_t->target_conn));

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

	if (ini_config->smtp_config->enabled && pg_db_t->pg_conf->email) {
		EmailInfo email_info = {
			.from = ini_config->smtp_config->from,
			.to = (const char *const *)ini_config->smtp_config->to,
			.to_len = ini_config->smtp_config->to_len,
			.cc = (const char *const *)ini_config->smtp_config->cc,
			.cc_len = ini_config->smtp_config->cc_len,
			.filepath = ini_config->general_config->log_filepath,
			.smtp_host = ini_config->smtp_config->smtp_host,
			.smtp_username = ini_config->smtp_config->username,
			.smtp_password = ini_config->smtp_config->password,
		};
		// set the file position indicator to start, so we can send the email
		fseek(log_file, 0, SEEK_SET);
		int result = send_email(email_info);
		if (result < 0) {
			pr_error("Unable to send email.\n");
		} else {
			pr_info("Email was successfully sent!\n");
			// set the file position at the end, to continue appending for other database systems
			fseek(log_file, 0, SEEK_END);
		}
	}

	free(pg_db_t->pg_conf);
	free(pg_db_t);
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
		pr_error_fd("`%s` failed\n", args[0]);
	}

	return ret;
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
	char backup_path[PATH_MAX + 1] = { 0 };
	char *pg_bin, prefixed_command_path[PATH_MAX + 6] = "PATH=";
	char command_path[PATH_MAX + 1] = "/usr/bin/";
	int command_path_size = strlen(command_path) + 1;

	construct_filepath(backup_path, PG_DUMP_FILE);

	char *const dump_args[] = {
		PG_DUMP_COMMAND,
		"-h",
		(char *const)pg_db_t->pg_conf->host.origin,
		"-F",
		"custom",
		"-p",
		(char *const)pg_db_t->pg_conf->port.origin,
		"-U",
		(char *const)pg_db_t->pg_conf->user.origin,
		"-d",
		(char *const)pg_db_t->pg_conf->database.origin,
		"-f",
		backup_path,
		"-v",
		pg_db_t->pg_conf->backup_type == SCHEMA ? "-s" : NULL,
		NULL
	};

	char *const restore_args[] = {
		PG_RESTORE_COMMAND,
		"-h",
		(char *const)pg_db_t->pg_conf->host.target,
		"-p",
		(char *const)pg_db_t->pg_conf->port.target,
		"-U",
		(char *const)pg_db_t->pg_conf->user.target,
		"-d",
		(char *const)pg_db_t->pg_conf->database.target,
		backup_path,
		"-v",
		NULL
	};

	pr_debug("Checking if $PG_BIN environmental variable.\n");
	pg_bin = getenv("PG_BIN");
	if (pg_bin) {
		pr_debug("$PG_BIN=%s was found.\n", pg_bin);
		command_path_size = strlen(pg_bin) + 1;
	} else {
		pr_warn_fd("PG_BIN was not found.\n");
	}

	strncat(prefixed_command_path, command_path, command_path_size + 1);

	setup_command(&pg_command_path, &pg_pass, PG_DUMP_COMMAND,
		      pg_db_t->pg_conf->password.origin, command_path,
		      command_path_size, strlen(PG_DUMP_COMMAND));
	ret = exec_command(pg_command_path, dump_args, pg_pass,
			   prefixed_command_path);
	if (ret != 0)
		goto cleanup;

	setup_command(&pg_command_path, &pg_pass, PG_RESTORE_COMMAND,
		      pg_db_t->pg_conf->password.target, command_path,
		      command_path_size, strlen(PG_RESTORE_COMMAND));
	ret = exec_command(pg_command_path, restore_args, pg_pass,
			   prefixed_command_path);
	if (ret != 0)
		goto cleanup;

	pr_info_fd("\nReplication of Postgres Database: `%s` was successful.\n",
		   pg_db_t->pg_conf->database.origin);
cleanup:
	free(pg_command_path);
	free(pg_pass);
	return ret;
}

ADD_FUNC(construct_pg);
