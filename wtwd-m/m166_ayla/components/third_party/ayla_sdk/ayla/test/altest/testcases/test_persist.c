/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jsmn.h>
#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/jsmn_get.h>
#include <al/al_os_mem.h>

#include <string.h>
#include <ayla/log.h>
#include <al/al_utypes.h>
#include <ayla/utypes.h>
#include <al/al_err.h>
#include <al/al_aes.h>
#include <al/al_random.h>
#include <al/al_os_thread.h>
#include <al/al_persist.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

#define DATA_LEN 256 /* Data length for normal data read/write */
#define MAX_DATA_LEN (1024+1) /* Data length will cause write-failure */
#define MAX_NAME_LEN (32+1) /* Name length will cause read/write failue */

static const char g_data[DATA_LEN] =
"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10"
"\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20"
"\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30"
"\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40"
"\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50"
"\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60"
"\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70"
"\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\x80"
"\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90"
"\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0"
"\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0"
"\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0"
"\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0"
"\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0"
"\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0"
"\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF\x00"
;
static const char *gs_names[] = {
	"abcdefgh",
	"ABCDEFGH",
	"12345678",
	"+-*/<>?~",
	"!@#$%^&*()_+",
	"好好学习",
};

static char *persist_test_case_proc_C610401(const struct pfm_test_desc *pcase,
	int argc, char **argv);
static char *persist_test_case_proc_C610392(const struct pfm_test_desc *pcase,
	int argc, char **argv);

static void make_random_name(char *name, ssize_t len)
{
	int i;
	len -= 1;
	al_random_fill(name, len);
	for (i = 0; i < len; i++) {
		if (!name[i]) {
			name[i] = 'A' + i % 0x30;
		}
	}
	name[len] = '\0';
}

/*
C610391	test al_persist_data_read calling in mutil-thread
1.no assert
2.al_persist_data_read read data success
*/
static void persist_test_case_mt_test_cb(struct pfm_tf_mthread_info *info)
{
	/* struct pfm_tf_mthread_desc *mthread_desc = info->desc; */
	char *result;
	int n; /* index of name */
	int t; /* test counter in a round */
	int r; /* round */
	int role;
	ssize_t size;
	enum al_persist_section sect;
	char buff[6 * DATA_LEN];
	char data[6 * DATA_LEN];
	const char *pname;
	char *pbuff;
	char *pdata;
	enum al_err err;

	role = info->id;
	if (role >= 2) {
		result = "Error: too many threads";
		pfm_test_frame_mthread_error(info, result);
		return;
	}
	for (r = 0; r < 500; r++) {
		if (al_os_thread_get_exit_flag(info->tid)) {
			break;
		}
		if ((r % 2) == 0) {
			al_random_fill(data, sizeof(data));
			memset(buff, 0, sizeof(buff));
		}
		for (t = 0; t < 6; t++) {
			sect = t / 3;
			n = t % 3;
			pname = gs_names[role * 3 + n];
			pdata = data + (sect * 3 + n) * DATA_LEN;
			pbuff = buff + (sect * 3 + n) * DATA_LEN;
			if ((r % 2) == 0) {
				err = al_persist_data_write(sect, pname,
					pdata, DATA_LEN);
				if (err != AL_ERR_OK) {
					result = "al_persist_data_write()"
						"error";
					pfm_test_frame_mthread_error(info,
						result);
					return;
				}
				continue;
			}
			size = al_persist_data_read(sect, pname,
				pbuff, DATA_LEN);
			if (size != DATA_LEN) {
				result = "al_persist_data_read() size error";
				pfm_test_frame_mthread_error(info, result);
				return;
			}
			if (memcmp(pbuff, pdata, DATA_LEN)) {
				result = "al_persist_data_compare() error";
				pfm_test_frame_mthread_error(info, result);
				return;
			}
		}
	}
	pfm_test_frame_mthread_error(info, NULL);
}

