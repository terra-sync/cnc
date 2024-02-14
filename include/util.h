#ifndef UTIL_H
#define UTIL_H

/* Write/Read end for pipes */
#define READ_END 0
#define WRITE_END 1

/*
 * This function is used to append a `\r` character before each `\n`, and it is needed
 * because some email providers don't support receiving emails with "bare" new-line characters.
 */
void format_buffer(char *);

#endif
