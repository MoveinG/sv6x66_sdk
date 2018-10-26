/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_SNPRINTF_H__
#define __AYLA_SNPRINTF_H__
#include <stdarg.h>
/*
 * Custom versions of vsnprintf() and snprintf().
 * This is to deal with SDKs that have different formatting support.
 */
int libayla_vsnprintf(char *bp, size_t size, const char *fmt, va_list ap);

int libayla_snprintf(char *bp, size_t size, const char *fmt, ...)
	ADA_ATTRIB_FORMAT(3, 4);

#ifdef ADA_BUILD_AYLA_SNPRINTF
/*
 * Use the above definitions instead of the standard ones if this is included.
 */
#undef snprintf
#define snprintf libayla_snprintf
#undef vsnprintf
#define vsnprintf libayla_vsnprintf
#endif /* ADA_BUILD_AYLA_SNPRINTF */

#endif /* __AYLA_SNPRINTF_H__ */