/*
C610383	test al_persist_data_read with normal parameters
1.no assert
2.al_persist_data_read read data success
*/
static char *persist_test_case_proc_C610383(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_persist_section sect;
	char buff[DATA_LEN];
	ssize_t size;
	int i;
	/* Prepare data for reading */
	persist_test_case_proc_C610401(pcase, argc, argv); /* Erase */
	persist_test_case_proc_C610392(pcase, argc, argv); /* Write */
	/* Testing */
	for (sect = AL_PERSIST_STARTUP; sect <= AL_PERSIST_FACTORY; sect++) {
		for (i = 0; i < ARRAY_LEN(gs_names); i++) {
			memset(buff, 0, DATA_LEN);
			size = al_persist_data_read(sect, gs_names[i],
				buff, DATA_LEN);
			if (size != DATA_LEN) {
				goto end1;
			}
			if (memcmp(buff, g_data, DATA_LEN)) {
				goto end2;
			}
		}
	}
	return NULL;
end1:
	return "C610383: al_persist_data_read() size error";

end2:
	return "C610383: al_persist_data_read() data error";
}

/*
C610384	test al_persist_data_read when section is not in
enum al_persist_section.
1.assert
*/
static char *persist_test_case_proc_C610384(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	ssize_t size;
	enum al_err err;
	char buff[DATA_LEN];
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_persist_data_read(AL_PERSIST_FACTORY + 1, gs_names[0],
			buff, DATA_LEN);
		err = (size <= 0) ? AL_ERR_ERR : AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610384: al_persist_data_read( invalid section ) "
		"should be failure";
}

/*
C610385	test al_persist_data_read when name is null
1.assert
*/
static char *persist_test_case_proc_C610385(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	ssize_t size;
	enum al_err err;
	char buff[DATA_LEN];
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_persist_data_read(AL_PERSIST_FACTORY, NULL,
			buff, DATA_LEN);
		err = (size <= 0) ? AL_ERR_ERR : AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610385: al_persist_data_read( name = null ) "
		"should be failure";
}

/* C610386 test al_persist_data_read when buf is null
   1.assert
*/
static char *persist_test_case_proc_C610386(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	ssize_t size;
	enum al_err err;
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		err = al_persist_data_write(AL_PERSIST_FACTORY, gs_names[0],
			g_data, DATA_LEN);
		size = al_persist_data_read(AL_PERSIST_FACTORY, gs_names[0],
			NULL, DATA_LEN);
		err = (size <= 0) ? AL_ERR_ERR : AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610386: al_persist_data_read( buf = null ) "
		"should be failure";
}

/*
C610387	test al_persist_data_read when len is 0
1. no assert
2. success
*/
static char *persist_test_case_proc_C610387(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	ssize_t size;
	enum al_err err;
	char buff[DATA_LEN];
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		err = al_persist_data_write(AL_PERSIST_FACTORY, gs_names[0],
			g_data, DATA_LEN);
		size = al_persist_data_read(AL_PERSIST_FACTORY, gs_names[0],
			buff, 0);
		err = (size <= 0) ? AL_ERR_ERR : AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610387: al_persist_data_read( len = 0 ) should be failure";
}

/*
C610388	test al_persist_data_read with long name
1.assert or failure
*/
static char *persist_test_case_proc_C610388(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	ssize_t size;
	enum al_err err;
	char buff[DATA_LEN];
	char name[MAX_NAME_LEN+1];

	make_random_name(name, sizeof(name));
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_persist_data_read(AL_PERSIST_STARTUP, name,
				buff, DATA_LEN);
		err = (size > 0) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK)
		return NULL; /* Pass */
	return "C610388: al_persist_data_read( long name ) should be failure";
}

