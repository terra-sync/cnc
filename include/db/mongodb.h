#ifndef DB_MONGO_H
#define DB_MONGO_H

#include "db/db.h"

/*
 * The format of the uri is
 * mongodb://[username]:[password]@[host]:[port]/[database].
 * Therefore the resulting string will have +15 characters for the '\0'
 * and the default characters of the uri
 */
#define MONGO_URI_SIZE 15
#define MONGO_DUMP_FILE "/mongodb.bson"

/*
 *  construct_mongo
 *
 *  Returns:
 *  0 on success
 * -1 error opening file
 * -ENOMEM Error allocating memory
 *
 */
int construct_mongo();

/*
 * connect_mongo
 *
 *   0 Success
 *  -1 Error to connect
 *  -2 Not enabled
 *  -3 Error constructing URI
 */
int connect_mongo(struct db_t *);
void close_mongo(struct db_t *);
int replicate_mongo(struct db_t *);

#endif
