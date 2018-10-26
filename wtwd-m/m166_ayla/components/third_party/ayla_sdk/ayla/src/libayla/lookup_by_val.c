/*
 * Copyright 2011 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/nameval.h>

const char *lookup_by_val(const struct name_val *table, int val)
{
	for (; table->name != NULL; table++) {
		if (table->val == val) {
			return table->name;
		}
	}
	return NULL;
}
