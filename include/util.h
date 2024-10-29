#ifndef UTIL_H
#define UTIL_H

#include <errno.h>
#include <stdio.h>

#include "log.h"

#ifdef _POSIX_C_SOURCE
#include <limits.h>
#else
#define PATH_MAX 4096
#endif

/* Write/Read end for pipes */
#define READ_END 0
#define WRITE_END 1

/* The log filename is of the format `cncDDMMYY_HHMMSS.log */
#define LOG_FILE_SIZE_NAME 21;

static inline void *CNC_MALLOC(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	return ptr;
}

/*
 *  read_buffer_pipe()
 *
 *  Returns:
 *  0 on success
 * -1 on failure reading from pipe
 */
int read_buffer_pipe(int *, FILE *fp);

/*
 * execve_binary
 *  This function utilizes a fork for calling `execve` and reads the
 *  output through the usage of `read_buffer_pipe`.
 *
 * Returns:
 *   0 Success
 *  -1 Failure of`pg_dump` or `psql`
 *  -2 Failure of creation of fork() or pipe()
 *
 */
int execve_binary(char *, char *const[], char *const[], FILE *fp);

/*
 * This function is used to append a `\r` character before each `\n`, and it is needed
 * because some email providers don't support receiving emails with "bare" new-line characters.
 */
void format_buffer(char *);

/*
 * Create the parent directories needed for a given path iteratively.
 */
int mkdir_p(char *path);

/*
 * cnc_strdup()
 * Calls `strdup` with `string_to_dup` as argument, and checks the result.
 * If success, the result is saved on `string`.
 *
 * Returns:
 *  0 Success
 * -ENOMEM of failure of `strdup` call
 *
 */
static inline int cnc_strdup(char **string, char *string_to_dup)
{
	char *temp_string = strdup(string_to_dup);
	if (temp_string != NULL) {
		*string = temp_string;
	} else {
		pr_error("strdup: Failed to allocate memory.\n");
		return -ENOMEM;
	}

	return 0;
}

/*
 * Append a filename to a given path. This function uses
 * PATH_MAX or 4096 as the max characters to be appended.
 */
void construct_filepath(char *path, char *filename);

#endif
