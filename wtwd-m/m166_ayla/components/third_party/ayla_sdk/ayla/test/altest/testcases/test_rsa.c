/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/base64.h>
#include <al/al_ada_thread.h>
#include <al/al_err.h>
#include <al/al_rsa.h>
#include <al/al_random.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <ayla/assert.h>

#include <platform/pfm_os_mem.h>
#include <platform/pfm_pwb_linux.h>
#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

#define RSA_KEY_SIZE	256
#define RSA_PLAIN_SIZE	(RSA_KEY_SIZE-11)
#define RSA_CYPHER_SIZE	RSA_KEY_SIZE
#define RSA_SIGN_SIZE	RSA_KEY_SIZE
#define RSA_DIGEST_SIZE	(RSA_KEY_SIZE-11)

unsigned char gab_pub_key1[RSA_KEY_SIZE*2];
unsigned char gab_pub_key2[RSA_KEY_SIZE*2];
size_t gv_pub_key1_len;
size_t gv_pub_key2_len;

/* From: test/altest-server/certs/altest.aylanetworks.com.pub.key */
static const char gabPubKey1Pem[] =
"MIIBCgKCAQEAuOcFJNtBq34DX4LGxM0NIaCTp+qC3gxcNtbRA9M1iGFu2IYs9Ldu"
"rEqSHpc5vo2UfL9HqaKmq3rnYJH6qx0P7IXqz6Bct1QSVofhrdcjldqO30OjxuWk"
"Qy/DnnGOzhAblpqPlOOB4JtG+mXUXEYlvG6rTireXi5I0yS7saLTEvx/XQQUU9bu"
"O5rhDG9B7GCHIgpbPyxAzuTkORHSjs7cit9ZfBWnjbM1NG+trtMUTqe2ZDZW/ebF"
"nFDJUyDjmuFrVTkkrzn0U0rhGIkSN6b+st8NgIxWhWsOdOLuiPfpMUwLz1tfS3G1"
"VwLewbszvmvbXg65kNR14RnwKuZw1sVs4wIDAQAB";

/* From: test/altest-server/certs/altest.aylanetworks.com.pub2.key */
static const char gabPubKey2Pem[] =
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuOcFJNtBq34DX4LGxM0N"
"IaCTp+qC3gxcNtbRA9M1iGFu2IYs9LdurEqSHpc5vo2UfL9HqaKmq3rnYJH6qx0P"
"7IXqz6Bct1QSVofhrdcjldqO30OjxuWkQy/DnnGOzhAblpqPlOOB4JtG+mXUXEYl"
"vG6rTireXi5I0yS7saLTEvx/XQQUU9buO5rhDG9B7GCHIgpbPyxAzuTkORHSjs7c"
"it9ZfBWnjbM1NG+trtMUTqe2ZDZW/ebFnFDJUyDjmuFrVTkkrzn0U0rhGIkSN6b+"
"st8NgIxWhWsOdOLuiPfpMUwLz1tfS3G1VwLewbszvmvbXg65kNR14RnwKuZw1sVs"
"4wIDAQAB";

static const char gabPlain[RSA_PLAIN_SIZE] =
"Long Live Ayla! Alya IoT from victory to victory. Hello, World!\n"
"From victory ---------------- Ayla ----------------- To Victory\n"
"\0";


static const char gabCypher[RSA_CYPHER_SIZE];


