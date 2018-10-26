/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#include <sys/types.h>
#include <stddef.h>
#include <ayla/utypes.h>
#include <ayla/parse.h>

/*
 * Format string for printing, limiting length and converting non-printable
 * characters.
 */
char *format_string(char *result, size_t rlen, const char *buf, size_t len)
{
	char c;
	char *to = result;

	while (len > 0 && rlen > 1) {
		c = *buf++;
		len--;
		if (c < 0x20) {
			c = '.';
		}
		*to++ = c;
		rlen--;
	}
	*to = '\0';
	return result;
}
