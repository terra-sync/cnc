#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ini.h>

#include "log.h"

#define CHECK_SECTION(s) strcmp(section, s) == 0
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

void remove_spaces(char *s)
{
	char *d = s;
	do {
		while (*d == ' ') {
			++d;
		}
	} while ((*s++ = *d++));
}

void config_split_array_string(char *dest_array[], const char *value, int *len)
{
	int idx = 0;
	char *temp_value = strdup(value);

	char *str_ = strtok(temp_value, ",");
	while (str_ != NULL && idx < 10) {
		remove_spaces(str_);
		dest_array[idx++] = str_;
		str_ = strtok(NULL, ",");
	}

	*len = idx;
}

config_t *ini_config;

int handler(void *user, const char *section, const char *name,
	    const char *value)
{
	ini_config = (config_t *)user;

	if (CHECK_SECTION("postgres")) {
		if (MATCH("postgres", "enabled")) {
			if (strcmp("true", value) == 0) {
				ini_config->postgres_config->enabled = true;
			} else if (strcmp("false", value) == 0) {
				ini_config->postgres_config->enabled = false;
			} else {
				pr_error(
					"Accepted `enabled` values are \"true\" or \"false \".\n");
				return 0;
			}
		} else if (MATCH("postgres", "origin_host")) {
			ini_config->postgres_config->origin_host =
				strdup(value);
		} else if (MATCH("postgres", "origin_user")) {
			ini_config->postgres_config->origin_user =
				strdup(value);
		} else if (MATCH("postgres", "origin_password")) {
			ini_config->postgres_config->origin_password =
				strdup(value);
		} else if (MATCH("postgres", "origin_port")) {
			ini_config->postgres_config->origin_port =
				strdup(value);
		} else if (MATCH("postgres", "origin_database")) {
			ini_config->postgres_config->origin_database =
				strdup(value);
		} else if (MATCH("postgres", "target_host")) {
			ini_config->postgres_config->target_host =
				strdup(value);
		} else if (MATCH("postgres", "target_user")) {
			ini_config->postgres_config->target_user =
				strdup(value);
		} else if (MATCH("postgres", "target_password")) {
			ini_config->postgres_config->target_password =
				strdup(value);
		} else if (MATCH("postgres", "target_port")) {
			ini_config->postgres_config->target_port =
				strdup(value);
		} else if (MATCH("postgres", "target_database")) {
			ini_config->postgres_config->target_database =
				strdup(value);
		} else if (MATCH("postgres", "backup_type")) {
			if (strcmp("schema", value) == 0) {
				ini_config->postgres_config->backup_type =
					SCHEMA;
			} else if (strcmp("full", value) == 0) {
				ini_config->postgres_config->backup_type = FULL;
			} else {
				pr_error(
					"Accepted `backup_type` values are \"schema\" or \"full\".\n");
				return 0;
			}
		} else if (MATCH("postgres", "email")) {
			if (strcmp("true", value) == 0) {
				ini_config->postgres_config->email = true;
			} else if (strcmp("false", value) == 0) {
				ini_config->postgres_config->email = false;
			} else {
				pr_error(
					"Accepted `enabled` values are \"true\" or \"false\".\n");
				return 0;
			}
		} else {
			return 0;
		}
	}

	// SMTP
	if (CHECK_SECTION("smtp")) {
		if (MATCH("smtp", "username")) {
			ini_config->smtp_config->username = strdup(value);
		} else if (MATCH("smtp", "password")) {
			ini_config->smtp_config->password = strdup(value);
		} else if (MATCH("smtp", "smtp_host")) {
			ini_config->smtp_config->smtp_host = strdup(value);
		} else if (MATCH("smtp", "smtp_port")) {
			ini_config->smtp_config->smtp_port = strdup(value);
		} else if (MATCH("smtp", "auth_mode")) {
			if (strcmp("ssl", value) == 0) {
				ini_config->smtp_config->auth_mode = SSL;
			} else if (strcmp("tls", value) == 0) {
				ini_config->smtp_config->auth_mode = TLS;
			} else {
				pr_error(
					"Accepted `auth_mode` values are \"ssl\" or \"tls\".\n");
				return 0;
			}
		} else if (MATCH("smtp", "from")) {
			ini_config->smtp_config->from = strdup(value);
		} else if (MATCH("smtp", "to")) {
			config_split_array_string(
				ini_config->smtp_config->to, value,
				&ini_config->smtp_config->to_len);
		} else if (MATCH("smtp", "cc")) {
			config_split_array_string(
				ini_config->smtp_config->cc, value,
				&ini_config->smtp_config->cc_len);
		} else if (MATCH("smtp", "enabled")) {
			if (strcmp("true", value) == 0) {
				ini_config->smtp_config->enabled = true;
			} else if (strcmp("false", value) == 0) {
				ini_config->smtp_config->enabled = false;
			} else {
				pr_error(
					"Accepted `email` values are \"true\" or \"false\".\n");
				return 0;
			}
		}
	}

	return 1;
}

/* 
 * initialize_config
 *
 *   0 Success
 *  -1 Error opening File
 *  -2 Error allocating Memory
 *  >0 Number of line with parsing error
 */
int initialize_config(const char *config_file)
{
	int ret = 0;

	ini_config = (config_t *)malloc(sizeof(config_t));
	ini_config->postgres_config = (postgres_t *)malloc(sizeof(postgres_t));
	ini_config->smtp_config = (smtp_t *)malloc(sizeof(smtp_t));
	ret = ini_parse(config_file, handler, ini_config);

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

	free((void *)ini_config->smtp_config->username);
	free((void *)ini_config->smtp_config->password);
	free((void *)ini_config->smtp_config->smtp_port);
	free((void *)ini_config->smtp_config->smtp_host);

	free((void *)ini_config->smtp_config->from);

	free(ini_config->smtp_config);
	free(ini_config->postgres_config);
	free(ini_config);
}
