#include "db/db.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <sys/syslimits.h> /* Needed for NAME_MAX. */
#endif

#include "util.h"
#include "log.h"

extern int db_ops_counter;
extern struct db_operations **available_dbs;

void format_buffer(char *buffer)
{
	int i = 0, num_of_new_lines = 0;
	while (buffer[i] != '\0') {
		if (buffer[i] == '\n') {
			num_of_new_lines++;
		}
		i++;
	}

	for (; i > 0; i--) {
		buffer[i + num_of_new_lines] = buffer[i];
		if (buffer[i + num_of_new_lines] == '\n') {
			num_of_new_lines--;
			buffer[i + num_of_new_lines] = '\r';
		}
	}
}

int execve_binary(char *command_path, char *const command_args[],
		  char *const envp[], FILE *fp)
{
	// A pipe to read the output from the process.
	int pipefd[2];
	int pg_pid;
	int wstatus;

	if (pipe(pipefd) == -1) {
		pr_error_fd(fp, "Failure creating the pipe: %s.\n",
			    strerror(errno));
		return -2;
	}

	pg_pid = fork();
	if (pg_pid == 0) {
		close(pipefd[READ_END]);
		dup2(pipefd[WRITE_END], STDERR_FILENO);
		dup2(pipefd[WRITE_END], STDOUT_FILENO);
		close(pipefd[WRITE_END]);

		execve(command_path, command_args, envp);
		pr_error_fd(fp, "Error executing `%s`\n", command_path);
		pr_debug("`%s` error: %s\n", command_path, strerror(errno));

		exit(EXIT_FAILURE);
	} else {
		if (pg_pid == -1) {
			pr_debug(
				"`%s` process failed to start: %s.\n Replication process will be terminated.\n",
				command_path, strerror(errno));
			pr_error_fd(fp, "%s\n", strerror(errno));

			return -2;
		}
	}
	close(pipefd[WRITE_END]);

	if (read_buffer_pipe(pipefd, fp) != 0) {
		pr_error_fd(fp, "Error reading from pipe\n");
		pr_debug("`read_buffer_pipe` error: %s\n", strerror(errno));
	}

	wait(&wstatus);
	// Check the status of the forked process, anything other than 0 means failure.
	if (wstatus != 0) {
		pr_debug(
			"`%s` was unsuccessful. Replication process will be terminated.\n",
			command_path);
		pr_error_fd(fp, "`%s` was unsuccessful\r\n", command_path);

		return -1;
	}
	return 0;
}

int read_buffer_pipe(int *pipefd, FILE *fp)
{
	char buffer[4096];
	int nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));

	if (nbytes < 0) {
		return -1;
	}

	while (nbytes > 0) {
		buffer[nbytes] = '\0'; // Null-terminate the buffer
		fprintf(fp, "%s", buffer);

		nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));
		if (nbytes < 0) {
			return -1;
		}
	}

	return 0;
}

/*
 *  mkdir_p()
 *
 *  Returns:
 *  0 Success
 * -1 Failure
 */
int mkdir_p(char *path)
{
	int ret = 0;
	struct stat st = { 0 };

	ret = stat(path, &st);
	if (ret == 0) {
		if (access(path, W_OK) != 0) {
			pr_error(
				"Error accessing directory %s, permission denied.\n",
				path);
			return -1;
		}
	} else {
		/*
		 * We use the `p` pointer to find each '/', then we temporary replace the '/'
		 * character with '\0', so we can actually create the directory.
		 * After that the '\0' gets replaced with '/' so we can continue iterating
		 * through the path. The final directory won't be created this way, therefore
		 * we have another call to `mkdir` after our `for` loop to make sure all the
		 * directories are created.
		 */
		char *p;
		for (p = path + 1; *p; p++) {
			if (*p == '/') {
				*p = '\0';
				ret = mkdir(path, 0777);
				if (ret != 0 && errno == EACCES) {
					pr_error(
						"Error creating directory %s, permission denied.\n",
						path);
					return ret;
				}
				*p = '/';
			}
		}

		ret = mkdir(path, 0777);
		if (ret != 0 && errno == EACCES) {
			pr_error(
				"Error creating directory %s, permission denied.\n",
				path);
			return ret;
		}

		size_t len = strlen(path);
		if (len > 0 && path[len - 1] != '/') {
			strcat(path, "/");
		}
	}

	return 0;
}

void construct_filepath(char *path, char *filename)
{
	char *home_path = getenv("HOME");
	snprintf(path, PATH_MAX - 1, "%s", home_path);
	strncat(path, filename, PATH_MAX - strlen(path) - 1);
}

void construct_sql_dump_file(char *backup_filename, const char *database_name)
{
	snprintf(backup_filename, NAME_MAX - 1, "/%s.sql", database_name);
}

int construct_db(void *db_config, bool (*is_enabled)(void *),
		 void *(*get_next)(void *), const char *(*get_origin)(void *),
		 int (*connect_func)(struct db_t *),
		 void (*close_func)(struct db_t *),
		 int (*replicate_func)(struct db_t *), size_t size_of_node)
{
	struct db_operations *db_ops;

	/*
	 * Iterate through the linked list, and for each node check if the
	 * database is enabled. If it is, allocate memory for the various
	 * structs and populate the `available_dbs` array.
	 */
	while (db_config != NULL) {
		if (db_ops_counter < MAX_AVAILABLE_DBS) {
			if (!is_enabled(db_config)) {
				pr_info("Database: `%s` is disabled, skipping.\n",
					get_origin(db_config));
				db_config = get_next(db_config);
				continue;
			}

			struct db_t *db_t = CNC_MALLOC(sizeof(struct db_t));
			db_t->db_conf = CNC_MALLOC(size_of_node);
			db_t->log_filename =
				CNC_MALLOC(sizeof(char) * (PATH_MAX + 1));
			db_ops = CNC_MALLOC(sizeof(struct db_operations));

			db_ops->connect = connect_func;
			db_ops->close = close_func;
			db_ops->replicate = replicate_func;

			memcpy(db_t->db_conf, db_config, size_of_node);
			db_ops->db = db_t;
			available_dbs[db_ops_counter] = db_ops;

			db_config = get_next(db_config);
			db_ops_counter++;
		} else {
			pr_warn("Max available database number was reached.\n");

			return -1;
		}
	}

	return 0;
}
