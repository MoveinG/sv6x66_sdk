/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <ayla/endian.h>
#include <ayla/tlv.h>
#include <ayla/utf8.h>

static ssize_t tlv_put(void *buf, size_t buflen, enum ayla_tlv_type type,
	const void *val, size_t len)
{
	struct ayla_tlv *tlv;

	if ((len > 0xff && type != ATLV_FILE) || len > 0x7eff ||
	    buflen < sizeof(*tlv) + len) {
		return -1;
	}
	((u8 *)buf)[0] = type | (len >> 8);
	((u8 *)buf)[1] = len;
	memcpy(buf + 2, val, len);
	return len + 2;
}

static ssize_t tlv_put_str(void *buf, size_t buflen, const char *val)
{
	return tlv_put(buf, buflen, ATLV_UTF8, val, strlen(val));
}

static ssize_t tlv_put_uint(void *buf, size_t buflen, enum ayla_tlv_type type,
		u32 val)
{
	u8 data[4];

	if (val & 0xffff0000) {
		put_ua_be32(&data, val);
		return tlv_put(buf, buflen, type, data, 4);
	} else if (val & 0xff00) {
		put_ua_be16(&data, val);
		return tlv_put(buf, buflen, type, data, 2);
	} else {
		data[0] = val;
		return tlv_put(buf, buflen, type, data, 1);
	}
}

static ssize_t tlv_put_int(void *buf, size_t buflen, s32 val)
{
	u8 data[4];

	if (val < MIN_S16 || val > MAX_S16) {
		put_ua_be32(&data, val);
		return tlv_put(buf, buflen, ATLV_INT, data, 4);
	} else if (val < MIN_S8 || val > MAX_S8) {
		put_ua_be16(&data, val);
		return tlv_put(buf, buflen, ATLV_INT, data, 2);
	} else {
		data[0] = val;
		return tlv_put(buf, buflen, ATLV_INT, data, 1);
	}
}

static ssize_t tlv_put_conf(void *buf, size_t buflen, enum conf_token *token,
		int ntoken)
{
	u8 *data;
	ssize_t len;
	ssize_t tmp;
	int i;

	if (buflen < 2 || ntoken > 0xff) {
		return -1;
	}

	data = ((u8 *)buf) + 2;
	len = 0;
	buflen -= 2;
	for (i = 0; i < ntoken; i++) {
		tmp = utf8_encode(data, buflen, token[i]);
		if (tmp <= 0) {
			return -1;
		}
		data += tmp;
		len += tmp;
		buflen -= tmp;
	}
	if (len > 0xff) {
		return -1;
	}
	((u8 *)buf)[0] = ATLV_CONF;
	((u8 *)buf)[1] = len;
	return len + 2;
}

enum conf_error tlv_import(u8 *buf, size_t *buflen,
		enum ayla_tlv_type type, const void *data, size_t len)
{
	u32 val;
	s32 sval;
	ssize_t size;

	if (!buf || !buflen || !data) {
		return CONF_ERR_NONE;
	}
	if (*buflen <= 2) {
		return CONF_ERR_LEN;
	}
	switch (type) {
	case ATLV_BOOL:
	case ATLV_UINT:
		switch (len) {
		case sizeof(u8):
			val = *(u8 *)data;
			break;
		case sizeof(u16):
			val = *(u16 *)data;
			break;
		case sizeof(u32):
			val = *(u32 *)data;
			break;
		default:
			return CONF_ERR_RANGE;
		}
		size = tlv_put_uint(buf, *buflen, type, val);
		break;
	case ATLV_INT:
		switch (len) {
		case sizeof(s8):
			sval = *(s8 *)data;
			break;
		case sizeof(s16):
			sval = *(s16 *)data;
			break;
		case sizeof(s32):
			sval = *(s32 *)data;
			break;
		default:
			return CONF_ERR_RANGE;
		}
		size = tlv_put_int(buf, *buflen, sval);
		break;
	case ATLV_UTF8:
		size = tlv_put_str(buf, *buflen, data);
		break;
	case ATLV_CONF:
		size = tlv_put_conf(buf, *buflen, (enum conf_token *)data,
		    len / sizeof(enum conf_token));
		break;
	default:
		size = tlv_put(buf, *buflen, type, data, len);
		break;
	}
	if (size < 0) {
		return CONF_ERR_LEN;
	}

