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

extern FILE *log_file;
extern config_t *ini_config;

extern struct db_operations **available_dbs;
static struct db_operations mongo_db_ops = {
	.connect = connect_mongo,
	.close = close_mongo,
	.replicate = replicate_mongo,
};

int construct_mongo(void)
{
	struct db_t *mongo_db_t = CNC_MALLOC(sizeof(struct db_t));

	mongo_db_t->mongodb_conf = CNC_MALLOC(sizeof(mongodb_t));
	memcpy(mongo_db_t->mongodb_conf, ini_config->mongodb_config,
	       sizeof(mongodb_t));

	mongo_db_ops.db = mongo_db_t;
	available_dbs[1] = &mongo_db_ops;
	
	return 0;
}

int connect_mongo(struct db_t *mongo_db_t)
{
	mongo_db_t->origin_conn = NULL;
	mongo_db_t->target_conn = NULL;
	char *origin_uri_string;
	char *target_uri_string;
	mongoc_uri_t *origin_uri;
	mongoc_uri_t *target_uri;
	bson_error_t error;
	int ret = 0;

	// Check if the database is `enabled` before attempting to connect
	if (!mongo_db_t->mongodb_conf->enabled) {
		ret = -2;
		return ret;
	}

	mongoc_init();

	pr_info_fd("Summary of MongoDB Database: `%s`\n\n",
		   mongo_db_t->mongodb_conf->database.origin);

	origin_uri_string =
		bson_strdup_printf("mongodb://%s:%s@%s:%s/%s",
				   mongo_db_t->mongodb_conf->user.origin,
				   mongo_db_t->mongodb_conf->password.origin,
				   mongo_db_t->mongodb_conf->host.origin,
				   mongo_db_t->mongodb_conf->port.origin,
				   mongo_db_t->mongodb_conf->database.origin);
	target_uri_string =
		bson_strdup_printf("mongodb://%s:%s@%s:%s/%s",
				   mongo_db_t->mongodb_conf->user.target,
				   mongo_db_t->mongodb_conf->password.target,
				   mongo_db_t->mongodb_conf->host.target,
				   mongo_db_t->mongodb_conf->port.target,
				   mongo_db_t->mongodb_conf->database.target);

	origin_uri = mongoc_uri_new_with_error(origin_uri_string, &error);
	if (!origin_uri) {
		pr_error_fd("failed to parse origin URI: %s\n"
			    "error message: %s\n",
			    origin_uri_string, error.message);
		return -3;
	}

	target_uri = mongoc_uri_new_with_error(target_uri_string, &error);
	if (!origin_uri) {
		pr_error_fd("failed to parse target URI: %s\n"
			    "error message: %s\n",
			    target_uri_string, error.message);
		return -3;
	}

	mongo_db_t->origin_conn = mongoc_client_new_from_uri(origin_uri);
	if (!mongo_db_t->origin_conn) {
		pr_error_fd("Failed to connect to origin-database server.\n");
		ret = -1;
		return ret;
	}
	mongo_db_t->target_conn = mongoc_client_new_from_uri(target_uri);
	if (!mongo_db_t->target_conn) {
		pr_error_fd("Failed to connect to target-database server.\n");
		ret = -1;
		return ret;
	}

	free(origin_uri_string);
	free(target_uri_string);

	mongoc_uri_destroy(origin_uri);
	mongoc_uri_destroy(target_uri);

	return ret;
}

void close_mongo(struct db_t *mongo_db_t)
{
	mongoc_client_destroy(mongo_db_t->origin_conn);
	mongoc_client_destroy(mongo_db_t->target_conn);
	free(mongo_db_t->mongodb_conf);
	free(mongo_db_t);
	mongoc_cleanup();
}

/*
  *  mongoc_dump_collection
  *
  *  Returns:
  *  0 on success
  * -1 Error writing to file
  * -2 Error opening file
  *
  */
static int mongoc_dump_collection(mongoc_client_t *client, const char *database,
				  const char *collection)
{
	mongoc_collection_t *col;
	mongoc_cursor_t *cursor;
	const bson_t *doc;
	bson_error_t error;
	bson_t query = BSON_INITIALIZER;
	FILE *stream;
	char path[256] = { 0 };
	int ret = 0;

	construct_filepath(path, MONGO_DUMP_FILE);

	stream = fopen(path, "w");
	if (!stream) {
		pr_error_fd("Failed to open \"%s\", aborting.\n", path);
		ret = -2;
		return ret;
	}

	col = mongoc_client_get_collection(client, database, collection);
	cursor = mongoc_collection_find_with_opts(col, &query, NULL, NULL);

	while (mongoc_cursor_next(cursor, &doc)) {
		if (BSON_UNLIKELY(doc->len != fwrite(bson_get_data(doc), 1,
						     doc->len, stream))) {
			pr_error_fd("Failed to write %u bytes to %s\n",
				    doc->len, path);
			ret = -1;
			goto cleanup;
		}
	}

	if (mongoc_cursor_error(cursor, &error)) {
		pr_error_fd("ERROR: %s\n", error.message);
		ret = -1;
	}

cleanup:
	fclose(stream);
	mongoc_cursor_destroy(cursor);
	mongoc_collection_destroy(col);

	return ret;
}

int mongodb_dump(struct db_t *mongo_db_t)
{
	mongoc_database_t *db;
	bson_error_t error;
	char **str;
	int ret = 0;

	BSON_ASSERT_PARAM(mongo_db_t->origin_conn);

	db = mongoc_client_get_database(
		mongo_db_t->origin_conn,
		mongo_db_t->mongodb_conf->database.origin);
	str = mongoc_database_get_collection_names_with_opts(db, NULL, &error);
	for (int i = 0; str[i]; i++) {
		mongoc_dump_collection(
			mongo_db_t->origin_conn,
			mongo_db_t->mongodb_conf->database.origin, str[i]);
	}

	mongoc_database_destroy(db);
	bson_strfreev(str);

	return ret;
}

int replicate_mongo(struct db_t *mongo_db_t)
{
	mongodb_dump(mongo_db_t);
	return 0;
}

ADD_FUNC(construct_mongo);
