#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

#include "db/db.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define LOG_FILEPATH "/var/log/"
#define LOG_DIRECTORY "/var/log/cnc/"

extern bool verbose;
extern FILE *log_file;

static inline bool get_verbose(void)
{
	return verbose;
}

/* Only if `verbose` is enabled (throught "-v" flag)
 * this print is going to print to STDOUT. Name
 * "pr_..." is inspired by pr_{info|warn|error} of
 * the Linux Kernel.
*/
#define pr_debug(...)                                      \
	if (get_verbose()) {                               \
		fprintf(stdout, "VERBOSE:%s\t", __func__); \
		fprintf(stdout, __VA_ARGS__);              \
	}
#define pr_info(...) fprintf(stdout, __VA_ARGS__);
#define pr_error(...)                                           \
	fprintf(stderr, ANSI_COLOR_RED "ERROR:%s\t", __func__); \
	fprintf(stderr, __VA_ARGS__);                           \
	fprintf(stderr, ANSI_COLOR_RESET);
#define pr_warn(...)                                                 \
	fprintf(stderr, ANSI_COLOR_YELLOW "WARNING:%s\t", __func__); \
	fprintf(stderr, __VA_ARGS__);                                \
	fprintf(stderr, ANSI_COLOR_RESET);

#define pr_debug_fd(log_file, ...)                           \
	if (get_verbose()) {                                 \
		fprintf(stdout, "VERBOSE:%s\t", __func__);   \
		fprintf(stdout, __VA_ARGS__);                \
		fprintf(log_file, "VERBOSE:%s\t", __func__); \
		fprintf(log_file, __VA_ARGS__);              \
	}
#define pr_info_fd(log_file, ...) \
	pr_info(__VA_ARGS__);     \
	fprintf(log_file, __VA_ARGS__);
#define pr_error_fd(log_file, ...)                                \
	pr_error(__VA_ARGS__);                                    \
	fprintf(log_file, ANSI_COLOR_RED "ERROR:%s\t", __func__); \
	fprintf(log_file, __VA_ARGS__);                           \
	fprintf(log_file, ANSI_COLOR_RESET);
#define pr_warn_fd(log_file, ...)                                      \
	pr_warn(__VA_ARGS__);                                          \
	fprintf(log_file, ANSI_COLOR_YELLOW "WARNING:%s\t", __func__); \
	fprintf(log_file, __VA_ARGS__);                                \
	fprintf(log_file, ANSI_COLOR_RESET);

void construct_log_filename(struct db_t *db_t, const char *log_name);

/*
 *  construct_log_filepath()
 *
 *  Returns:
 *  0 Success
 * -3 Failure
 */
int construct_log_filepath(const char *, char **);

#endif