static const u8 gabSign[RSA_SIGN_SIZE] = {
	0x18, 0xcb, 0x7d, 0x71, 0x5d, 0xa7, 0x50, 0x22,
	0x7f, 0x47, 0x24, 0x04, 0x63, 0x66, 0xa5, 0x88,
	0xb0, 0x13, 0x8e, 0x78, 0xd0, 0x39, 0x3c, 0x39,
	0xc6, 0x0f, 0xd2, 0x86, 0x26, 0x27, 0x32, 0x50,
	0x33, 0x05, 0xe9, 0xdb, 0xa8, 0x70, 0x15, 0x35,
	0xf0, 0x70, 0xc7, 0xdf, 0x2d, 0x19, 0x93, 0x25,
	0xde, 0x75, 0xff, 0x84, 0x48, 0xe8, 0x0d, 0x9a,
	0xd5, 0xc3, 0x86, 0x3b, 0x92, 0x92, 0x0a, 0x18,
	0xb9, 0x1f, 0x23, 0x55, 0xb9, 0x22, 0xeb, 0x80,
	0x09, 0x32, 0x0e, 0x08, 0xc7, 0x90, 0x9c, 0xe1,
	0x47, 0x1c, 0xa9, 0xbc, 0x04, 0x6e, 0x01, 0x00,
	0xe1, 0x89, 0x25, 0xb2, 0x31, 0x0c, 0x66, 0x12,
	0x0f, 0xbd, 0x85, 0x8a, 0x2e, 0x2c, 0x59, 0x79,
	0xcf, 0x03, 0xd5, 0x36, 0xe5, 0x58, 0x8a, 0xed,
	0xb4, 0xeb, 0x65, 0x4e, 0x96, 0x4b, 0x76, 0xc9,
	0xb0, 0x5f, 0x68, 0x75, 0x02, 0xd1, 0xa9, 0xf2,
	0x26, 0x38, 0x1d, 0xc7, 0x08, 0xf0, 0x98, 0x94,
	0xc7, 0x87, 0xc0, 0x26, 0xef, 0x14, 0x78, 0xff,
	0x81, 0xc5, 0xbb, 0x48, 0xd7, 0xed, 0xfa, 0x2b,
	0xbe, 0xf1, 0x15, 0x95, 0xbf, 0x21, 0x68, 0x5d,
	0x1f, 0xfd, 0xb1, 0x3c, 0xdc, 0xe0, 0xe3, 0x38,
	0xb9, 0xfd, 0x1c, 0x67, 0x66, 0x34, 0xb2, 0x19,
	0x45, 0xe7, 0xb1, 0xec, 0x4d, 0x38, 0x6c, 0x1e,
	0xf4, 0x5b, 0xc8, 0xdc, 0xc1, 0x74, 0xbe, 0xdb,
	0x27, 0x49, 0x1f, 0x7f, 0x73, 0x24, 0xa1, 0x6d,
	0x39, 0x02, 0x49, 0x9e, 0x87, 0x55, 0x1d, 0x2c,
	0x6c, 0x11, 0x83, 0x84, 0x3e, 0x5d, 0x8a, 0x2f,
	0x89, 0x3c, 0xa1, 0x13, 0x81, 0xbd, 0x3e, 0x5e,
	0xa2, 0x56, 0x8f, 0x61, 0x3b, 0x7f, 0x8b, 0x4e,
	0x4d, 0x1f, 0x8a, 0x4f, 0x05, 0x28, 0x0c, 0xd2,
	0xf7, 0x56, 0xe6, 0xaa, 0x8e, 0x39, 0x60, 0xd6,
	0x41, 0xb8, 0xe4, 0x81, 0x3c, 0xca, 0xa4, 0xd8,
};

static const u8 gabDigest[RSA_DIGEST_SIZE] = {
	0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
	0x41, 0x79, 0x6c, 0x61, 0x20, 0x63, 0x65, 0x72,
	0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65,
	0x20, 0x64, 0x69, 0x67, 0x65, 0x73, 0x74, 0x20,
	0x66, 0x6f, 0x72, 0x20, 0x61, 0x6c, 0x74, 0x65,
	0x73, 0x74, 0x2e, 0x20, 0x48, 0x65, 0x6c, 0x6c,
	0x6f, 0x2c, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64,
	0x20, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x0a,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x30, 0x20, 0x61, 0x62, 0x63, 0x64, 0x65,
	0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d,
	0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
	0x76, 0x77, 0x78, 0x79, 0x7a, 0x41, 0x42, 0x43,
	0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
	0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
	0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x0a,
	0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
	0x41, 0x79, 0x6c, 0x61, 0x20, 0x63, 0x65, 0x72,
	0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65,
	0x20, 0x64, 0x69, 0x67, 0x65, 0x73, 0x74, 0x20,
	0x66, 0x6f, 0x72, 0x20, 0x61, 0x6c, 0x74, 0x65,
	0x73, 0x74, 0x2e, 0x20, 0x48, 0x65, 0x6c, 0x6c,
	0x6f, 0x21, 0x21, 0x21, 0x0a
};

