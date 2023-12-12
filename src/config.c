#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cyaml/cyaml.h>

static const cyaml_schema_field_t postgres_fields_schema[] = {
	CYAML_FIELD_BOOL("enabled", CYAML_FLAG_DEFAULT, postgres_t, enabled),

	CYAML_FIELD_STRING_PTR("origin_host", CYAML_FLAG_POINTER, postgres_t,
			       origin_host, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("origin_user", CYAML_FLAG_POINTER, postgres_t,
			       origin_user, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("origin_password", CYAML_FLAG_POINTER,
			       postgres_t, origin_password, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("origin_port", CYAML_FLAG_POINTER, postgres_t,
			       origin_port, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("origin_database", CYAML_FLAG_POINTER,
			       postgres_t, origin_database, 0, CYAML_UNLIMITED),

	CYAML_FIELD_STRING_PTR("target_host", CYAML_FLAG_POINTER, postgres_t,
			       target_host, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("target_user", CYAML_FLAG_POINTER, postgres_t,
			       target_user, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("target_password", CYAML_FLAG_POINTER,
			       postgres_t, target_password, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("target_port", CYAML_FLAG_POINTER, postgres_t,
			       target_port, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("target_database", CYAML_FLAG_POINTER,
			       postgres_t, target_database, 0, CYAML_UNLIMITED),
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
	free((void *)yaml_config->postgres_config->origin_host);
	free((void *)yaml_config->postgres_config->origin_user);
	free((void *)yaml_config->postgres_config->origin_password);
	free((void *)yaml_config->postgres_config->origin_port);
	free((void *)yaml_config->postgres_config->origin_database);

	free((void *)yaml_config->postgres_config->target_host);
	free((void *)yaml_config->postgres_config->target_user);
	free((void *)yaml_config->postgres_config->target_password);
	free((void *)yaml_config->postgres_config->target_port);
	free((void *)yaml_config->postgres_config->target_database);

	free(yaml_config->postgres_config);
	free(yaml_config);
}
