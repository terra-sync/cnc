#ifndef UTIL_H
#define UTIL_H

#include <errno.h>

/* Write/Read end for pipes */
#define READ_END 0
#define WRITE_END 1
#define EMAIL_BODY_LENGTH 4096

#define CNC_MALLOC(SIZE)                  \
	({                                \
		void *ptr = malloc(SIZE); \
		if (ptr == NULL) {        \
			return -ENOMEM;   \
		}                         \
		ptr;                      \
	})

/*
 *  read_buffer_pipe()
 *
 *  Returns:
 *  0 on success
 * -1 on failure reading from pipe
 */
int read_buffer_pipe(int *, char *, int *);

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
int execve_binary(char *, char *const[], char *const[], char *, int *);

/*
 * This function is used to append a `\r` character before each `\n`, and it is needed
 * because some email providers don't support receiving emails with "bare" new-line characters.
 */
void format_buffer(char *, int *);

#endif
