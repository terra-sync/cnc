#ifndef DB_POSTGRES_H
#define DB_POSTGRES_H

#include "db/db.h"

int connect_pg(struct db_t *);
void close_pg(struct db_t *pg_db_t);
int replicate(struct db_t *pg_db_t, struct options *pg_options);

#endif
