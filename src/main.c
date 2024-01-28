#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "db/db.h"
#include "log.h"

extern config_t *ini_config;
extern bool verbose;

static struct option options[] = {
	{ "config-file", required_argument, NULL, 'f' },
	{ "verbose", no_argument, NULL, 'v' },
};

int main(int argc, char **argv)
{
	int ret = 0;
	int c;
	char *config_file = NULL;

	while ((c = getopt_long(argc, argv, "f:v", options, NULL)) != -1) {
		switch (c) {
		case 'f':
			config_file = strdup(optarg);
			printf("%s\n", config_file);
			break;
		case 'v':
			verbose = true;
			break;
		}
	}

	if (config_file != NULL) {
		ret = initialize_config(config_file);
		if (ret < 0) {
			if (ret == -1) {
				fprintf(stderr,
					"Config: Error opening `.ini` file\n");
			} else if (ret == -2) {
				fprintf(stderr,
					"Config: Error allocating memory\n");
			}

			free((void *)config_file);
			return ret;
		} else if (ret > 0) {
			fprintf(stderr,
				"Error parsing line: %d. Please check your `.ini.` file\n",
				ret);

			free((void *)config_file);
			free_config();
			return ret;
		}
	}

	ret = execute_db_operations();

	free((void *)config_file);
	free_config();

	return ret;
}
