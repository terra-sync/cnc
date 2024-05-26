#ifndef DB_H
#define DB_H

#include <stdio.h>

#include "config.h"

#define MAX_AVAILABLE_DBS 10
typedef int (*init_db_func)(void);

extern init_db_func init_functions[MAX_AVAILABLE_DBS];
extern int num_init_functions;

#define ADD_FUNC(init_func)                                               \
	void __attribute__((constructor)) init_func##_register(void)      \
	{                                                                 \
		if (num_init_functions < MAX_AVAILABLE_DBS) {             \
			init_functions[num_init_functions++] = init_func; \
		}                                                         \
	}

// Macro to iterate over all registered functions
#define section_foreach_entry(elem)                                \
	for (int _index = 0; _index < num_init_functions &&        \
			     ((elem = init_functions[_index]), 1); \
	     ++_index)

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

	void *origin_conn;
	void *target_conn;

	char *log_filename;
	FILE *log_file;
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
	int (*replicate)(struct db_t *);
};

int execute_db_operations(void);

#endif
