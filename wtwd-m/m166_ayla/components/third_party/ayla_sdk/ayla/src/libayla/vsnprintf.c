/*
 * Copyright 2011 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/snprintf.h>

struct fmt_state {
	unsigned alt_form:1;
	unsigned zero_pad:1;
	unsigned left_adj:1;
	unsigned blank_delim:1;
	unsigned sign_req:1;
	unsigned neg:1;
	char fmt_char;
	unsigned char base;
	unsigned long width;
	unsigned long prec;
};

static int printf_putu(char *bp, size_t size,
	struct fmt_state *state, unsigned long long ival)
{
	char nbuf[23];	/* a 64-bit number can be 22 octal digits */
	char *cp = nbuf;
	unsigned long long val = ival;
	int num_width;
	int field_width;
	int zeros;
	int i;
	size_t len = 0;

	do {
		i = val % state->base;
		if (i > 9) {
			if (state->fmt_char == 'X') {
				*cp++ = i - 10 + 'A';
			} else {
				*cp++ = i - 10 + 'a';
			}
		} else {
			*cp++ = i + '0';
		}
		val /= state->base;
	} while (val != 0);

	num_width = cp - nbuf;
	field_width = num_width;
	if (field_width < state->prec) {
		zeros = state->prec - field_width;
		field_width = state->prec;
	} else {
		zeros = 0;
	}
	field_width += (state->sign_req | state->neg | state->blank_delim);
	if (state->alt_form && ival != 0) {
		switch (state->fmt_char) {
		case 'x':
		case 'X':
			field_width += 2;
			break;
		case 'o':
			field_width++;
			break;
		}
	}

	if (state->zero_pad && state->width > field_width) {
		zeros += state->width - field_width;
		field_width = state->width;
	} else if (!state->left_adj) {
		while (state->width > field_width) {
			if (len < size) {
				*bp++ = ' ';
				len++;
			}
			field_width++;
		}
	}

	if (state->neg) {
		if (len < size) {
			*bp++ = '-';
			len++;
		}
	} else if (state->sign_req) {
		if (len < size) {
			*bp++ = '+';
			len++;
		}
	} else if (state->blank_delim) {
		if (len < size) {
			*bp++ = ' ';
			len++;
		}
	}

	while (zeros-- > 0) {
		if (len < size) {
			*bp++ = '0';
			len++;
		}
		field_width++;
	}

	while (cp > nbuf) {
		if (len < size) {
			*bp++ = *--cp;
			len++;
		} else {
			break;
		}
	}

	if (state->left_adj) {
		while (state->width > field_width) {
			if (len < size) {
				*bp++ = ' ';
				len++;
			}
			field_width++;
		}
	}
	return len;
}

static int printf_putn(char *bp, size_t size,
	struct fmt_state *state, long long val)
{
	if (val < 0) {
		state->neg = 1;
		val = -val;
	}
	return printf_putu(bp, size, state, (unsigned long long)val);
}

static int printf_puts(char *bp, size_t size,
	struct fmt_state *state, char *str)
{
	size_t slen;
	size_t i;
	size_t len = 0;

	slen = strlen(str);
	if (!state->left_adj) {
		for (i = slen; state->width > i; i++) {
			if (len < size) {
				*bp++ = ' ';
				len++;
			}
		}
	}
	for (i = 0; i < slen; i++) {
		if (len < size) {
			*bp++ = str[i];
			len++;
		}
	}
	if (state->left_adj) {
		for (i = slen; state->width > i; i++) {
			if (len < size) {
				*bp++ = ' ';
				len++;
			}
		}
	}
	return len;
}

