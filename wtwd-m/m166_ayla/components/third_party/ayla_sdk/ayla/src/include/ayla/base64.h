/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_BASE64_H__
#define __AYLA_BASE64_H__

/* Given a size, this macro returns what the base64 encoded size would be */
#define BASE64_LEN_EXPAND(x) ((((x) + 2) / 3) * 4)

int ayla_base64_encode(const void *in, size_t in_len,
			void *out, size_t *out_len);
int ayla_base64_decode(const void *inv, size_t in_len,
			void *outv, size_t *out_len);

#endif /* __AYLA_BASE64_H__ */