/*
C610389	test al_persist_data_read when length of data value is up to 2K
1.no assert
2.al_persist_data_read() return success
*/
static char *persist_test_case_proc_C610389(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	ssize_t size;
	enum al_err err;
	char buff[MAX_DATA_LEN];
	err = al_persist_data_write(AL_PERSIST_STARTUP,
		gs_names[0], g_data, DATA_LEN);
	if (err != AL_ERR_OK) {
		return "C610389: write in preparation failure";
	}
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		al_persist_data_write(AL_PERSIST_STARTUP, gs_names[0],
			g_data, DATA_LEN);
		size = al_persist_data_read(AL_PERSIST_STARTUP, gs_names[0],
			buff, MAX_DATA_LEN);
		if (size != DATA_LEN)
			err = AL_ERR_LEN;
		else if (memcmp(buff, g_data, DATA_LEN))
			err = AL_ERR_INVAL_VAL;
		else
			err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect success */
	if (err == AL_ERR_LEN) {
		return "C610389: al_persist_data_read( long data ) "
			"size error";
	} else if (err == AL_ERR_INVAL_VAL) {
		return "C610389: al_persist_data_read( long data ) "
			"data error";
	} else if (err == AL_ERR_ERR) {
		return "C610389: al_persist_data_read( long data ) "
			"assert error";
	}
	return NULL;
}

/*
C610390	test al_persist_data_read when item is not exist
1.no assert
2.al_persist_data_read return error code
*/
static char *persist_test_case_proc_C610390(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	ssize_t size;
	enum al_err err;
	char buff[DATA_LEN];
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		al_persist_data_erase(AL_PERSIST_STARTUP);
		size = al_persist_data_read(AL_PERSIST_STARTUP, gs_names[0],
			buff, DATA_LEN);
		err = (size <= 0) ? AL_ERR_ERR : AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610390: al_persist_data_read( non exist name )"
		"shoud be failure";
}


/*
C610392	test al_persist_data_write with normal parameters
1.no assert
2.al_persist_data_write write data success
*/
static char *persist_test_case_proc_C610392(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_persist_section sect;
	enum al_err err;
	int i;
	for (sect = AL_PERSIST_STARTUP; sect <= AL_PERSIST_FACTORY; sect++) {
		for (i = 0; i < ARRAY_LEN(gs_names); i++) {
			err = al_persist_data_write(sect, gs_names[i],
				g_data, DATA_LEN);
			if (err != AL_ERR_OK)
				goto end;
		}
	}
end:
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL;
	}
	return "C610392: al_persist_data_write() error";
}

/*
C610393	test al_persist_data_write when section is not in
enum al_persist_section
1.assert
*/
static char *persist_test_case_proc_C610393(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		err = al_persist_data_write(AL_PERSIST_FACTORY+1,
			gs_names[0], g_data, DATA_LEN);
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL;
	}
	return "C610393: al_persist_data_write( invalid ection )"
		"shoud be failure";
}

/*
C610394	test al_persist_data_write when name is null
1.assert
*/
static char *persist_test_case_proc_C610394(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		err = al_persist_data_write(AL_PERSIST_FACTORY,
			NULL, g_data, DATA_LEN);
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL;
	}
	return "C610394: al_persist_data_write( name = null )"
		"shoud be failure";
}

/*
C610395	test al_persist_data_write when buf is null
1.no assert
2.al_persist_data_write return success
3.the item is set to empty
*/
static char *persist_test_case_proc_C610395(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_err err;
	err = al_persist_data_write(AL_PERSIST_FACTORY,
		gs_names[0], NULL, DATA_LEN);
	if (err == AL_ERR_OK) {
		return NULL;
	}
	return "C610395: al_persist_data_write( buf = null ) failure";
}

/*
C610396	test al_persist_data_write when len is 0
1.no assert
2.al_persist_data_write return success
3.the item is set to empty
*/
static char *persist_test_case_proc_C610396(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_err err;

	err = al_persist_data_write(AL_PERSIST_FACTORY, gs_names[0],
		g_data, 0);
	if (err == AL_ERR_OK) {
		return NULL;
	}
	return "C610396: al_persist_data_write( len = 0 ) failure";
}

