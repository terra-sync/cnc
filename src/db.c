#include "db/db.h"
#include "db/postgres.h"
#include "log.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/errno.h>

init_db_func init_functions[MAX_AVAILABLE_DBS];
int num_init_functions = 0;

/* We need to hold info of how many
 * available databases implementations
 * are out there.
 */
struct db_operations **available_dbs;
size_t db_ops_counter = 0;

int execute_db_operations(void)
{
	int ret = 0;
	struct db_t *available_db;
	available_dbs = (struct db_operations **)calloc(
		MAX_AVAILABLE_DBS, sizeof(struct db_operations **));
	init_db_func init_function;

	section_foreach_entry(init_function)
	{
		if (db_ops_counter < MAX_AVAILABLE_DBS) {
			int ret = init_function();
			if (ret == -ENOMEM) {
				pr_error_fd("Error allocating memory\n");
				free(available_dbs);
				return ret;
			}
			db_ops_counter++;
		} else {
			pr_info_fd(
				"Max available database number was reached.\n"
				"Executing replication for %ld database systems.\n",
				db_ops_counter);
			break;
		}
	}

	for (int i = 0; i < db_ops_counter; i++) {
		/* do db_operations for all available db impl. */
		if (available_dbs[i] != NULL) {
			available_db = available_dbs[i]->db;
			ret = available_dbs[i]->connect(available_db);
			if (ret == 0) {
				available_dbs[i]->replicate(available_db);
			}
			available_dbs[i]->close(available_db);
		}
	}

	free(available_dbs);

	return 0;
}
