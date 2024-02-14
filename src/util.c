#include "util.h"

void format_buffer(char *buffer)
{
	int i = 0, num_of_new_lines = 0;
	while (buffer[i] != '\0') {
		if (buffer[i] == '\n') {
			num_of_new_lines++;
		}
		i++;
	}

	while (i > 0) {
		buffer[i + num_of_new_lines] = buffer[i];
		if (buffer[i + num_of_new_lines] == '\n') {
			num_of_new_lines--;
			buffer[i + num_of_new_lines] = '\r';
		}
		i--;
	}
}
