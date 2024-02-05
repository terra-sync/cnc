#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "db/db.h"
#include "cnc.h"

extern config_t *ini_config;
extern bool verbose;

static struct option options[] = {
	{ "config-file", required_argument, NULL, 'f' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 } // End of options
};

void help(void)
{
	printf("Usage: cnc [options]\n");
	printf("Options:\n");
	printf("  -f, --config-file <file>  Specify the config file to use\n");
	printf("  -v, --verbose             Run in verbose mode\n");
	printf("  -h, --help                Print this help message\n");
}

int process_args(int argc, char **argv)
{
	if (argc == 1) {
		help();
		return 0;
	}

	int ret = 0;
	int c;
	char *config_file = NULL;

	optind = 0;
	while ((c = getopt_long(argc, argv, "f:vh", options, NULL)) != -1) {
		switch (c) {
		case 'f':
			config_file = strdup(optarg);
			printf("%s\n", config_file);
			break;
		case 'v':
			verbose = true;
			break;
		case 'h':
		case '?':
		default:
			help();
			return 0;
		}
	}
	// Check for non-option arguments (arguments without '-' or '--')
	if (optind < argc) {
		fprintf(stderr, "Non-option argument: %s\n", argv[optind]);
		help();
		return -1;
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
				"Error parsing line: %d. Please check your `.ini` file\n",
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