static void rsa_test_prpare_rsa_keys(void)
{
	static u8 gb_rsa_key_inited;
	int rc;
	if (!gb_rsa_key_inited) {
		gb_rsa_key_inited = 1;
		gv_pub_key1_len = sizeof(gab_pub_key1);
		rc = ayla_base64_decode(gabPubKey1Pem, strlen(gabPubKey1Pem),
			gab_pub_key1, &gv_pub_key1_len);
		ASSERT(rc == 0);
		gv_pub_key2_len = sizeof(gab_pub_key2);
		rc = ayla_base64_decode(gabPubKey2Pem, strlen(gabPubKey2Pem),
			gab_pub_key2, &gv_pub_key2_len);
		ASSERT(rc == 0);
	}
}

#define hex(c)	(c < 10) ? ('0' + c) : ('A' + c - 10)

#undef  N
#define N	16
static void test_print_hex(const void *pb_data, ssize_t size)
{
	const u8 *ps = pb_data;
	char buf[N*2+1];
	char *pt;
	int n;
	int i;
	int c;
	while (size > 0) {
		n = (size >= N) ? N : size;
		pt = buf;
		for (i = 0; i < n; i++, ps++) {
			c = ps[0]>>4;
			*pt++ = hex(c);
			c = ps[0] & 0x0F;
			*pt++ = hex(c);
		}
		*pt = '\0';
		test_printf("[%s]", buf);
		size -= n;
	}
}

