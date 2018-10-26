/*
 * Copyright 2011 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_PARSE_H__
#define __AYLA_PARSE_H__

char *format_mac(const u8 *mac, char *buf, size_t len);

/*
 * Format string which may contain non-loggable characters for logging.
 */
char *format_string(char *result, size_t rlen, const char *buf, size_t len);

int parse_argv(char **argv, int argv_len, char *buf);
int parse_mac(u8 *mac, const char *val);
ssize_t parse_hex(void *buf, size_t len, const char *hex, size_t hex_len);
void parse_url(char *name, char **access, char **host, char **path);
int string_strip(char *src, char *dst, int maxlen);

int parse_http_date(u32 *timep, int argc, char **argv);

int hostname_valid(char *);

#endif /* __AYLA_PARSE_H__ */
