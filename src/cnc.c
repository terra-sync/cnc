#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "config.h"
#include "db/db.h"
#include "cnc.h"
#include "util.h"

extern config_t *ini_config;
extern bool verbose;
extern FILE *log_file;

static struct option options[] = {
	{ "config-file", required_argument, NULL, 'f' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{ 0, 0, 0, 0 } // End of options
};

void help(void)
{
	pr_info("Usage: cnc [options]\n");
	pr_info("Options:\n");
	pr_info("  -f, --config-file <file>  Specify the config file to use\n");
	pr_info("  -v, --verbose             Run in verbose mode\n");
	pr_info("  -h, --help                Print this help message\n");
	pr_info(" -V, --version Print the version information\n");
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
	while ((c = getopt_long(argc, argv, "f:vhV", options, NULL)) != -1) {
		switch (c) {
		case 'f':
			config_file = strdup(optarg);
			pr_info("%s\n", config_file);
			break;
		case 'v':
			verbose = true;
			break;
		case '?':
		case 'V':
			pr_info("Version: %s\n", VERSION);
			pr_info("Commit: %s\n", COMMIT_HASH);
			return 0;
		case 'h':
		default:
			help();
			return 0;
		}
	}
	// Check for non-option arguments (arguments without '-' or '--')
	if (optind < argc) {
		pr_error("Non-option argument: %s\n", argv[optind]);
		help();
		return -1;
	}

	if (config_file == NULL) {
		help();
		return 0;
	}

	ret = initialize_config(config_file);
	if (ret < 0) {
		if (ret == -1) {
			pr_error("Config: Error opening `.ini` file\n");
		} else if (ret == -2) {
			pr_error("Config: Error allocating memory\n");
		} else if (ret == -3) {
			pr_error("Config: Error opening the log file\n");
		}

		free((void *)config_file);
		return ret;
	} else if (ret > 0) {
		pr_error(
			"Error parsing line: %d. Please check your `.ini` file\n",
			ret);

		free((void *)config_file);
		free_config();
		return ret;
	}

	ret = execute_db_operations();
	free((void *)config_file);
	free_config();

	return ret;
}
