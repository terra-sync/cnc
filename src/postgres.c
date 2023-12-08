#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpq-fe.h"
#include "db/postgres.h"
#include "config.h"

extern config_t *yaml_config;

/*
 * pg_connect
 *
 *  0 Success
 * -1 Database Connection Error
 *
 */
int pg_connect(void)
{
	int ret = 0;
	PGconn *conn;
	// Initialize the fields that are to be used to connect to postgres
	const char *keywords[] = { "host", "user",   "password",
				   "port", "dbname", 0 };
	// Construct the array that will hold the values of the fields
	char *values[5];

	construct_pg_values(values);
	conn = PQconnectdbParams(keywords, (const char *const *)values, 0);
	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQfinish(conn);
		for (int i = 0; i < 5; i++) {
			free(values[i]);
		}
		ret = -1;
		return ret;
	} else if (PQstatus(conn) == CONNECTION_OK) {
		printf("Ping PostgresDB: Success\n");
	}

	PQfinish(conn);
	for (int i = 0; i < 5; i++) {
		free(values[i]);
	}

	return ret;
}

/* contruct_pg_values
 * Construct an array with data provided from our configuration.
 */
void construct_pg_values(char **values)
{
	values[0] = strdup(yaml_config->postgres_config->host);
	values[1] = strdup(yaml_config->postgres_config->user);
	values[2] = strdup(yaml_config->postgres_config->password);
	values[3] = strdup(yaml_config->postgres_config->port);
	values[4] = strdup(yaml_config->postgres_config->database);
}
