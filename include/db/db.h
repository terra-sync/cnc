#ifndef DB_H
#define DB_H

#include "config.h"
#include <stdbool.h>

#define MAX_AVAILABLE_DBS 10

typedef int (*init_db_func)(void);

typedef struct func_ptr_s {
	init_db_func func;
} func_ptr_t;

#define ADD_FUNC(func_cb)                                    \
	static func_ptr_t ptr_##func_cb                      \
		__attribute((used, section("my_array"))) = { \
			.func = func_cb,                     \
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
/* 	const char *origin; */
/* 	const char *target; */
/* }; */

struct db_t {
	postgres_t *pg_conf;

	void *host_conn;
	void *target_conn;
};

struct options {
	enum backup_type {
		SCHEMA_ONLY = 0,
		FULL,
	} backup_type;
};

struct db_operations {
	struct db_t *db;
	int (*construct)();
	int (*connect)(struct db_t *);
	void (*close)(struct db_t *);
	int (*replicate)(struct db_t *, struct options *);
};

int execute_db_operations(void);

#endif