	*buflen = size;
	return CONF_ERR_NONE;
}

size_t tlv_info(const void *tlv, enum ayla_tlv_type *type,
		const u8 **data, size_t *len)
{
	size_t _len;
	_len = ((u8 *)tlv)[1];
	if (((u8 *)tlv)[0] & ATLV_FILE) {
		_len |= (((u8 *)tlv)[0] & (~ATLV_FILE)) << 8;
	}

	if (type) {
		if (((u8 *)tlv)[0] & ATLV_FILE) {
			*type = ATLV_FILE;
		} else {
			*type = ((u8 *)tlv)[0];
		}
	}
	if (len) {
		*len = _len;
	}
	if (data) {
		*data = ((u8 *)tlv) + 2;
	}
	return _len + 2;
}

enum conf_error tlv_export(enum ayla_tlv_type type, void *data,
		size_t *data_len, const void *tlv, size_t len)
{
	enum ayla_tlv_type tlv_type;
	const u8 *tlv_data;
	size_t tlv_len;
	size_t bytes;
	u32 val;
	s32 sval;
	enum conf_token *token;
	size_t ntoken;

	if (len < 2) {
		return CONF_ERR_LEN;
	}
	if (!tlv || !data) {
		return CONF_ERR_NONE;
	}
	bytes = tlv_info(tlv, &tlv_type, &tlv_data, &tlv_len);
	if (bytes > len) {
		return CONF_ERR_LEN;
	}

	switch (type) {
	case ATLV_BOOL:
	case ATLV_UINT:
		switch (tlv_len) {
		case sizeof(u8):
			val = *tlv_data;
			break;
		case sizeof(u16):
			val = get_ua_be16(tlv_data);
			break;
		case sizeof(u32):
			val = get_ua_be32(tlv_data);
			break;
		default:
			return CONF_ERR_LEN;
		}
		switch (*data_len) {
		case sizeof(u8):
			if (val > MAX_U8) {
				return CONF_ERR_RANGE;
			}
			*(u8 *)data = val;
			break;
		case sizeof(u16):
			if (val > MAX_U16) {
				return CONF_ERR_RANGE;
			}
			*(u16 *)data = val;
			break;
		case sizeof(u32):
			*(u32 *)data = val;
			break;
		default:
			return CONF_ERR_LEN;
		}
		break;
	case ATLV_INT:
		switch (tlv_len) {
		case sizeof(s8):
			sval = *(s8 *)tlv_data;
			break;
		case sizeof(s16):
			sval = (s16)get_ua_be16(tlv_data);;
			break;
		case sizeof(s32):
			sval = (s32)get_ua_be32(tlv_data);
			break;
		default:
			return CONF_ERR_LEN;
		}
		switch (*data_len) {
		case sizeof(s8):
			if (sval < MIN_S8 || sval > MAX_S8) {
				return CONF_ERR_RANGE;
			}
			*(s8 *)data = sval;
			break;
		case sizeof(s16):
			if (sval < MIN_S16 || sval > MAX_S16) {
				return CONF_ERR_RANGE;
			}
			*(s16 *)data = sval;
			break;
		case sizeof(s32):
			*(s32 *)data = sval;
			break;
		default:
			return CONF_ERR_LEN;
		}
		break;
	case ATLV_UTF8:
		if (tlv_len + 1 > *data_len) {
			return CONF_ERR_LEN;
		}
		memcpy(data, tlv_data, tlv_len);
		*((char *)data + tlv_len) = '\0';
		*data_len = tlv_len + 1;
		break;
	case ATLV_CONF:
		token = (enum conf_token *)data;
		ntoken = 0;
		*data_len /= sizeof(enum conf_token);
		while (tlv_len) {
			bytes = utf8_get(&val, tlv_data, tlv_len);
			if (bytes == 0 || bytes > tlv_len) {
				return CONF_ERR_UTF8;
			}
			tlv_data += bytes;
			tlv_len -= bytes;
			if (ntoken > *data_len) {
				return CONF_ERR_LEN;
			}
			*(token++) = (enum conf_token)val;
			ntoken++;
		}
		*data_len = ntoken * sizeof(enum conf_token);
		break;
	default:
		if (tlv_len > *data_len) {
			return CONF_ERR_LEN;
		}
		memcpy(data, tlv_data, tlv_len);
		*data_len = tlv_len;
		break;
	}
	return CONF_ERR_NONE;
}
