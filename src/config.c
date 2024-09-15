#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ini.h>

#include "email.h"
#include "log.h"
#include "util.h"

config_t *ini_config;
char *log_filepath;
bool new_node = false;

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
		// If true, create a new node, and change the head of our linked list.
		if (new_node) {
			postgres_node_t *temp_node =
				CNC_MALLOC(sizeof(postgres_node_t));
			temp_node->next = ini_config->postgres_config;
			ini_config->postgres_config = temp_node;
			new_node = false;
		}
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
			/*
			 * Since we reached the end of the `postgres` section, we need to change
			 * the flag so we can create a new node the next time we enter to
			 * this section.
			 */
			new_node = true;
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

	ini_config = CNC_MALLOC(sizeof(config_t));
	ini_config->postgres_config = CNC_MALLOC(sizeof(postgres_node_t));
	ini_config->postgres_config->next = NULL;
	ini_config->smtp_config = CNC_MALLOC(sizeof(smtp_t));
	ini_config->general_config = CNC_MALLOC(sizeof(general_t));
	ini_config->general_config->log_filepath = NULL;

	initialize_email_sender();

	ret = ini_parse(config_file, handler, ini_config);
	if (ret != 0) {
		return ret;
	}

	ret = construct_log_filepath(ini_config->general_config->log_filepath,
				     &log_filepath);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

/*
 * A helper function to free each node that we allocated. We iterate through the list
 * and free all the fields that we have parsed from the `.ini` file.
 */
void config_free_linked_list(postgres_node_t *head)
{
	postgres_node_t *temp;
	while (head != NULL) {
		temp = head;
		head = head->next;
		free((void *)temp->host.origin);
		free((void *)temp->user.origin);
		free((void *)temp->password.origin);
		free((void *)temp->port.origin);
		free((void *)temp->database.origin);
		free((void *)temp->host.target);
		free((void *)temp->user.target);
		free((void *)temp->password.target);
		free((void *)temp->port.target);
		free((void *)temp->database.target);
		free(temp);
	}
}

void free_config(void)
{
	config_free_linked_list(ini_config->postgres_config);

	free((void *)ini_config->smtp_config->username);
	free((void *)ini_config->smtp_config->password);
	free((void *)ini_config->smtp_config->smtp_port);
	free((void *)ini_config->smtp_config->smtp_host);
	free((void *)ini_config->smtp_config->from);

	for (int i = 0; i < ini_config->smtp_config->to_len; i++) {
		free(ini_config->smtp_config->to[i]);
	}
	for (int i = 0; i < ini_config->smtp_config->cc_len; i++) {
		free(ini_config->smtp_config->cc[i]);
	}

	free((void *)ini_config->general_config->log_filepath);

	free(ini_config->smtp_config);
	free(ini_config->general_config);
	free(ini_config);
	free(log_filepath);
}
