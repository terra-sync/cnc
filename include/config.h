#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// enum backup_type - schema-only or full backup strategy

typedef enum backup_type {
	SCHEMA = 0,
	FULL,
} backup_type;

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

	backup_type backup_type;
} postgres_t;

typedef enum auth_mode_t {
	SSL = 1,
	TLS,
} auth_mode_t;

typedef struct smtp_t {
	const char *username;
	const char *password;
	const char *smtp_host;
	const char *smtp_port;
	auth_mode_t auth_mode;

	const char *from;
	const char *to;
	const char *cc;
} smtp_t;

typedef struct config_t {
	postgres_t *postgres_config;
	smtp_t *smtp_config;
} config_t;

int initialize_config(const char *config_file);
void free_config(void);
int handler(void *user, const char *section, const char *name,
	    const char *value);

#endif
