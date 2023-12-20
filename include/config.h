#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct d_str_t {
	const char *origin;
	const char *target;
} d_str_t;

// enum backup_type - schema-only or full backup strategy
typedef enum backup_type {
	SCHEMA = 0,
	FULL,
} backup_type;

typedef struct postgres_t {
	bool enabled;

	d_str_t host;
	d_str_t user;
	d_str_t password;
	d_str_t port;
	d_str_t database;

	backup_type backup_type;
	bool email;
} postgres_t;


typedef struct mongodb_t {
	bool enabled;

	d_str_t host;
	d_str_t user;
	d_str_t password;
	d_str_t port;
	d_str_t database;

	bool email;
} mongodb_t;

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
	char *to[10];
	int to_len;
	char *cc[10];
	int cc_len;

	bool enabled;
} smtp_t;

typedef struct general_t {
	const char *log_filepath;
} general_t;

typedef struct config_t {
	postgres_t *postgres_config;
	mongodb_t *mongodb_config;
	smtp_t *smtp_config;
	general_t *general_config;
} config_t;

/* 
 * initialize_config
 *
 *   0 Success
 *  -1 Error opening File
 *  -2 Error allocating memory (`inih` library)
 *  -3 Error creating directory
 *  -ENOMEM Error allocating memory (CNC_MALLOC macro)
 *  >0 Number of line with parsing error
 */
int initialize_config(const char *config_file);
void free_config(void);
int handler(void *user, const char *section, const char *name,
	    const char *value);

/* Utilities */
void config_split_array_string(char *dest_array[], const char *value, int *len);

inline static void remove_spaces(char *s)
{
	char *d = s;
	do {
		while (*d == ' ') {
			++d;
		}
	} while ((*s++ = *d++));
}

#endif
