#include "utils.h"

int matches(const char *prefix, const char *string)
{
	if (!*prefix)
		return false;

	while (*string && *prefix == *string) {
		prefix++;
		string++;
	}

	return !*prefix;
}
