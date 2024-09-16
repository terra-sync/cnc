#include "log.h"

#include "db/db.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <sys/syslimits.h> /* Needed for NAME_MAX, PATH_MAX. */
#endif

#include "util.h"
#include "config.h"

bool verbose = false;

extern config_t *ini_config;
extern char *log_filepath;

void construct_log_filename(char *log_filename, const char *log_name)
{
	time_t t = time(NULL);
	struct tm tm;
	localtime_r(&t, &tm);

	snprintf(log_filename, PATH_MAX,
		 "%scnc_%s_%02d%02d%02d%02d%02d%02d.log", log_filepath,
		 log_name, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
		 tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int construct_log_filepath(const char *config_filepath, char **log_filepath)
{
	int ret = 0;

	// If the user didn't specify a path, we create the `cnc` directory under `/var/log/`
	if (config_filepath == NULL || strcmp(config_filepath, "") == 0) {
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

			char current_directory[PATH_MAX + 1];
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
		char path[PATH_MAX + 1];
		int length;

		/* This is to ensure that the user-provided path is correctly terminated with
		 * the '/' character, which is needed to build the directories
		 */
		length = snprintf(path, sizeof(path), "%s", config_filepath);
		strncat(path, "/", length);

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
