#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpq-fe.h"

#include "config.h"
#include "db/db.h"
#include "db/postgres.h"

extern config_t *yaml_config;

extern struct db_operations **available_dbs;

static struct db_operations pg_db_ops = {
	.connect = connect_pg,
	.close = close_pg,
	.replicate = replicate,
};

int construct_pg()
{
	printf("construct\n");
	struct db_t *pg_db_t = (struct db_t *)malloc(sizeof(struct db_t));

	pg_db_t->pg_conf = (postgres_t *)malloc(sizeof(postgres_t));
	memcpy(pg_db_t->pg_conf, yaml_config->postgres_config,
	       sizeof(postgres_t));

	pg_db_ops.db = pg_db_t;
	available_dbs[0] = &pg_db_ops;

	return 0;
}

int connect_pg(struct db_t *pg_db_t)
{
	int ret = 0;
	// Initialize the fields that are to be used to connect to postgres
	const char *keywords[] = { "host", "user",   "password",
				   "port", "dbname", 0 };
	// Construct the array that will hold the values of the fields
	const char *values[] = { pg_db_t->pg_conf->origin_host, pg_db_t->pg_conf->origin_user,
				 pg_db_t->pg_conf->origin_password,
				 pg_db_t->pg_conf->origin_port,
				 pg_db_t->pg_conf->origin_database };

	pg_db_t->host_conn = PQconnectdbParams(keywords, values, 0);
	if (PQstatus(pg_db_t->host_conn) != CONNECTION_OK) {
		fprintf(stderr, "%s", PQerrorMessage(pg_db_t->host_conn));
		ret = -1;
	}
	return ret;
}

void close_pg(struct db_t *pg_db_t)
{
	PQfinish(pg_db_t->host_conn);
	free(pg_db_t->pg_conf);
}

int replicate(struct db_t *pg_db_t, struct options *pg_options)
{
	printf("replicate\n");
	return 0;
}

ADD_FUNC(construct_pg);
