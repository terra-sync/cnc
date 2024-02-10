#include "db/db.h"
#include "db/postgres.h"

#include <stddef.h>
#include <stdlib.h>

/* We need to hold info of how many
 * available databases implementations
 * are out there.
 */
struct db_operations **available_dbs;

int execute_db_operations(void)
{
	int ret = 0;
	struct db_t *available_db;
	available_dbs = (struct db_operations **)calloc(
		MAX_AVAILABLE_DBS, sizeof(struct db_operations **));

	section_foreach_entry(my_array, init_db_func_ptr_t, entry)
	{
		entry->func();
	}

	for (int i = 0; i < MAX_AVAILABLE_DBS; i++) {
		/* do db_operations for all available db impl. */
		if (available_dbs[i] != NULL) {
			available_db = available_dbs[i]->db;
			ret = available_dbs[i]->connect(available_db);
			if (ret != 0) {
				available_dbs[i]->close(available_db);
				return ret;
			}
			available_dbs[i]->replicate(available_db);
			available_dbs[i]->close(available_db);
		}
	}

	free(available_dbs);

	return 0;
}
