#ifndef DB_POSTGRES_H
#define DB_POSTGRES_H

#include "config.h"

int pg_connect(void);
void construct_pg_values(char **values);
void set_info_env(void);

#endif
