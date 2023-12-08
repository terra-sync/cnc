#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "db/postgres.h"

extern config_t *yaml_config;

static struct option options[] = {
	{ "config-file", required_argument, 0, 'f' },
};

int main(int argc, char **argv)
{
	int ret = 0;
	int c;
	char *config_file = NULL;

	while ((c = getopt_long(argc, argv, "f:", options, NULL)) != -1) {
		switch (c) {
		case 'f':
			config_file = strdup(optarg);
			printf("%s\n", config_file);
			break;
		}
	}

	if (config_file != NULL) {
		ret = initialize_config(config_file);
		if (ret != 0) {
			free((void *)config_file);
			return ret;
		}
	}
	ret = pg_connect();

#if DEBUG == 1
	printf("%s\t%s\t%s\n", yaml_config->postgres_config->host,
	       yaml_config->postgres_config->user,
	       yaml_config->postgres_config->database);
#endif
	free((void *)config_file);
	free_config();

	return ret;
}