int libayla_vsnprintf(char *bp, size_t size, const char *fmt, va_list ap)
{
	struct fmt_state state;
	char cbuf[2];
	unsigned long long uval = 0;
	char *ptr;
	char len_mod;
	long long val;
	size_t tlen;
	size_t len = 0;

	ASSERT(bp);
	while (*fmt != '\0' && size > len) {
		if (*fmt != '%') {
			*bp++ = *fmt++;
			len++;
			continue;
		}

		state.alt_form = 0;
		state.zero_pad = 0;
		state.blank_delim = 0;
		state.left_adj = 0;
		state.sign_req = 0;
		state.width = 0;
		state.prec = 1;
		state.neg = 0;
		len_mod = 0;
		state.base = 10;

		switch (*++fmt) {
		case '#':
			state.alt_form = 1;
			fmt++;
			break;
		case '-':
			state.left_adj = 1;
			fmt++;
			break;
		case ' ':
			state.blank_delim = 1;
			fmt++;
			break;
		case '+':
			state.sign_req = 1;
			fmt++;
			break;
		case '0':
			state.zero_pad = 1;
			/* fall through */
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			state.width = strtoul(fmt, (char **)&fmt, 10);
			if (*fmt == '.') {
				state.prec = strtoul(fmt + 1,
				    (char **)&fmt, 10);
			}
			break;
		}

		/*
		 * Length modifier
		 */
		switch (*fmt) {
		case 'h':
			len_mod = *fmt++;
			if (*fmt == 'h') {
				len_mod = 'H';
				fmt++;
			}
			break;
		case 'l':
			len_mod = *fmt++;
			if (*fmt == 'l') {
				len_mod = 'q';
				fmt++;
			}
			break;
		case 'q':
			len_mod = *fmt++;
			break;
		case 'j':
			len_mod = *fmt++;
			break;
		case 'z':
			len_mod = *fmt++;
			break;
		case 't':
			len_mod = *fmt++;
			break;
		}

		/*
		 * conversion specifier
		 */
		state.fmt_char = *fmt;
		switch (*fmt) {
		case 'd':
		case 'i':
			switch (len_mod) {
			case 0:
			default:
				val = va_arg(ap, int);
				break;
			case 'l':
				val = va_arg(ap, long);
				break;
			case 'q':
				val = va_arg(ap, long long);
				break;
			case 'z':
				val = va_arg(ap, size_t);
				break;
			case 't':
				val = va_arg(ap, ptrdiff_t);
				break;
			}
			tlen = printf_putn(bp, size - len, &state, val);
			bp += tlen;
			len += tlen;
			fmt++;
			break;
		case 'o':
			state.base = 8;
			goto put_uint;
		case 'u':
put_uint:
			switch (len_mod) {
			case 0:
			default:
				uval = va_arg(ap, unsigned int);
				break;
			case 'l':
				uval = va_arg(ap, unsigned long);
				break;
			case 'q':
				uval = va_arg(ap, unsigned long long);
				break;
			case 'z':
				uval = va_arg(ap, size_t);
				break;
			case 't':
				uval = va_arg(ap, ptrdiff_t);
				break;
			}
			tlen = printf_putu(bp, size - len, &state, uval);
			bp += tlen;
			len += tlen;
			fmt++;
			break;
		case 'x':
		case 'X':
			state.base = 16;
			goto put_uint;
		case 'p':
			state.base = 16;
			uval = (ptrdiff_t)va_arg(ap, void *);
			tlen = printf_putu(bp, size - len, &state, uval);
			bp += tlen;
			len += tlen;
			fmt++;
			break;
		case 'c':
			cbuf[0] = va_arg(ap, int);
			cbuf[1] = '\0';
			tlen = printf_puts(bp, size - len, &state, cbuf);
			bp += tlen;
			len += tlen;
			fmt++;
			break;
		case 's':
			ptr = va_arg(ap, char *);
			if (ptr == NULL) {
				ptr = "(null)";
			}
			tlen = printf_puts(bp, size - len, &state, ptr);
			bp += tlen;
			len += tlen;
			fmt++;
			break;
		case 'n': case 'm': case 'e': case 'E': case 'f':
		case 'F': case 'g': case 'G': case 'a': case 'A':
		case '%':
		default:
			*bp++ = *fmt++;
			len++;
			break;
		}
	}
	if (len < size) {
		*bp = '\0';
	}
	return len;
}
