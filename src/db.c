#include "db/db.h"
#include "db/postgres.h"
#include "log.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <pthread.h>

init_db_func init_functions[MAX_AVAILABLE_DBS];
int num_init_functions = 0;

/* We need to hold info of how many
 * available databases implementations
 * are out there.
 */
struct db_operations **available_dbs;
size_t db_ops_counter = 0;

void *db_thread(void *arg)
{
	struct db_operations *db_ops = (struct db_operations *)arg;
	struct db_t *available_db;

	if (db_ops != NULL) {
		available_db = db_ops->db;
		int ret = db_ops->connect(available_db);
		if (ret == 0) {
			db_ops->replicate(available_db);
		}
		db_ops->close(available_db);
	}

	return NULL;
}

int execute_db_operations(void)
{
	int ret = 0;
	pthread_t threads[db_ops_counter];
	init_db_func init_function;

	available_dbs = (struct db_operations **)calloc(
		MAX_AVAILABLE_DBS, sizeof(struct db_operations **));

	section_foreach_entry(init_function)
	{
		if (db_ops_counter < MAX_AVAILABLE_DBS) {
			int ret = init_function();
			if (ret == -ENOMEM) {
				pr_error("Error allocating memory\n");
				free(available_dbs);
				return ret;
			}
			db_ops_counter++;
		} else {
			pr_info("Max available database number was reached.\n"
				"Executing replication for %ld database systems.\n",
				db_ops_counter);
			break;
		}
	}

	for (int i = 0; i < db_ops_counter; i++) {
		if (available_dbs[i] != NULL) {
			pthread_create(&threads[i], NULL, db_thread,
				       (void *)available_dbs[i]);
		}
	}

	for (int i = 0; i < db_ops_counter; i++) {
		if (available_dbs[i] != NULL) {
			pthread_join(threads[i], NULL);
		}
	}

	free(available_dbs);
	return ret;
}
