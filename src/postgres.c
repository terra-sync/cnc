#include <string.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/error.h>
#include <sys/syslimits.h> /* Needed for NAME_MAX, PATH_MAX */
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
#include "util.h"
#include "email.h"
#include <stdlib.h>

extern config_t *ini_config;
extern struct db_operations **available_dbs;
extern size_t db_ops_counter;

bool pg_is_enabled(void *postgres_node)
{
	return ((postgres_node_t *)postgres_node)->enabled;
}

const char *pg_get_origin(void *postgres_node)
{
	return ((postgres_node_t *)postgres_node)->database.origin;
}

void *pg_get_next(void *postgres_node)
{
	return ((postgres_node_t *)postgres_node)->next;
}

int construct_pg(void)
{
	postgres_node_t *temp_node = ini_config->postgres_config;
	int ret = construct_db(temp_node, pg_is_enabled, pg_get_next,
			       pg_get_origin, connect_pg, close_pg, replicate,
			       sizeof(*ini_config->postgres_config));

	return ret;
}

int connect_pg(struct db_t *pg_db_t)
{
	int ret = 0;
	/*
	 * We should NULL the connection to avoid using `free` on an
	 * uninitialized connection.
	 */
	pg_db_t->origin_conn = NULL;
	pg_db_t->target_conn = NULL;

	construct_log_filename(
		pg_db_t->log_filename,
		((postgres_node_t *)pg_db_t->db_conf)->database.origin);

	pg_db_t->log_file = fopen(pg_db_t->log_filename, "a");
	if (pg_db_t->log_file == NULL) {
		pr_error("Error opening %s\n", pg_db_t->log_filename);
		return -2;
	}

	pr_info_fd(pg_db_t->log_file, "Summary of Postgres Database: `%s`\n\n",
		   ((postgres_node_t *)pg_db_t->db_conf)->database.origin);

	// Initialize the fields that are to be used to connect to postgres.
	const char *keywords[] = { "host", "user",   "password",
				   "port", "dbname", NULL };
	// Construct the array that will hold the values of the fields.
	const char *origin_values[] = {
		((postgres_node_t *)pg_db_t->db_conf)->host.origin,
		((postgres_node_t *)pg_db_t->db_conf)->user.origin,
		((postgres_node_t *)pg_db_t->db_conf)->password.origin,
		((postgres_node_t *)pg_db_t->db_conf)->port.origin,
		((postgres_node_t *)pg_db_t->db_conf)->database.origin
	};
	const char *target_values[] = {
		((postgres_node_t *)pg_db_t->db_conf)->host.target,
		((postgres_node_t *)pg_db_t->db_conf)->user.target,
		((postgres_node_t *)pg_db_t->db_conf)->password.target,
		((postgres_node_t *)pg_db_t->db_conf)->port.target,
		((postgres_node_t *)pg_db_t->db_conf)->database.target
	};

	// Connect to origin-database.
	pthread_mutex_lock(&mutex);
	pg_db_t->origin_conn = PQconnectdbParams(keywords, origin_values, 0);
	pthread_mutex_unlock(&mutex);
	if (PQstatus(pg_db_t->origin_conn) != CONNECTION_OK) {
		pr_debug_fd(pg_db_t->log_file,
			    "Postgres origin-database connection: Failed!\n");
		pr_error_fd(pg_db_t->log_file, "%s",
			    PQerrorMessage(pg_db_t->origin_conn));

		ret = -1;
		return ret;
	}

	pr_debug("Origin-database connection: Success!\n");

	// Connect to target-database.
	pthread_mutex_lock(&mutex);
	pg_db_t->target_conn = PQconnectdbParams(keywords, target_values, 0);
	pthread_mutex_unlock(&mutex);
	if (PQstatus(pg_db_t->target_conn) != CONNECTION_OK) {
		pr_debug_fd(pg_db_t->log_file,
			    "Target-database connection: Failed!\n");
		pr_error_fd(pg_db_t->log_file, "%s",
			    PQerrorMessage(pg_db_t->target_conn));

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

	if (ini_config->smtp_config->enabled &&
	    ((postgres_node_t *)pg_db_t->db_conf)->email) {
		EmailInfo email_info = {
			.from = ini_config->smtp_config->from,
			.to = (const char **)&ini_config->smtp_config->to,
			.to_len = ini_config->smtp_config->to_len,
			.cc = (const char **)&ini_config->smtp_config->cc,
			.cc_len = ini_config->smtp_config->cc_len,
			.filepath = pg_db_t->log_filename,
			.smtp_host = ini_config->smtp_config->smtp_host,
			.smtp_username = ini_config->smtp_config->username,
			.smtp_password = ini_config->smtp_config->password,
		};
		// set the file position indicator to start, so we can send the email
		fseek(pg_db_t->log_file, 0, SEEK_SET);
		int result = send_email(email_info);
		if (result < 0) {
			pr_error("Unable to send email.\n");
		} else {
			pr_info("Email was successfully sent!\n");
		}
	}

	fclose(pg_db_t->log_file);
	free(pg_db_t->log_filename);
	free(((postgres_node_t *)pg_db_t->db_conf));
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
		 char *prefixed_command_path, FILE *fp)
{
	char const *command_envp[3] = { pg_pass, prefixed_command_path, NULL };
	int ret = execve_binary((char *)pg_command_path, args,
				(char *const *)command_envp, fp);

	if (ret != 0) {
		pr_error_fd(fp, "`%s` failed\n", args[0]);
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
 *  -1 Failure of`pg_dump` or `psql`
 *  -2 Failure of creation of fork() or pipe()
 *
 */
int replicate(struct db_t *pg_db_t)
{
	int ret = 0;
	char *pg_pass = NULL;
	char *pg_command_path = NULL;
	char backup_path[PATH_MAX + 1] = { 0 };
	char backup_filename[NAME_MAX] = { 0 };
	char *pg_bin, prefixed_command_path[PATH_MAX + 6] = "PATH=";
	char command_path[PATH_MAX + 1] = "/usr/bin/";
	int command_path_size = strlen(command_path) + 1;

	construct_sql_dump_file(
		backup_filename,
		((postgres_node_t *)pg_db_t->db_conf)->database.origin);
	construct_filepath(backup_path, backup_filename);

	char *const dump_args[] = {
		PG_DUMP_COMMAND,
		"-U",
		(char *const)((postgres_node_t *)pg_db_t->db_conf)->user.origin,
		"-h",
		(char *const)((postgres_node_t *)pg_db_t->db_conf)->host.origin,
		"-p",
		(char *const)((postgres_node_t *)pg_db_t->db_conf)->port.origin,
		"-l",
		(char *const)((postgres_node_t *)pg_db_t->db_conf)
			->database.origin,
		"-f",
		backup_path,
		"-v",
		((postgres_node_t *)pg_db_t->db_conf)->backup_type == SCHEMA ?
			"-s" :
			NULL,
		NULL
	};

	char *const restore_args[] = {
		PSQL_COMMAND,
		"-h",
		(char *const)((postgres_node_t *)pg_db_t->db_conf)->host.target,
		"-U",
		(char *const)((postgres_node_t *)pg_db_t->db_conf)->user.target,
		"-p",
		(char *const)((postgres_node_t *)pg_db_t->db_conf)->port.target,
		(char *const)((postgres_node_t *)pg_db_t->db_conf)
			->database.target,
		"-f",
		backup_path,
		NULL
	};

	pr_debug("Checking if $PG_BIN environmental variable.\n");
	pg_bin = getenv("PG_BIN");
	if (pg_bin) {
		pr_debug("$PG_BIN=%s was found.\n", pg_bin);
		command_path_size = strlen(pg_bin) + 1;
	} else {
		pr_warn_fd(pg_db_t->log_file, "PG_BIN was not found.\n");
	}

	strncat(prefixed_command_path, command_path, command_path_size + 1);

	setup_command(&pg_command_path, &pg_pass, PG_DUMP_COMMAND,
		      ((postgres_node_t *)pg_db_t->db_conf)->password.origin,
		      command_path, command_path_size, strlen(PG_DUMP_COMMAND));
	ret = exec_command(pg_command_path, dump_args, pg_pass,
			   prefixed_command_path, pg_db_t->log_file);
	if (ret != 0)
		goto cleanup;

	setup_command(&pg_command_path, &pg_pass, PSQL_COMMAND,
		      ((postgres_node_t *)pg_db_t->db_conf)->password.target,
		      command_path, command_path_size, strlen(PSQL_COMMAND));
	ret = exec_command(pg_command_path, restore_args, pg_pass,
			   prefixed_command_path, pg_db_t->log_file);
	if (ret != 0)
		goto cleanup;

	pr_info_fd(pg_db_t->log_file,
		   "\nReplication of Postgres Database: `%s` was successful.\n",
		   ((postgres_node_t *)pg_db_t->db_conf)->database.origin);

cleanup:
	free(pg_command_path);
	free(pg_pass);
	remove(backup_path);
	return ret;
}

ADD_FUNC(construct_pg);
