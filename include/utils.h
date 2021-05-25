#ifndef __EOIP_UTILS_H__
#define __EOIP_UTILS_H__

#include <stdbool.h>

/* Returns true if 'prefix' is a not empty prefix of 'string'.
 */
int matches(const char *prefix, const char *string);

#endif /* __EOIP_UTILS_H__ */
