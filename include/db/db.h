#ifndef DB_H
#define DB_H

#include "config.h"
#include <stdbool.h>

#define MAX_AVAILABLE_DBS 10

typedef int (*init_db_func)(void);

typedef struct init_db_func_ptr_s {
	init_db_func func;
} init_db_func_ptr_t;

#define ADD_FUNC(init_func)                                  \
	static init_db_func_ptr_t ptr_##init_func            \
		__attribute((used, section("my_array"))) = { \
			.func = init_func,                   \
		}

#define section_foreach_entry(section_name, type_t, elem)  \
	for (type_t *elem = ({                             \
		     extern type_t __start_##section_name; \
		     &__start_##section_name;              \
	     });                                           \
	     elem != ({                                    \
		     extern type_t __stop_##section_name;  \
		     &__stop_##section_name;               \
	     });                                           \
	     ++elem)

/* TODO: Later on when we are going to support */
/* target alongside origin database. */
/* struct d_str_t { */
/*	const char *origin; */
/*	const char *target; */
/* }; */

/* struct db_t
 * Holds the information every database will need.
 *
 * * postgres_t *pg_conf - inherited from the config
 * * void *host_conn     - host connection, should be
 *                         allocated based on the target (e.g. PGconn *)
 * * void *target_conn   - target connection (same logic as host_conn)
 */
struct db_t {
	postgres_t *pg_conf;

	void *host_conn;
	void *target_conn;
};

/* struct options
 * Replication options
 *
 * * enum backup_type - schema-only or full backup strategy
 */
struct options {
	enum backup_type {
		SCHEMA_ONLY = 0,
		FULL,
	} backup_type;
};

/* struct db_operations
 * db_operations structure holds pointers to functions defined by
 * the database driver that performs construct, connect, replicate,
 * and close operations. It also holds an object of db_t that might
 * be needed by the functions.
 *
 * int (*connect)(struct db_t *)   - connect to the database and populate struct db_t *
 *  Returns: 0 if success
 *          -1 to -3 for errors
 * void (*close)(struct db_t *)    - close all connections and free allocated memory.
 *  Returns: 0 if success
 *          -1 to -3 for errors
 * int (*replicate)(struct db_t *) - replication/backup operation
 *  Returns: 0 if success
 *          -1 to -3 for errors
 *
 * Example:
   static struct db_operations pg_db_ops = {
	   .connect = connect_pg,
	   .close = close_pg,
	   .replicate = replicate,
   };
 */
struct db_operations {
	struct db_t *db;
	int (*connect)(struct db_t *);
	void (*close)(struct db_t *);
	int (*replicate)(struct db_t *, struct options *);
};

int execute_db_operations(void);

#endif
