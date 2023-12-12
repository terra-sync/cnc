#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct postgres_t {
	bool enabled;

	const char *origin_host;
	const char *origin_user;
	const char *origin_password;
	const char *origin_port;
	const char *origin_database;

	const char *target_host;
	const char *target_user;
	const char *target_password;
	const char *target_port;
	const char *target_database;
} postgres_t;

typedef struct config_t {
	postgres_t *postgres_config;
} config_t;

int initialize_config(const char *config_file);
void free_config(void);

#endif
