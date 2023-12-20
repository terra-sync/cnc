#ifndef DB_MONGO_H
#define DB_MONGO_H

#include "db/db.h"

int construct_mongo();
void construct_origin_uri(char *, struct db_t*);
void construct_target_uri(char *, struct db_t*);
int connect_mongo(struct db_t *);
void close_mongo(struct db_t *);
int mongo_replicate(struct db_t *);

#endif