/*
C610397	test al_persist_data_write when length of name is up to 2K
1. assert or return failure
*/
static char *persist_test_case_proc_C610397(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_persist_section sect;
	enum al_err err;
	char name[MAX_NAME_LEN+1];
	make_random_name(name, sizeof(name));
	for (sect = 0; sect <= AL_PERSIST_FACTORY; sect++) {
		rc = pfm_try_catch_assert();
		if (rc == 0) {
			err = al_persist_data_write(sect, name,
				g_data, DATA_LEN);
		} else {
			err = AL_ERR_ERR;
		}
		pfm_try_catch_final();
		/* Expect failure */
		if (err == AL_ERR_OK) {
			break;
		}
	}
	if (sect > AL_PERSIST_FACTORY)
		return NULL;
	return "C610397: al_persist_data_write( long name ) should be failure";
}

/*
C610398	test al_persist_data_write when length of data value is up to 2K
1.assert
*/
static char *persist_test_case_proc_C610398(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	enum al_persist_section sect;
	char data[MAX_DATA_LEN];
	al_random_fill(data, sizeof(data));
	for (sect = 0; sect <= AL_PERSIST_FACTORY; sect++) {
		rc = pfm_try_catch_assert();
		if (rc == 0) {
			err = al_persist_data_write(sect, gs_names[0],
				data, MAX_DATA_LEN);
		} else {
			err = AL_ERR_ERR;
		}
		pfm_try_catch_final();
		/* Expect failure */
		if (err == AL_ERR_OK) {
			break;
		}
	}
	if (sect > AL_PERSIST_FACTORY) {
		return NULL;
	}
	return "C610398: al_persist_data_write( long data ) should be failure";
}

/*
C610400	test al_persist_data_write calling in mutil-thread
1.no assert
2.al_persist_data_write write data success
See:	test_case_persist_desc_mtt_C610400
*/

/*
C610401	test al_persist_data_erase with normal parameters
1.no assert
2.al_persist_data_erase return AL_ERR_OK
*/
static char *persist_test_case_proc_C610401(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_persist_section sect;
	enum al_err err;
	for (sect = AL_PERSIST_STARTUP; sect < AL_PERSIST_FACTORY; sect++) {
		err = al_persist_data_erase(sect);
		if (err != AL_ERR_OK) {
			break;
		}
	}
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL;
	}
	return "C610401: al_persist_data_erase() failure";
}

/*
C610402	test al_persist_data_erase when section is not in
enum al_persist_section
1.assert
*/
static char *persist_test_case_proc_C610402(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		err = al_persist_data_erase(AL_PERSIST_FACTORY+1);
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL;
	}
	return "C610402: al_persist_data_erase( invalid section )"
		"shoulb be failure";
}

/*
C610403	test al_persist_data_erase when persist is empty
1.no assert
2.al_persist_data_erase return AL_ERR_OK
*/
static char *persist_test_case_proc_C610403(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	return NULL;
}

/*
C610404	test double call al_persist_data_erase
1.no assert
2.al_persist_data_erase return AL_ERR_OK
*/
static char *persist_test_case_proc_C610404(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_persist_section sect;
	enum al_err err;

	for (sect = 0; sect < AL_PERSIST_FACTORY; sect++) {
		err = al_persist_data_erase(sect);
		if (err != AL_ERR_OK)
			break;
		err = al_persist_data_erase(sect);
		if (err != AL_ERR_OK)
			break;
	}
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL;
	}
	return "C610404: al_persist_data_erase() twice failed";
}

/*
C610405	test al_persist_data_read after calling al_persist_data_erase
1.no assert
2.al_persist_data_erase return AL_ERR_OK
3.al_persist_data_read read return AL_ERR_ERR
*/
static char *persist_test_case_proc_C610405(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	enum al_persist_section sect;
	enum al_err err;
	char buff[DATA_LEN];
	ssize_t size;
	int rc;
	for (sect = 0; sect < AL_PERSIST_FACTORY; sect++) {
		err = al_persist_data_write(sect, gs_names[0],
			g_data, DATA_LEN);
		if (err != AL_ERR_OK) {
			return "C610405: write in preparation fails";
		}

		err = al_persist_data_erase(sect);
		if (err != AL_ERR_OK) {
			return "C610405: erase in preparation fails";
		}

		rc = pfm_try_catch_assert();
		if (rc == 0) {
			size = al_persist_data_read(sect, gs_names[0],
				buff, DATA_LEN);
			err = (size > 0) ? AL_ERR_OK : AL_ERR_ERR;
		} else {
			err = AL_ERR_ERR;
		}
		pfm_try_catch_final();
		/* Expect failure */
		if (err != AL_ERR_OK) {
			continue;
		}
		return "C610405: al_persist_data, read after erase "
				"should be faulure";
	}
	return NULL; /* Pass */
}

