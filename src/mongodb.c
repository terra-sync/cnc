#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/error.h>
#else
#include <error.h>
#endif

#include "config.h"
#include "db/db.h"
#include "db/mongodb.h"
#include "log.h"
#include "rust/email-bindings.h"
#include "util.h"

#include <libmongoc-1.0/mongoc/mongoc.h>

/*
 * The format of the uri is
 * mongodb://[username]:[password]@[host]:[port]/[database].
 * Therefore the resulting string will have +15 characters for the '\0'
 * and the default characters of the uri
 */
#define MONGO_URI_SIZE 15

extern FILE *log_file;
extern config_t *ini_config;

extern struct db_operations **available_dbs;
static struct db_operations mongo_db_ops = {
	.connect = connect_mongo,
	.close = close_mongo,
	.replicate = mongo_replicate,
};

int construct_mongo(void)
{
	printf("mongo_construct\n");
	struct db_t *mongo_db_t = CNC_MALLOC(sizeof(struct db_t));

	mongo_db_t->mongo_conf = CNC_MALLOC(sizeof(mongo_t));
	memcpy(mongo_db_t->mongodb_conf, ini_config->mongodb_config,
	       sizeof(mongo_t));

	pr_info_fd("Summary of MongoDB Database: `%s`\n\n",
		   mongo_db_t->mongodb_conf->database.origin);

	mongo_db_ops.db = mongo_db_t;
	available_dbs[1] = &mongo_db_ops;

	return 0;
}

/*
 * connect_mongo
 *
 *   0 Success
 *  -1 Failure to connect
 *  -2 Not enabled
 */

int connect_mongo(struct db_t *mongo_db_t)
{
	mongo_db_t->origin_conn = NULL;
	mongo_db_t->target_conn = NULL;
	bson_error_t error = { 0 };
	mongoc_database_t *origin_database = NULL;
	mongoc_database_t *target_database = NULL;
	bson_t *command = NULL;
	bson_t reply_origin = BSON_INITIALIZER;
	bson_t reply_target = BSON_INITIALIZER;
	int ret = 0;
	bool ok = true;

	// Check if the database is `enabled` before attempting to connect
	if (!mongo_db_t->mongo_conf->enabled) {
		ret = -2;
		return ret;
	}

	int origin_uri_size = strlen(mongo_db_t->mongodb_conf->host.origin) +
			      strlen(mongo_db_t->mongodb_conf->user.origin) +
			      strlen(mongo_db_t->mongodb_conf->password.origin) +
			      strlen(mongo_db_t->mongodb_conf->port.origin) +
			      strlen(mongo_db_t->mongodb_conf->database.origin) +
			      MONGO_URI_SIZE;

	int target_uri_size = strlen(mongo_db_t->mongodb_conf->host.target) +
			      strlen(mongo_db_t->mongodb_conf->user.target) +
			      strlen(mongo_db_t->mongodb_conf->password.target) +
			      strlen(mongo_db_t->mongodb_conf->port.target) +
			      strlen(mongo_db_t->mongodb_conf->database.target) +
			      MONGO_URI_SIZE;

	char *origin_uri = CNC_MALLOC(origin_uri_size * sizeof(char));
	char *target_uri = CNC_MALLOC(target_uri_size * sizeof(char));
	construct_origin_uri(origin_uri, mongo_db_t);
	construct_target_uri(target_uri, mongo_db_t);

	mongoc_init();

	mongo_db_t->origin_conn = mongoc_client_new(origin_uri);
	if (!((mongoc_client_t *)mongo_db_t->origin_conn)) {
		fprintf(stderr,
			"Failed to create a MongoDB client for origin-database.\n");
		ret = -1;
		return ret;
	}
	mongo_db_t->target_conn = mongoc_client_new(target_uri);
	if (!((mongoc_client_t *)mongo_db_t->target_conn)) {
		fprintf(stderr,
			"Failed to create a MongoDB client for target-database.\n");
		ret = -1;
		return ret;
	}

	origin_database = mongoc_client_get_database(
		(mongoc_client_t *)mongo_db_t->origin_conn, "admin");
	if (!origin_database) {
		fprintf(stderr,
			"Failed to get a MongoDB origin-database handle.\n");
		ret = -1;
	}
	target_database = mongoc_client_get_database(
		(mongoc_client_t *)mongo_db_t->target_conn, "admin");
	if (!target_database) {
		fprintf(stderr,
			"Failed to get a MongoDB target-database handle.\n");
		ret = -1;
	}

	command = BCON_NEW("ping", BCON_INT32(1));

	ok = mongoc_database_command_simple(origin_database, command, NULL,
					    &reply_origin, &error);
	if (!ok) {
		fprintf(stderr, "error: %s\n", error.message);
		ret = 1;
	}
	ok = mongoc_database_command_simple(target_database, command, NULL,
					    &reply_target, &error);
	if (!ok) {
		fprintf(stderr, "error: %s\n", error.message);
		ret = 1;
	}

	pr_debug(
		"Pinged your deployment. You successfully connected to MongoDB!\n");

	free(origin_uri);
	free(target_uri);

	bson_destroy(&reply_origin);
	bson_destroy(&reply_target);
	bson_destroy(command);
	mongoc_database_destroy(origin_database);
	mongoc_database_destroy(target_database);
	mongoc_cleanup();

	return ret;
}

void close_mongo(struct db_t *mongo_db_t)
{
	mongoc_client_destroy((mongoc_client_t *)mongo_db_t->origin_conn);
	mongoc_client_destroy((mongoc_client_t *)mongo_db_t->target_conn);
	free(mongo_db_t->mongo_conf);
	free(mongo_db_t);
	printf("mongo_close\n");
}

int mongo_replicate(struct db_t *mongo_db_t)
{
	printf("mongo_replicate\n");
	return 0;
}

void construct_origin_uri(char *uri, struct db_t *mongo_db_t)
{
	strncpy(uri, "mongodb://", strlen("mongodb://") + 1);
	strncat(uri, mongo_db_t->mongodb_conf->user.origin, strlen(uri));
	strncat(uri, ":", strlen(":") + 1);
	strncat(uri, mongo_db_t->mongodb_conf->password.origin, strlen(uri));
	strncat(uri, "@", strlen("@") + 1);
	strncat(uri, mongo_db_t->mongodb_conf->host.origin, strlen(uri));
	strncat(uri, ":", strlen(":") + 1);
	strncat(uri, mongo_db_t->mongodb_conf->port.origin, strlen(uri));
	strncat(uri, "/", strlen("/") + 1);
	strncat(uri, mongo_db_t->mongodb_conf->database.origin, strlen(uri));
}

void construct_target_uri(char *uri, struct db_t *mongo_db_t)
{
	strncpy(uri, "mongodb://", strlen("mongodb://") + 1);
	strncat(uri, mongo_db_t->mongo_conf->user.target, strlen(uri));
	strncat(uri, ":", strlen(":") + 1);
	strncat(uri, mongo_db_t->mongo_conf->password.target, strlen(uri));
	strncat(uri, "@", strlen("@") + 1);
	strncat(uri, mongo_db_t->mongo_conf->host.target, strlen(uri));
	strncat(uri, ":", strlen(":") + 1);
	strncat(uri, mongo_db_t->mongo_conf->port.target, strlen(uri));
	strncat(uri, "/", strlen("/") + 1);
	strncat(uri, mongo_db_t->mongo_conf->database.target, strlen(uri));
}

ADD_FUNC(construct_mongo);
