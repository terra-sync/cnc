#include "db/db.h"
#include "db/postgres.h"
#include "log.h"

#include <bits/pthreadtypes.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <pthread.h>
#include <string.h>

init_db_func init_functions[MAX_AVAILABLE_DBS];
int num_init_functions = 0;

/* We need to hold info of how many
 * available databases implementations
 * are out there.
 */
struct db_operations **available_dbs;
size_t db_ops_counter = 0;
pthread_mutex_t mutex;

void *db_operation_thread(void *arg)
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
	pthread_t threads[MAX_AVAILABLE_DBS];
	init_db_func init_function;

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		printf("Mutex initialization failed\n");
		return 1;
	}

	available_dbs = (struct db_operations **)calloc(
		MAX_AVAILABLE_DBS, sizeof(struct db_operations **));

	section_foreach_entry(init_function)
	{
		int ret = init_function();
		if (ret == -ENOMEM) {
			pr_error("Error allocating memory\n");
			free(available_dbs);
			pthread_mutex_destroy(&mutex);
			return ret;
		} else if (ret == -1) {
			break;
		}
	}

	for (int i = 0; i < db_ops_counter; i++) {
		int ret = pthread_create(&threads[i], NULL, db_operation_thread,
					 (void *)available_dbs[i]);
		if (ret != 0) {
			pr_error("Error creating thread: %s\n", strerror(ret));
			pthread_mutex_destroy(&mutex);
			free(available_dbs);

			return -2;
		}
	}

	for (int i = 0; i < db_ops_counter; i++) {
		if (available_dbs[i] != NULL) {
			int ret = pthread_join(threads[i], NULL);
			if (ret != 0) {
				pr_error("Error joining thread: %s\n",
					 strerror(ret));
				pthread_mutex_destroy(&mutex);
				free(available_dbs[i]);

				return ret;
			}
			free(available_dbs[i]);
		}
	}

	pthread_mutex_destroy(&mutex);

	free(available_dbs);
	return ret;
}
