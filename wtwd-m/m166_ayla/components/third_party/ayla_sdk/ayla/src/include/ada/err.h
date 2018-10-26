/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_ADA_ERR_H__
#define __AYLA_ADA_ERR_H__

#include <al/al_err.h>

enum ada_err {
	AE_OK = 0,
	AE_BUF = -AL_ERR_BUF,
	AE_ALLOC = -AL_ERR_ALLOC,
	AE_ERR = -AL_ERR_ERR,
	AE_NOT_FOUND = -AL_ERR_NOT_FOUND,
	AE_INVAL_VAL = -AL_ERR_INVAL_VAL,
	AE_INVAL_TYPE = -AL_ERR_INVAL_TYPE,
	AE_IN_PROGRESS = -AL_ERR_IN_PROGRESS,
	AE_BUSY = -AL_ERR_BUSY,
	AE_LEN = -AL_ERR_LEN,
	AE_INVAL_STATE = -AL_ERR_INVAL_STATE,
	AE_TIMEOUT = -AL_ERR_TIMEOUT,
	AE_ABRT = -AL_ERR_ABRT,
	AE_RST = -AL_ERR_RST,
	AE_CLSD = -AL_ERR_CLSD,
	AE_NOTCONN = -AL_ERR_NOTCONN,
	AE_INVAL_NAME = -AL_ERR_INVAL_NAME,
	AE_RDONLY = -AL_ERR_RDONLY,
	AE_CERT_EXP = -AL_ERR_CERT_EXP,
	/* Note: add values in <al/err.h> */
	AE_COUNT		/* keep at end */
};

#define ADA_ERR_STRINGS AL_ERR_STRINGS

const char *ada_err_string(enum ada_err);

#endif /* __AYLA_ADA_ERR_H__ */
