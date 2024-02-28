#ifndef DB_POSTGRES_H
#define DB_POSTGRES_H

#include "db/db.h"

/*
 * For the `pg_dump` and `pg_restore` to work we need to set the passwords as env variables,
 * prefixed with `PGPASSWORD=`
 */
#define PG_PASS_PREFIX 11
#define PG_DUMP_COMMAND "pg_dump"
#define PG_RESTORE_COMMAND "pg_restore"
#define PG_DUMP_FILE "/backup.dump"

void construct_dump_path(char *);

/*
 *  construct_pg
 *
 *  Returns:
 *  0 on success
 * -1 error opening file
 * -ENOMEM Error allocating memory
 *
 */
int construct_pg(void);

int connect_pg(struct db_t *);
void close_pg(struct db_t *pg_db_t);
int replicate(struct db_t *pg_db_t);

#endif
