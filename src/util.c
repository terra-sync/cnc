#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "util.h"
#include "log.h"

void format_buffer(char *buffer, int *email_body_size)
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
			*email_body_size += sizeof('\r');
		}
	}
}

int execve_binary(char *command_path, char *const command_args[],
		  char *const envp[], char *email_body, int *email_body_size)
{
	// A pipe to read the output from the process.
	int pipefd[2];
	int pg_pid;
	int wstatus;

	if (pipe(pipefd) == -1) {
		pr_error("Failure creating the pipe.\n");
		strncat(email_body, "Failure creating the pipe\r\n",
			EMAIL_BODY_LENGTH);
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
		*email_body_size += snprintf(temp_buffer, EMAIL_BODY_LENGTH - 1,
					     "Error executing `%s`\r\n",
					     command_path);
		strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);
		exit(EXIT_FAILURE);
	} else {
		if (pg_pid == -1) {
			pr_debug(
				"`%s` process failed to start: %s.\n Replication process will be terminated.\n",
				command_path, strerror(errno));
			pr_error("%s\n", strerror(errno));
			char temp_buffer[EMAIL_BODY_LENGTH - 1];
			snprintf(temp_buffer, EMAIL_BODY_LENGTH - 1,
				 "`%s` failed to start: `%s`\r\n", command_path,
				 strerror(errno));
			strncat(email_body, temp_buffer, EMAIL_BODY_LENGTH - 1);

			return -2;
		}
	}
	close(pipefd[WRITE_END]);

	if (read_buffer_pipe(pipefd, email_body, email_body_size) != 0) {
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

int read_buffer_pipe(int *pipefd, char *email_body, int *email_body_size)
{
	char buffer[4096] = { 0 };
	int nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));
	int no_of_new_lines = 0;

	// We keep track of the email body size so we can correctly reallocate memory if needed.
	*email_body_size += nbytes;

	if (nbytes < 0) {
		return -1;
	}

	while (nbytes > 0) {
		pr_info("%s", buffer);
		format_buffer(buffer, email_body_size);

		if (*email_body_size > 4096) {
			/*
			 * If we exceed 4096 we should realloc 8192 to make sure
			 * that we do not overflow.
			 */
			email_body =
				realloc(email_body, 8196 * sizeof(char) + 1);
		}

		strncat(email_body, buffer, nbytes + no_of_new_lines + 1);
		memset(buffer, 0, sizeof(buffer));
		nbytes = read(pipefd[READ_END], buffer, sizeof(buffer));
		if (nbytes < 0) {
			return -1;
		}

		*email_body_size += nbytes;
	}

	return 0;
}