/*
C610406	test al_persist_data_write after calling al_persist_data_erase
1.no assert
2.al_persist_data_erase return AL_ERR_OK
3.al_persist_data_write write data success
*/
static char *persist_test_case_proc_C610406(const struct pfm_test_desc *pcase,
int argc, char **argv)
{
	enum al_persist_section sect;
	enum al_err err;
	for (sect = 0; sect < AL_PERSIST_FACTORY; sect++) {
		err = al_persist_data_erase(sect);
		if (err != AL_ERR_OK) {
			return "C610406: erase in preparation fails";
		}
		err = al_persist_data_write(sect, gs_names[0],
			g_data, DATA_LEN);
		if (err != AL_ERR_OK) {
			return "C610406: write after erase fails";
		}
	}
	return NULL; /* Pass */
}

#define TEST_PERSIST_MTT_NUM	2
#define NULL_PARAM		NULL

#define PERSIST_TEST_CASE_DEF(id, ord) \
	PFM_TEST_DESC_DEF(persist_test_case_desc_##id,\
	pfm_tcase_make_order(PFM_TCASE_ORD_PERSIST, ord),\
	"persist-" #id, EXPECT_SUCCESS, NULL,\
	persist_test_case_proc_##id, NULL)

PERSIST_TEST_CASE_DEF(C610383, 1);
PERSIST_TEST_CASE_DEF(C610384, 2);
PERSIST_TEST_CASE_DEF(C610385, 3);
PERSIST_TEST_CASE_DEF(C610386, 4);
PERSIST_TEST_CASE_DEF(C610387, 5);
PERSIST_TEST_CASE_DEF(C610388, 6);
PERSIST_TEST_CASE_DEF(C610389, 7);
PERSIST_TEST_CASE_DEF(C610390, 8);
PFM_MTHREAD_TEST_DESC_DEF(persist_test_case_desc_C610391,
		pfm_tcase_make_order(PFM_TCASE_ORD_PERSIST, 9),
		pfm_tcase_make_name("persist", "C610391"),
		TEST_PERSIST_MTT_NUM, EXPECT_SUCCESS, NULL, NULL_PARAM, 0,
		NULL, persist_test_case_mt_test_cb, NULL);
PERSIST_TEST_CASE_DEF(C610392, 10);
PERSIST_TEST_CASE_DEF(C610393, 11);
PERSIST_TEST_CASE_DEF(C610394, 12);
PERSIST_TEST_CASE_DEF(C610395, 13);
PERSIST_TEST_CASE_DEF(C610396, 14);
PERSIST_TEST_CASE_DEF(C610397, 15);
PERSIST_TEST_CASE_DEF(C610398, 16);
PFM_MTHREAD_TEST_DESC_DEF(persist_test_case_desc_C610400,
		pfm_tcase_make_order(PFM_TCASE_ORD_PERSIST, 18),
		pfm_tcase_make_name("persist", "C610400"),
		TEST_PERSIST_MTT_NUM, EXPECT_SUCCESS, NULL, NULL_PARAM, 0,
		NULL, persist_test_case_mt_test_cb, NULL);
PERSIST_TEST_CASE_DEF(C610401, 19);
PERSIST_TEST_CASE_DEF(C610402, 20);
PERSIST_TEST_CASE_DEF(C610403, 21);
PERSIST_TEST_CASE_DEF(C610404, 22);
PERSIST_TEST_CASE_DEF(C610405, 23);
PERSIST_TEST_CASE_DEF(C610406, 24);
