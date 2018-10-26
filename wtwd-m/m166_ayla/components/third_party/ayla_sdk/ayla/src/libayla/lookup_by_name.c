/*
 * Copyright 2011 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/nameval.h>

int lookup_by_name(const struct name_val *table, const char *name)
{
	for (; table->name != NULL; table++) {
		if (!strcasecmp(table->name, name)) {
			break;
		}
	}
	return table->val;
}
