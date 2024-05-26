#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "util.h"
#include "log.h"

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
		pr_error_fd(fp, "Failure creating the pipe.\n");

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
	char buffer[4096] = { 0 };
	int nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));

	if (nbytes < 0) {
		return -1;
	}

	while (nbytes > 0) {
		fprintf(fp, "%s", buffer);
		memset(buffer, 0, sizeof(buffer));
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