/*
C610411	test al_rsa_ctxt_alloc and al_ctxt_free in single thread
1.no assert
2.al_rsa_ctxt_alloc return non-null pointer
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610411(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	struct al_rsa_ctxt *ctx;

	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	al_rsa_ctxt_free(ctx);
	return NULL;
}

/*
C610413	test al_ctxt_free when ctxt is null
1.no assert
*/
static char *rsa_test_case_proc_C610413(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;

	rc = pfm_try_catch_assert();
	if (rc == 0) {
		al_rsa_ctxt_free(NULL);
		err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610413: al_rsa_ctxt_free() when ctxt is null"
		"should be success";
}

/*
C610414	test al_rsa_pub_key_set with normal parameters
1.no assert
2.al_rsa_ctxt_alloc return non-null pointer
3.al_rsa_pub_key_set return key length
	key={0x0,0x1,0x2,...0xff}
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610414(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t len;
	struct al_rsa_ctxt *ctx;

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		len = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
		err = (len == RSA_KEY_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610414: test al_rsa_pub_key_set() with normal parameters"
		" should be success";
}

/*
C610415	test al_rsa_pub_key_set when ctxt is null
1.assert	"ctxt=null
	1.call al_rsa_pub_key_set directly
*/
static char *rsa_test_case_proc_C610415(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;

	rsa_test_prpare_rsa_keys();
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		key_size = al_rsa_pub_key_set(NULL, gab_pub_key1,
			gv_pub_key1_len);
		err = (key_size == RSA_KEY_SIZE) ?
			AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610415: al_rsa_pub_key_set() when ctxt is null"
		" should be failure";
}

/*
C610416	test al_rsa_pub_key_set when key is null
1.assert	key=null
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610416(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	struct al_rsa_ctxt *ctx;

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		key_size = al_rsa_pub_key_set(ctx, NULL, gv_pub_key1_len);
		err = (key_size == RSA_KEY_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610416: al_rsa_pub_key_set() when key is null"
		"should be failure";
}

/*
C610417	test al_rsa_pub_key_set when keylen is 0
1.assert
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610417(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	struct al_rsa_ctxt *ctx;

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, 0);
		err = (key_size == RSA_KEY_SIZE) ?
			AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610417: al_rsa_pub_key_set() when keylen is 0"
		"should be failure";
}

/*
C610418	test al_rsa_pub_key_set when keylen is 1MB
1.no assert
2.al_rsa_ctxt_alloc return non-null pointer
3.al_rsa_pub_key_set return key length
	key={0x0,0x1,0x2,...0xff...}
	keylen=1MB"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610418(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	size_t key_size;
	struct al_rsa_ctxt *ctx;
	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, 1024*1024);
	al_rsa_ctxt_free(ctx);
	ASSERT(key_size == RSA_KEY_SIZE);
	return NULL;
}

/*
C610419	test al_rsa_key_clear with normal parameters
1.no assert
2.al_rsa_ctxt_alloc return non-null pointer
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_key_clear
	3.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610419(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	struct al_rsa_ctxt *ctx;
	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	al_rsa_key_clear(ctx);
	al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	al_rsa_key_clear(ctx);
	al_rsa_ctxt_free(ctx);
	return NULL;
}

/*
C610420	test al_rsa_key_clear when ctxt is null
1.assert	ctxt=null
	1.call al_rsa_key_clear directly
*/
static char *rsa_test_case_proc_C610420(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;

	rc = pfm_try_catch_assert();
	if (rc == 0) {
		al_rsa_key_clear(NULL);
		err = AL_ERR_OK;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* Pass */
	}
	return "C610420: al_rsa_key_clear() when ctxt is null"
		"should be failure";
}

/*
C610421	test al_rsa_encrypt_pub with normal parameters
1.no assert
	1.call al_rsa_ctxt_alloc, return non-null pointer
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
#undef  RSA_USE_RANDOM_DATA

static char *rsa_test_case_proc_C610421(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	ssize_t size;
	size_t key_size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[RSA_CYPHER_SIZE];
	u8 abPlain[RSA_PLAIN_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);

	test_printf("C610421 RSA PubKey1 (len=%d):", strlen(gabPubKey1Pem));
	test_printf("%s\n", gabPubKey1Pem);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);

#ifdef RSA_USE_RANDOM_DATA
	size = RSA_PLAIN_SIZE;
	al_random_fill(abPlain, size);
	test_printf("C610421 RSA Plain1 (len=%d):", size);
	log_bytes_in_hex(LOG_MOD_DEFAULT, (void *)abPlain, size);
#else
	size = strlen(gabPlain) + 1;
	memcpy(abPlain, gabPlain, size);
	test_printf("C610421 RSA Plain1 (len=%d):\n%s", size, gabPlain);
#endif
	size = al_rsa_encrypt_pub(ctx, abPlain, size,
		abCypher, sizeof(abCypher));
	ASSERT(size == RSA_CYPHER_SIZE);
	test_printf("C610421 RSA Cypher1 Start (len=%d):", size);
	test_print_hex(abCypher, size);
	test_printf("C610421 RSA Cypher1 End");
	al_rsa_key_clear(ctx);

	test_printf("C610421 RSA PubKey2 (len=%d):", strlen(gabPubKey2Pem));
	test_printf("%s\n", gabPubKey2Pem);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key2, gv_pub_key2_len);
	ASSERT(key_size == RSA_KEY_SIZE);

#ifdef RSA_USE_RANDOM_DATA
	size = RSA_PLAIN_SIZE;
	al_random_fill(abPlain, size);
	test_printf("C610421 RSA Plain2 (len=%d):", size);
	log_bytes_in_hex(LOG_MOD_DEFAULT, (void *)abPlain, size);
#else
	size = strlen(gabPlain) + 1;
	memcpy(abPlain, gabPlain, size);
	test_printf("C610421 RSA Plain2 (len=%d):\n%s", size, gabPlain);
#endif
	size = al_rsa_encrypt_pub(ctx, abPlain, size,
		abCypher, sizeof(abCypher));
	ASSERT(size == RSA_CYPHER_SIZE);
	test_printf("C610421 RSA Cypher2 Start (len=%d):", size);
	test_print_hex(abCypher, size);
	test_printf("C610421 RSA Cypher2 End");

	al_rsa_ctxt_free(ctx);
	return NULL;
}

/*
C610422	test al_rsa_encrypt_pub when ctxt is null
1.assert	ctxt=null
	1.call al_rsa_encrypt_pub directly
*/
static char *rsa_test_case_proc_C610422(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	ssize_t size;
	u8 abCypher[RSA_CYPHER_SIZE];

	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_encrypt_pub(NULL, gabPlain, RSA_PLAIN_SIZE,
			abCypher, sizeof(abCypher));
		err = (size == RSA_CYPHER_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();

	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610422: al_rsa_encrypt_pub() when ctxt is null"
		" should be failure";
}

/*
C610423	test al_rsa_encrypt_pub when in is null
1.assert	in=null
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610423(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[RSA_CYPHER_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_encrypt_pub(ctx, NULL, RSA_PLAIN_SIZE,
			abCypher, sizeof(abCypher));
		err = (size == RSA_CYPHER_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610423: al_rsa_encrypt_pub() when in is null"
		" should be failure";
}

/*
C610424	test al_rsa_encrypt_pub when in_len is 0
1.assert	"in={0x0,0x1,0x2,...0xff} in_len=0
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610424(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[RSA_CYPHER_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_encrypt_pub(ctx, gabPlain, 0,
			abCypher, sizeof(abCypher));
		err = (size == RSA_CYPHER_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610424: al_rsa_encrypt_pub() when in_len is 0"
		" should be failure";
}

/*
C610425	test al_rsa_encrypt_pub when in_len > (RSA_key_size_in_bytes - 11)
1.assert
2.al_rsa_ctxt_alloc return non-null pointer
3.al_rsa_encrypt_pub return output length
4.the output data match the given data"	"in={0x0,0x1,0x2,...0xff} in_len=1MB"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610425(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t  key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abPlain[RSA_KEY_SIZE*2];
	u8 abCypher[RSA_KEY_SIZE*3];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		ASSERT(key_size == RSA_KEY_SIZE);
		al_random_fill(abPlain, sizeof(abPlain));
		size = al_rsa_encrypt_pub(ctx, abPlain, sizeof(abPlain),
			abCypher, sizeof(abCypher));
		err = (size > 0) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610425: al_rsa_encrypt_pub() when in_len > (RSA_key_size-11)"
		" should be failure";
}

/*
C610426	test al_rsa_encrypt_pub when out is null
1.no assert	"in={0x0,0x1,0x2,...0xff} out=null"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	4.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610426(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[RSA_CYPHER_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_encrypt_pub(ctx, gabPlain, RSA_PLAIN_SIZE,
			NULL, sizeof(abCypher));
		/* In this case, it returns the output buffer size required */
		err = (size == RSA_CYPHER_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610426: al_rsa_encrypt_pub() when out is null"
		" should be failure";
}

/*
C610427	test al_rsa_encrypt_pub when out_len is 0
1.assert	"in={0x0,0x1,0x2,...0xff} out_len=0"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610427(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[RSA_CYPHER_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_encrypt_pub(ctx, gabPlain, RSA_PLAIN_SIZE,
			abCypher, 0);
		err = (size == RSA_CYPHER_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610427: al_rsa_encrypt_pub() when out_len is 0"
		" should be failure";
}

/*
C610428	test al_rsa_encrypt_pub when out_len >= RSA_key_size
1.no assert
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610428(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t  key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[1024];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		ASSERT(key_size == RSA_KEY_SIZE);
		size = al_rsa_encrypt_pub(ctx, gabPlain, RSA_PLAIN_SIZE,
			abCypher, 1024);
		err = (size > 0) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610428: al_rsa_encrypt_pub() when out_len is >= RSA key size"
		" should be success";

}

/*
C610429	test al_rsa_encrypt_pub when out_len is 1
1.assert
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_encrypt_pub
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610429(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[RSA_CYPHER_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_encrypt_pub(ctx, gabPlain, RSA_PLAIN_SIZE,
			abCypher, 1);
		err = (size == RSA_CYPHER_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610429: al_rsa_encrypt_pub() when out_len is 1"
		" should be failure";
}

/*
C610430	test al_rsa_encrypt_pub without calling al_rsa_pub_key_set
1.assert	in={0x0,0x1,0x2,...0xff}
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_encrypt_pub
	3.compare the output data with the given data
	4.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610430(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abCypher[RSA_CYPHER_SIZE];

	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_encrypt_pub(ctx, gabPlain, RSA_PLAIN_SIZE,
			abCypher, sizeof(abCypher));
		err = (size == RSA_CYPHER_SIZE) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610430: al_rsa_encrypt_pub() without calling "
		"al_rsa_pub_key_se() should be failure";
}

/*
C610431	test al_rsa_verify with normal parameters
1.no assert
2.al_rsa_ctxt_alloc return non-null pointer
3.al_rsa_verify return output length
4.the output data match the given data"	in={0x0,0x1,0x2,...0xff}
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610431(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_DIGEST_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key2, gv_pub_key2_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, sizeof(gabSign),
			abDigest, sizeof(abDigest));
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else if (memcmp(abDigest, gabDigest, size)) {
			log_bytes_in_hex(LOG_MOD_DEFAULT, abDigest, size);
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610431: al_rsa_verify() with normal parameters"
		" should be success";
}

/*
C610432	test al_rsa_verify when ctxt is null
1.assert	ctxt=null
	1.call al_rsa_verify directly
*/
static char *rsa_test_case_proc_C610432(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	ssize_t size;
	u8 abDigest[RSA_DIGEST_SIZE];

	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(NULL, gabSign, sizeof(gabSign),
			abDigest, sizeof(abDigest));
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else if (memcmp(abDigest, gabDigest, size)) {
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	/* Expect faulure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610432: al_rsa_verify() when ctxt is null"
		" should be faulure";
}

/*
C610433	test al_rsa_verify when in is null
1.assert	in=null
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610433(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	ssize_t size;
	size_t key_size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_DIGEST_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, NULL, sizeof(gabSign),
			abDigest, sizeof(abDigest));
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else if (memcmp(abDigest, gabDigest, size)) {
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610433: al_rsa_verify() when in is null"
		" should be failure";
}

/*
C610434	test al_rsa_verify when in_len is 0
1.assert	"in={0x0,0x1,0x2,...0xff} in_len=0"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610434(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_DIGEST_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, 0,
			abDigest, sizeof(abDigest));
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else if (memcmp(abDigest, gabDigest, size)) {
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610434: al_rsa_verify() when in_len is 0"
		" should be failure";
}

/*
C610435	test al_rsa_verify when in_len != RSA key size.
1.assert
2.al_rsa_ctxt_alloc return non-null pointer
3.al_rsa_verify return output length
4.the output data match the given data"	"in={0x0,0x1,0x2,...0xff} in_len=1MB"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610435(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_KEY_SIZE * 3];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, RSA_KEY_SIZE * 2,
			abDigest, sizeof(abDigest));
		err = (size > 0) ? AL_ERR_OK : AL_ERR_ERR;
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610435: al_rsa_verify() when in_len is != RSA key size"
		" should be failure";
}

/*
C610436	test al_rsa_verify when out is null
1.no assert	"in={0x0,0x1,0x2,...0xff} out=null"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610436(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_DIGEST_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, sizeof(gabSign),
			NULL, sizeof(abDigest));
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610436: al_rsa_verify() when out is null"
		" should be success";
}

/*
C610437	test al_rsa_verify when out_len is 0
1.assert	"in={0x0,0x1,0x2,...0xff} out_len=0
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610437(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_DIGEST_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, sizeof(gabSign),
			abDigest, 0);
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else if (memcmp(abDigest, gabDigest, size)) {
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610437: al_rsa_verify() when out_len is 0"
		" should be failure";
}

/*
C610438	test al_rsa_verify when out_len is 1MB
1.no assert
2.al_rsa_ctxt_alloc return non-null pointer
3.al_rsa_verify return output length
4.the output data match the given data	in={0x0,0x1,0x2,...0xff} out_len=1MB
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610438(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_DIGEST_SIZE];

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, sizeof(gabSign),
			abDigest, 1024*1024);
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else if (memcmp(abDigest, gabDigest, size)) {
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect success */
	if (err == AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610427: al_rsa_verify() when out_len is 1MB"
		" should be success";
}

/*
C610439	test al_rsa_verify when out_len is 1
1.assert
2.al_rsa_ctxt_alloc return non-null pointer
3.al_rsa_verify return output length
4.the output data match the given data"	"in={0x0,0x1,0x2,...0xff} out_len=1"
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_pub_key_set
	3.call al_rsa_verify
	4.compare the output data with the given data
	5.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610439(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	size_t key_size;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[1]; /* RSA_DIGEST_SIZE */

	rsa_test_prpare_rsa_keys();
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, sizeof(gabSign),
			abDigest, 1);
		ASSERT(size >= RSA_DIGEST_SIZE);
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610439: al_rsa_verify() when out_len is 1"
		" should be failure";
}

/*
C610440	test al_rsa_verify without calling al_rsa_pub_key_set
1.assert	in={0x0,0x1,0x2,...0xff}
	1.call al_rsa_ctxt_alloc
	2.call al_rsa_verify
	3.compare the output data with the given data
	4.call al_rsa_ctxt_free
*/
static char *rsa_test_case_proc_C610440(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int rc;
	enum al_err err;
	ssize_t size;
	struct al_rsa_ctxt *ctx;
	u8 abDigest[RSA_DIGEST_SIZE];

/*	rsa_test_prpare_rsa_keys();
*/
	ctx = al_rsa_ctxt_alloc();
	ASSERT(ctx);
/*
	key_size = al_rsa_pub_key_set(ctx, gab_pub_key1, gv_pub_key1_len);
	ASSERT(key_size == RSA_KEY_SIZE);
*/
	rc = pfm_try_catch_assert();
	if (rc == 0) {
		size = al_rsa_verify(ctx, gabSign, sizeof(gabSign),
			abDigest, sizeof(abDigest));
		if (size != RSA_DIGEST_SIZE) {
			err = AL_ERR_ERR;
		} else if (memcmp(abDigest, gabDigest, size)) {
			err = AL_ERR_ERR;
		} else {
			err = AL_ERR_OK;
		}
	} else {
		err = AL_ERR_ERR;
	}
	pfm_try_catch_final();
	al_rsa_ctxt_free(ctx);
	/* Expect failure */
	if (err != AL_ERR_OK) {
		return NULL; /* pass */
	}
	return "C610440: al_rsa_verify() without calling al_rsa_pub_key_set()"
		" should be failure";
}

#define RSA_TEST_CASE_DEF(id, ord) \
	PFM_TEST_DESC_DEF(rsa_test_case_desc_##id,\
	pfm_tcase_make_order(PFM_TCASE_ORD_RSA, ord),\
	"rsa-" #id, EXPECT_SUCCESS, NULL,\
	rsa_test_case_proc_##id, NULL)

RSA_TEST_CASE_DEF(C610411, 1);
RSA_TEST_CASE_DEF(C610413, 3);
RSA_TEST_CASE_DEF(C610414, 4);
RSA_TEST_CASE_DEF(C610415, 5);
RSA_TEST_CASE_DEF(C610416, 6);
RSA_TEST_CASE_DEF(C610417, 7);
RSA_TEST_CASE_DEF(C610418, 8);
RSA_TEST_CASE_DEF(C610419, 9);
RSA_TEST_CASE_DEF(C610420, 10);
PFM_TEST_DESC_DEF(rsa_test_case_desc_C610421, PFM_TCASE_ORD_MANUAL,
    "rsa-C610421", EXPECT_SUCCESS, NULL, rsa_test_case_proc_C610421,
    NULL);
RSA_TEST_CASE_DEF(C610422, 12);
RSA_TEST_CASE_DEF(C610423, 13);
RSA_TEST_CASE_DEF(C610424, 14);
RSA_TEST_CASE_DEF(C610425, 15);
RSA_TEST_CASE_DEF(C610426, 16);
RSA_TEST_CASE_DEF(C610427, 17);
RSA_TEST_CASE_DEF(C610428, 18);
RSA_TEST_CASE_DEF(C610429, 19);
RSA_TEST_CASE_DEF(C610430, 20);
RSA_TEST_CASE_DEF(C610431, 21);
RSA_TEST_CASE_DEF(C610432, 22);
RSA_TEST_CASE_DEF(C610433, 23);
RSA_TEST_CASE_DEF(C610434, 24);
RSA_TEST_CASE_DEF(C610435, 25);
RSA_TEST_CASE_DEF(C610436, 26);
RSA_TEST_CASE_DEF(C610437, 27);
RSA_TEST_CASE_DEF(C610438, 28);
RSA_TEST_CASE_DEF(C610439, 29);
RSA_TEST_CASE_DEF(C610440, 30);

