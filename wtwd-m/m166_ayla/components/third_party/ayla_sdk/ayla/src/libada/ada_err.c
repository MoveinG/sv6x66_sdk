/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ada/err.h>

static const char * const ada_err_strings[] = ADA_ERR_STRINGS;

const char *ada_err_string(enum ada_err err)
{
	err = -err;

	if (err < 0 || err >= ARRAY_LEN(ada_err_strings)) {
		return "unknown";
	}
	return ada_err_strings[err];
}
