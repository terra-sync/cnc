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
	struct db_t *pg_db_t = malloc(sizeof(struct db_t *));

	pg_db_t->pg_conf = (postgres_t *)malloc(sizeof(postgres_t));
	memcpy(pg_db_t->pg_conf, yaml_config->postgres_config,
	       sizeof(postgres_t));

	pg_db_ops.db = pg_db_t;
	available_dbs[0] = &pg_db_ops;

	return 0;
}

int connect_pg(struct db_t *pg_db_t)
{
	printf("connect\n");
	printf("%s\n", pg_db_t->pg_conf->origin_host);
	printf("%s\n", pg_db_t->pg_conf->origin_user);
	printf("%s\n", pg_db_t->pg_conf->origin_password);
	return 0;
}

void close_pg(struct db_t *pg_db_t)
{
	printf("close\n");
	free(pg_db_t->pg_conf);
}

int replicate(struct db_t *pg_db_t, struct options *pg_options)
{
	printf("replicate\n");
	return 0;
}

ADD_FUNC(construct_pg);
