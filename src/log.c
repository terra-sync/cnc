#include "log.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "config.h"

bool verbose;
FILE *log_file;
extern config_t *ini_config;

void construct_log_filename(char **log_filename, char *log_filepath)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	sprintf(*log_filename, "%scnc%02d%02d%02d_%02d%02d%02d.log",
		log_filepath, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	ini_config->general_config->log_filepath = strdup(*log_filename);
}

int construct_log_filepath(const char *config_filepath, char **log_filepath)
{
	int ret = 0;

	// If the user didn't specify a path, we create the `cnc` directory under `/var/log/`
	if (config_filepath == NULL || strcmp(config_filepath, "") == 0) {
		//stat(LOG_FILEPATH, &st);
		if (access(LOG_FILEPATH, W_OK) == 0) {
			ret = mkdir(LOG_DIRECTORY, 0777);
			if (ret != 0 && errno != EEXIST) {
				pr_error("Error creating directory at '%s'",
					 LOG_FILEPATH);
				return -3;
			} else {
				ret = cnc_strdup(log_filepath, LOG_DIRECTORY);
				if (ret != 0) {
					return ret;
				}
			}
		} else {
			pr_warn("No permissions to create directory on `/var/log/`. "
				"Current directory will be used.\n");

			char current_directory[256];
			if (getcwd(current_directory,
				   sizeof(current_directory)) != NULL) {
				strncat(current_directory, "/",
					sizeof(current_directory) - 1);

				ret = cnc_strdup(log_filepath,
						 current_directory);
				if (ret != 0) {
					return ret;
				}
			}
		}
	} else {
		// else we iterate through the path and create each parent directory needed
		char path[256];
		snprintf(path, sizeof(path), "%s", config_filepath);
		if (*path != '/') {
			pr_error(
				"Please provide an absolute path for log filepath\n");
			return -3;
		}

		if (mkdir_p(path) != 0) {
			return -3;
		}

		ret = cnc_strdup(log_filepath, path);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

bool get_verbose(void)
{
	return verbose;
}
