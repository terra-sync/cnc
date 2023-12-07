#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cyaml/cyaml.h>

static const cyaml_schema_field_t postgres_fields_schema[] = {
	CYAML_FIELD_BOOL_PTR("enabled", CYAML_FLAG_POINTER, postgres_t,
			     enabled),
	CYAML_FIELD_STRING_PTR("host", CYAML_FLAG_POINTER, postgres_t, host, 0,
			       CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("user", CYAML_FLAG_POINTER, postgres_t, user, 0,
			       CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("password", CYAML_FLAG_POINTER, postgres_t,
			       password, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("port", CYAML_FLAG_POINTER, postgres_t, port, 0,
			       CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("database", CYAML_FLAG_POINTER, postgres_t,
			       database, 0, CYAML_UNLIMITED),
};

static const cyaml_schema_field_t config_fields_schema[] = {
	CYAML_FIELD_MAPPING_PTR("postgres", CYAML_FLAG_POINTER, config_t,
				postgres_config, postgres_fields_schema),
};

static const cyaml_schema_value_t config_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, config_t, config_fields_schema),
};

static const cyaml_config_t config = {
	.log_fn = cyaml_log, /* Use the default logging function. */
	.mem_fn = cyaml_mem, /* Use the default memory allocator. */
	.log_level = CYAML_LOG_WARNING, /* Logging errors and warnings only. */
};

config_t *yaml_config;

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
	enum cyaml_err err;
	err = cyaml_load_file(config_file, &config, &config_schema,
			      (void **)&yaml_config, NULL);
	if (err != CYAML_OK) {
		if (err == CYAML_ERR_FILE_OPEN)
			ret = -1;
		fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
		ret = -2;

		return ret;
	}

	return ret;
}

void free_config(void)
{
	free((void *)yaml_config->postgres_config->host);
	free((void *)yaml_config->postgres_config->user);
	free((void *)yaml_config->postgres_config->password);
	free((void *)yaml_config->postgres_config->port);
	free((void *)yaml_config->postgres_config->database);
	free(yaml_config->postgres_config);
	free(yaml_config);
}
