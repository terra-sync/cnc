#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

/* Only if `verbose` is enabled (throught "-v" flag)
 * this print is going to print to STDOUT. Name
 * "pr_..." is inspired by pr_{info|warn|error} of
 * the Linux Kernel.
*/
#define pr_info(...)                                       \
	if (get_verbose()) {                               \
		fprintf(stdout, "VERBOSE:%s\t", __func__); \
		fprintf(stdout, __VA_ARGS__);              \
	}

bool get_verbose(void);

#endif
