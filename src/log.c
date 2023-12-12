#include "log.h"

#include <stdbool.h>
#include <stdio.h>

bool verbose;

bool get_verbose(void)
{
	return verbose;
}
