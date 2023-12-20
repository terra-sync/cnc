#include <string.h>
#include <stdlib.h>
#include <ini.h>

#include "config.h"
#include "log.h"
#include "util.h"

config_t *ini_config;
extern FILE *log_file;

#define CHECK_SECTION(s) strcmp(section, s) == 0
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

void config_split_array_string(char *dest_array[], const char *value, int *len)
{
	int idx = 0;
	char *temp_value = strdup(value);

	char *str_ = strtok(temp_value, ",");
	while (str_ != NULL && idx < 10) {
		remove_spaces(str_);
		dest_array[idx] = strdup(str_);
		if (dest_array[idx] == NULL) {
			free(temp_value);
			return;
		}
		idx++;
		str_ = strtok(NULL, ",");
	}

	*len = idx;
	free(temp_value); // Free the duplicated string after tokenizing.
}

int handler(void *user, const char *section, const char *name,
	    const char *value)
{
	ini_config = (config_t *)user;

	if (CHECK_SECTION("general")) {
		if (MATCH("general", "log_filepath")) {
			ini_config->general_config->log_filepath =
				strdup(value);
		}
	}

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
			ini_config->postgres_config->host.origin =
				strdup(value);
		} else if (MATCH("postgres", "origin_user")) {
			ini_config->postgres_config->user.origin =
				strdup(value);
		} else if (MATCH("postgres", "origin_password")) {
			ini_config->postgres_config->password.origin =
				strdup(value);
		} else if (MATCH("postgres", "origin_port")) {
			ini_config->postgres_config->port.origin =
				strdup(value);
		} else if (MATCH("postgres", "origin_database")) {
			ini_config->postgres_config->database.origin =
				strdup(value);
		} else if (MATCH("postgres", "target_host")) {
			ini_config->postgres_config->host.target =
				strdup(value);
		} else if (MATCH("postgres", "target_user")) {
			ini_config->postgres_config->user.target =
				strdup(value);
		} else if (MATCH("postgres", "target_password")) {
			ini_config->postgres_config->password.target =
				strdup(value);
		} else if (MATCH("postgres", "target_port")) {
			ini_config->postgres_config->port.target =
				strdup(value);
		} else if (MATCH("postgres", "target_database")) {
			ini_config->postgres_config->database.target =
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
	
	if (CHECK_SECTION("mongodb")) {
		if (MATCH("mongodb", "enabled")) {
			if (strcmp("true", value) == 0) {
				ini_config->mongodb_config->enabled = true;
			} else if (strcmp("false", value) == 0) {
				ini_config->mongodb_config->enabled = false;
			} else {
				pr_error(
					"Accepted `enabled` values are \"true\" or \"false \".\n");
				return 0;
			}
		} else if (MATCH("mongodb", "origin_host")) {
			ini_config->mongodb_config->host.origin =
				strdup(value);
		} else if (MATCH("mongodb", "origin_user")) {
			ini_config->mongodb_config->user.origin =
				strdup(value);
		} else if (MATCH("mongodb", "origin_password")) {
			ini_config->mongodb_config->password.origin =
				strdup(value);
		} else if (MATCH("mongodb", "origin_port")) {
			ini_config->mongodb_config->port.origin =
				strdup(value);
		} else if (MATCH("mongodb", "origin_database")) {
			ini_config->mongodb_config->database.origin =
				strdup(value);
		} else if (MATCH("mongodb", "target_host")) {
			ini_config->mongodb_config->host.target =
				strdup(value);
		} else if (MATCH("mongodb", "target_user")) {
			ini_config->mongodb_config->user.target =
				strdup(value);
		} else if (MATCH("mongodb", "target_password")) {
			ini_config->mongodb_config->password.target =
				strdup(value);
		} else if (MATCH("mongodb", "target_port")) {
			ini_config->mongodb_config->port.target =
				strdup(value);
		} else if (MATCH("mongodb", "target_database")) {
			ini_config->mongodb_config->database.target =
				strdup(value);
		} else if (MATCH("mongodb", "email")) {
			if (strcmp("true", value) == 0) {
				ini_config->mongodb_config->email = true;
			} else if (strcmp("false", value) == 0) {
				ini_config->mongodb_config->email = false;
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

int initialize_config(const char *config_file)
{
	int ret = 0;
	char *log_filepath = NULL;
	char *log_filename = malloc(sizeof(char) * (PATH_MAX + 1));

	ini_config = CNC_MALLOC(sizeof(config_t));
	ini_config->postgres_config = CNC_MALLOC(sizeof(postgres_t));
	ini_config->mongodb_config = CNC_MALLOC(sizeof(mongodb_t));
	ini_config->smtp_config = CNC_MALLOC(sizeof(smtp_t));
	ini_config->general_config = CNC_MALLOC(sizeof(general_t));
	ini_config->general_config->log_filepath = NULL;
	ret = ini_parse(config_file, handler, ini_config);

	if (ret != 0) {
		free(log_filename);
		return ret;
	}
	ret = construct_log_filepath(ini_config->general_config->log_filepath,
				     &log_filepath);
	if (ret != 0) {
		free(log_filename);
		return ret;
	}
	construct_log_filename(&log_filename, log_filepath);
	log_file = fopen(log_filename, "a");

	if (log_file == NULL) {
		return -3;
	}

	free(log_filepath);
	free(log_filename);

	return ret;
}

void free_config(void)
{
	free((void *)ini_config->postgres_config->host.origin);
	free((void *)ini_config->postgres_config->user.origin);
	free((void *)ini_config->postgres_config->password.origin);
	free((void *)ini_config->postgres_config->port.origin);
	free((void *)ini_config->postgres_config->database.origin);

	free((void *)ini_config->postgres_config->host.target);
	free((void *)ini_config->postgres_config->user.target);
	free((void *)ini_config->postgres_config->password.target);
	free((void *)ini_config->postgres_config->port.target);
	free((void *)ini_config->postgres_config->database.target);

	free((void *)ini_config->smtp_config->username);
	free((void *)ini_config->smtp_config->password);
	free((void *)ini_config->smtp_config->smtp_port);
	free((void *)ini_config->smtp_config->smtp_host);

	if (ini_config->general_config->log_filepath != NULL) {
		free((void *)ini_config->general_config->log_filepath);
	}
	free((void *)ini_config->smtp_config->from);

	for (int i = 0; i < ini_config->smtp_config->to_len; i++) {
		free(ini_config->smtp_config->to[i]);
	}
	for (int i = 0; i < ini_config->smtp_config->cc_len; i++) {
		free(ini_config->smtp_config->cc[i]);
	}

	free(ini_config->smtp_config);
	free(ini_config->postgres_config);
	free(ini_config->general_config);
	free(ini_config);
	if (log_file != NULL) {
		fclose(log_file);
	}
}
