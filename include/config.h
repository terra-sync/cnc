#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct postgres_t {
	const char *host;
	const char *user;
	const char *password;
	const char *port;
	const char *database;
} postgres_t;

typedef struct config_t {
	postgres_t *postgres_config;
} config_t;

int initialize_config(const char *config_file);
void free_config(void);

#endif
