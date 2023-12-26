#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ini.h>

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

config_t *ini_config;

static int handler(void *user, const char *section, const char *name,
		   const char *value)
{
	postgres_t *postgres_config = (postgres_t *)user;

	if (MATCH("postgres", "enabled")) {
		if (strcmp(value, "true") == 0)
			postgres_config->enabled = true;
		else
			postgres_config->enabled = false;
	}

	// Origin
	if (MATCH("postgres", "origin_host")) {
		postgres_config->origin_host = strdup(value);
	} else if (MATCH("postgres", "origin_user")) {
		postgres_config->origin_user = strdup(value);
		printf("%s\n", postgres_config->origin_user);
	} else if (MATCH("postgres", "origin_password")) {
		postgres_config->origin_password = strdup(value);
	} else if (MATCH("postgres", "origin_port")) {
		postgres_config->origin_port = strdup(value);
	} else if (MATCH("postgres", "origin_database")) {
		postgres_config->origin_database = strdup(value);
	}

	// Target
	if (MATCH("postgres", "target_host")) {
		postgres_config->target_host = strdup(value);
	} else if (MATCH("postgres", "target_user")) {
		postgres_config->target_user = strdup(value);
	} else if (MATCH("postgres", "target_password")) {
		postgres_config->target_password = strdup(value);
	} else if (MATCH("postgres", "target_port")) {
		postgres_config->target_port = strdup(value);
	} else if (MATCH("postgres", "target_database")) {
		postgres_config->target_database = strdup(value);
	}

	return 1;
}

/* 
 * initialize_config
 *
 *   0 Success
 *  -1 Error reading file
 *  -2 Error parsing file
 *
 */
int initialize_config(const char *config_file)
{
	int ret = 0;
	postgres_t postgres_config;

	ini_config = (config_t *)malloc(sizeof(config_t));
	ini_config->postgres_config = (postgres_t *)malloc(sizeof(postgres_t));

	if (ini_parse("test.ini", handler, &postgres_config) < 0) {
		return -1;
	}

	memcpy(ini_config->postgres_config, &postgres_config,
	       sizeof(postgres_t));

	return ret;
}

void free_config(void)
{
	free((void *)ini_config->postgres_config->origin_host);
	free((void *)ini_config->postgres_config->origin_user);
	free((void *)ini_config->postgres_config->origin_password);
	free((void *)ini_config->postgres_config->origin_port);
	free((void *)ini_config->postgres_config->origin_database);

	free((void *)ini_config->postgres_config->target_host);
	free((void *)ini_config->postgres_config->target_user);
	free((void *)ini_config->postgres_config->target_password);
	free((void *)ini_config->postgres_config->target_port);
	free((void *)ini_config->postgres_config->target_database);

	free(ini_config->postgres_config);
	free(ini_config);
}
