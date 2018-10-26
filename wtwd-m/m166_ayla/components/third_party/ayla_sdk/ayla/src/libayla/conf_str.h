/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
static const char * const conf_token_names[] = {
#define CONF_TOKEN(val, name)	[val] = #name,
#include <ayla/conf_tokens.h>
#undef CONF_TOKEN
};
