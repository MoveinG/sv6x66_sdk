/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_aes.h>
#include <al/al_random.h>
#include <al/al_ada_thread.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

/* tca = Test Case Arguments */
struct tca_aes_arg {
	/* testing parameters */
	struct al_aes_ctxt *ctxt;
	char *key;
	size_t key_len;
	void *iv;
	int decrypt;
	void *in;
	void *out;
	void *given;
	size_t in_len;

	/* running parameters */
	int exp_match;
};

/* TCDP = Test Case Default Parameter */
/* base command to get the test data as above:
 *
 * 1.AES encode :
 * echo "helloworld" | openssl enc -aes-128-cbc -a
 * -iv "61796c6161796c6161796c6161796c61"
 * -K "6c6f766561796c616c6f766561796c61"
 * return the encrypt data format base64.
 *
 * 2.transform the data from base64 to the ascii:
 * echo "4gzXK7dE260oy/HV5G/XTA==" | base64 -d | hexdump -C
 *
 * 3.AES decode :
 * echo "4gzXK7dE260oy/HV5G/XTA==" | openssl enc -d -aes-128-cbc -a
 * -iv "61796c6161796c6161796c6161796c61"
 * -K "6c6f766561796c616c6f766561796c61"
 *
 * NOTICE: If the context length is not an integral multiple of block size,
 * openssl complemente the context with 0x05.
 * */
#define TEST_AES_BLOCK_SIZE	(16)
static char tcdp_aes_key[TEST_AES_BLOCK_SIZE] = {
		0x6c, 0x6f, 0x76, 0x65, 0x61, 0x79, 0x6c, 0x61,
		0x6c, 0x6f, 0x76, 0x65, 0x61, 0x79, 0x6c, 0x61
};
static char tcdp_aes_iv[TEST_AES_BLOCK_SIZE] = "aylaaylaaylaayla";
static char tcdp_aes_enc_context[TEST_AES_BLOCK_SIZE] = {
		0x68, 0x65, 0x6c, 0x6c,
		0x6f, 0x77, 0x6f, 0x72,
		0x6c, 0x64, 0x0a,
		/* openssl complemente the context with 0x05 */
		0x05, 0x05, 0x05, 0x05,
		0x05};
/* "4gzXK7dE260oy/HV5G/XTA==" in base64 */
static char tcdp_aes_dec_context[TEST_AES_BLOCK_SIZE] = {
			0xe2, 0x0c, 0xd7, 0x2b,
			0xb7, 0x44, 0xdb, 0xad,
			0x28, 0xcb, 0xf1, 0xd5,
			0xe4, 0x6f, 0xd7, 0x4c};
/* random field */
static char tcdp_aes_random[TEST_AES_BLOCK_SIZE];
static char tcdp_aes_tmp[TEST_AES_BLOCK_SIZE];

PFM_COMBI_PARAM_VALS(invalid_ptr, void *, 1, NULL);
PFM_COMBI_PARAM_VALS(invalid_size, size_t, 2, 0, 1<<20);

static const struct tca_aes_arg aes_encrypt_arg_dft = {
	.key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
	.iv = tcdp_aes_iv,
	.in = tcdp_aes_enc_context,
	.out = tcdp_aes_tmp,
	.given = tcdp_aes_dec_context, .exp_match = 1,
	.in_len = TEST_AES_BLOCK_SIZE, .decrypt = 0,
};

static const struct tca_aes_arg aes_decrypt_arg_dft = {
	.key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
	.iv = tcdp_aes_iv,
	.in = tcdp_aes_dec_context,
	.out = tcdp_aes_tmp,
	.given = tcdp_aes_enc_context, .exp_match = 1,
	.in_len = TEST_AES_BLOCK_SIZE, .decrypt = 1,
};

/* alist = arguments' list */
static char *testcases_aes_alloc_free(const struct pfm_test_desc *pcase,
		int argc, char **argv);
static char *testcases_aes_free_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_crypto_with_givenkey(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_crypto_with_random(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_crypto_with_defferent(const struct pfm_test_desc
		*pcase, int argc, char **argv);

static char *testcases_aes_key_set_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_iv_get_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_encrypt_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_decrypt_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv);

static char *testcases_aes_iv_get_before_set(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_iv_get_after_set(const struct pfm_test_desc
		*pcase, int argc, char **argv);
static char *testcases_aes_iv_get_after_crypto(const struct pfm_test_desc
		*pcase, int argc, char **argv);

static char *testcases_aes_crypto_set_wrong_decrypt(const struct pfm_test_desc
		*pcase, int argc, char **argv);

static struct tca_aes_arg aes_alist[] = {
		/* 0 */
		{0},
		/* 1 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv,
			.in = tcdp_aes_enc_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_dec_context, .exp_match = 1,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 2 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv, .decrypt = 1,
			.in = tcdp_aes_dec_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_enc_context, .exp_match = 1,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 3 */
		{ .key = tcdp_aes_random, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv, .decrypt = 1,
			.in_len = TEST_AES_BLOCK_SIZE, .exp_match = 1},
		/* 4 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_random, .decrypt = 1,
			.in_len = TEST_AES_BLOCK_SIZE, .exp_match = 1},
		/* 5 */
		{ .key = tcdp_aes_random, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv, .decrypt = 1,
			.in = tcdp_aes_dec_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_enc_context,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 6 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_random, .decrypt = 1,
			.in = tcdp_aes_dec_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_enc_context,
			.in_len = TEST_AES_BLOCK_SIZE},

		/* 7 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv},
		/* 8 */
		{ .out = tcdp_aes_tmp, .in_len = TEST_AES_BLOCK_SIZE},
		/* 9 */
		{ .out = tcdp_aes_tmp, .in_len = TEST_AES_BLOCK_SIZE},
		/* 10 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv, .exp_match = 1,
			.out = tcdp_aes_tmp, .in_len = TEST_AES_BLOCK_SIZE},
		/* 11 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv,
			.in = tcdp_aes_enc_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_dec_context, .exp_match = 1,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 12 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv, .decrypt = 1,
			.in = tcdp_aes_dec_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_enc_context, .exp_match = 1,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 13 */
		{ .in = tcdp_aes_enc_context, .out = tcdp_aes_tmp,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 14 */
		{ .in = tcdp_aes_dec_context, .out = tcdp_aes_tmp,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 15 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv,
			.in = tcdp_aes_enc_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_dec_context,
			.in_len = TEST_AES_BLOCK_SIZE},
		/* 16 */
		{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
			.iv = tcdp_aes_iv, .decrypt = 1,
			.in = tcdp_aes_dec_context,
			.out = tcdp_aes_tmp,
			.given = tcdp_aes_enc_context,
			.in_len = TEST_AES_BLOCK_SIZE},
};

static int testcases_aes_crypto_block(struct al_aes_ctxt *ctxt, void *key,
		size_t key_len, void *iv, void *in, size_t in_len, void *out,
		int decrypt)
{
	int rc;

	rc = al_aes_cbc_key_set(ctxt, key, key_len,
	    iv, decrypt);
	if (0 != rc) {
		return rc;
	}
	if (0 == decrypt) {
		rc = al_aes_cbc_encrypt(ctxt, in, out, in_len);
		if (0 != rc) {
			return rc;
		}
	} else {
		rc = al_aes_cbc_decrypt(ctxt, in, out, in_len);
		if (0 != rc) {
			return rc;
		}
	}
	return rc;
}

static char *testcases_aes_alloc_free(const struct pfm_test_desc *pcase,
		int argc, char **argv)
{
	struct al_aes_ctxt *ctxt;

	ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != ctxt);
	al_aes_ctxt_free(ctxt);
	return NULL;
}

static char *testcases_aes_free_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

static char *testcases_aes_crypto_with_givenkey(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	size_t out_len;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);
	aes_arg->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != aes_arg->ctxt);

	out_len = ((aes_arg->in_len)>>4)<<4;
	if ((aes_arg->in_len) > out_len) {
		out_len += 1<<4;
	}
	memset(aes_arg->out, 0x0, out_len);

	rc = testcases_aes_crypto_block(aes_arg->ctxt, aes_arg->key,
	    aes_arg->key_len, aes_arg->iv, aes_arg->in, aes_arg->in_len,
	    aes_arg->out, aes_arg->decrypt);
	ASSERT(0 == rc);

	rc = memcmp(aes_arg->out, aes_arg->given, out_len);
	ASSERT((0 == rc && 0 != aes_arg->exp_match)
	    || (0 != rc && 0 == aes_arg->exp_match));

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

static char *testcases_aes_crypto_with_random(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	size_t out_len;
	char aes_in[TEST_AES_BLOCK_SIZE];
	char aes_tmp[TEST_AES_BLOCK_SIZE];
	char aes_out[TEST_AES_BLOCK_SIZE];
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);
	aes_arg->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != aes_arg->ctxt);

	out_len = ((aes_arg->in_len)>>4)<<4;
	if ((aes_arg->in_len) > out_len) {
		out_len += 1<<4;
	}
	al_random_fill(aes_in, sizeof(aes_in));
	memset(aes_tmp, 0x0, sizeof(aes_tmp));
	memset(aes_out, 0x0, sizeof(aes_out));
	al_random_fill(tcdp_aes_random, TEST_AES_BLOCK_SIZE);

	rc = testcases_aes_crypto_block(aes_arg->ctxt, aes_arg->key,
	    aes_arg->key_len, aes_arg->iv, aes_in, sizeof(aes_in),
	    aes_tmp, 0);
	ASSERT(0 == rc);

	rc = testcases_aes_crypto_block(aes_arg->ctxt, aes_arg->key,
	    aes_arg->key_len, aes_arg->iv, aes_tmp, sizeof(aes_tmp),
	    aes_out, 1);
	ASSERT(0 == rc);

	rc = memcmp(aes_out, aes_in, out_len);
	ASSERT((0 == rc && 0 != aes_arg->exp_match)
	    || (0 != rc && 0 == aes_arg->exp_match));

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

static char *testcases_aes_crypto_with_defferent(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	size_t out_len;
	char aes_tmp[TEST_AES_BLOCK_SIZE];
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);
	aes_arg->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != aes_arg->ctxt);

	out_len = ((aes_arg->in_len)>>4)<<4;
	if ((aes_arg->in_len) > out_len) {
		out_len += 1<<4;
	}
	memset(aes_arg->out, 0x0, out_len);
	memset(aes_tmp, 0x0, sizeof(aes_tmp));

	al_random_fill(tcdp_aes_random, TEST_AES_BLOCK_SIZE);
	rc = testcases_aes_crypto_block(aes_arg->ctxt, aes_arg->key,
	    aes_arg->key_len, aes_arg->iv, aes_arg->in, aes_arg->in_len,
	    aes_tmp, 0);
	ASSERT(0 == rc);

	al_random_fill(tcdp_aes_random, TEST_AES_BLOCK_SIZE);
	rc = testcases_aes_crypto_block(aes_arg->ctxt, aes_arg->key,
	    aes_arg->key_len, aes_arg->iv, aes_tmp, sizeof(aes_tmp),
	    aes_arg->out, 1);
	ASSERT(0 == rc);

	rc = memcmp(aes_arg->out, aes_arg->given, out_len);
	ASSERT((0 == rc && 0 != aes_arg->exp_match)
	    || (0 != rc && 0 == aes_arg->exp_match));

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

static char *testcases_aes_key_set_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);

	rc = al_aes_cbc_key_set(aes_arg->ctxt, aes_arg->key, aes_arg->key_len,
	    aes_arg->iv, aes_arg->decrypt);
	ASSERT(0 == rc);
	return NULL;
}

static char *testcases_aes_iv_get_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);

	al_aes_iv_get(aes_arg->ctxt, aes_arg->out, aes_arg->in_len);
	return NULL;
}

static char *testcases_aes_encrypt_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);

	rc = al_aes_cbc_encrypt(aes_arg->ctxt, aes_arg->in, aes_arg->out,
	    aes_arg->in_len);
	ASSERT(0 == rc);
	return NULL;
}

static char *testcases_aes_decrypt_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);

	rc = al_aes_cbc_decrypt(aes_arg->ctxt, aes_arg->in, aes_arg->out,
	    aes_arg->in_len);
	ASSERT(0 == rc);
	return NULL;
}

static char *testcases_aes_iv_get_before_set(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);
	aes_arg->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != aes_arg->ctxt);

	rc = al_aes_iv_get(aes_arg->ctxt, aes_arg->out, aes_arg->in_len);
	ASSERT(0 == rc);

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

static char *testcases_aes_iv_get_after_set(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);
	aes_arg->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != aes_arg->ctxt);

	rc = al_aes_cbc_key_set(aes_arg->ctxt, aes_arg->key, aes_arg->key_len,
	    aes_arg->iv, aes_arg->decrypt);
	ASSERT(0 == rc);

	al_aes_iv_get(aes_arg->ctxt, aes_arg->out, aes_arg->in_len);

	rc = memcmp(aes_arg->out, aes_arg->iv, aes_arg->in_len);
	ASSERT((0 == rc && 0 != aes_arg->exp_match)
	    || (0 != rc && 0 == aes_arg->exp_match));

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

static char *testcases_aes_iv_get_after_crypto(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	int i;
	char *p;
	size_t out_len;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);
	aes_arg->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != aes_arg->ctxt);

	out_len = ((aes_arg->in_len)>>4)<<4;
	if ((aes_arg->in_len) > out_len) {
		out_len += 1<<4;
	}

	rc = testcases_aes_crypto_block(aes_arg->ctxt, aes_arg->key,
	    aes_arg->key_len, aes_arg->iv, aes_arg->in, aes_arg->in_len,
	    aes_arg->out, aes_arg->decrypt);
	ASSERT(0 == rc);

	rc = memcmp(aes_arg->out, aes_arg->given, out_len);
	ASSERT((0 == rc && 0 != aes_arg->exp_match)
	    || (0 != rc && 0 == aes_arg->exp_match));

	memset(aes_arg->out, 0x0, aes_arg->in_len);
	al_aes_iv_get(aes_arg->ctxt, aes_arg->out, aes_arg->in_len);

	rc = -1;
	for (i = 0, p = (char *)aes_arg->out; i < aes_arg->in_len; i++) {
		if ('\0' != *p) {
			rc = 0;
			break;
		}
	}
	ASSERT(0 == rc);

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

static char *testcases_aes_crypto_set_wrong_decrypt(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	size_t out_len;
	struct tca_aes_arg *aes_arg;

	aes_arg = (struct tca_aes_arg *)(pcase->arg);
	ASSERT(NULL != aes_arg);
	aes_arg->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != aes_arg->ctxt);

	out_len = ((aes_arg->in_len)>>4)<<4;
	if ((aes_arg->in_len) > out_len) {
		out_len += 1<<4;
	}
	memset(aes_arg->out, 0x0, out_len);

	rc = al_aes_cbc_key_set(aes_arg->ctxt, aes_arg->key, aes_arg->key_len,
	    aes_arg->iv, aes_arg->decrypt);
	ASSERT(0 == rc);
	if (0 == aes_arg->decrypt) {
		rc = al_aes_cbc_decrypt(aes_arg->ctxt, aes_arg->in,
		    aes_arg->out, aes_arg->in_len);
	} else {
		rc = al_aes_cbc_encrypt(aes_arg->ctxt, aes_arg->in,
		    aes_arg->out, aes_arg->in_len);
	}
	ASSERT(0 == rc);

	al_aes_ctxt_free(aes_arg->ctxt);
	return NULL;
}

/*
C610096 test al_aes_ctxt_alloc and al_aes_ctxt_free
expect:
1. no assert
2. al_aes_ctxt_alloc returns non-null
*/
PFM_TEST_DESC_DEF(al_aes_alloc_free_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 1),
		pfm_tcase_make_name("aes", "C610096"), EXPECT_SUCCESS,
		NULL, testcases_aes_alloc_free, NULL);

/*
C610097 test al_aes_ctxt_free when ctxt is null
expect:
1. assert
*/
PFM_TEST_DESC_DEF(al_aes_free_invalid_ctxt_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 2),
		pfm_tcase_make_name("aes", "C610097"), EXPECT_SUCCESS,
		NULL, testcases_aes_free_invalid_ctxt, aes_alist);

/*
C610098 test al_aes_cbc_key_set and al_aes_cbc_encrypt with given key
expect:
1. no assert
2. al_aes_ctxt_alloc returns non-null
3. The encryption result is the same as the given result
*/
PFM_TEST_DESC_DEF(al_aes_encry_with_givenkey_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 3),
		pfm_tcase_make_name("aes", "C610098"), EXPECT_SUCCESS,
		NULL, testcases_aes_crypto_with_givenkey, aes_alist + 1);

/*
C610099 test al_aes_cbc_key_set and al_aes_cbc_decrypt with given key
expect:
1. no assert
2. al_aes_ctxt_alloc returns non-null
3. The decryption result is the same as the given result
*/
PFM_TEST_DESC_DEF(al_aes_decry_with_givenkey_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 4),
		pfm_tcase_make_name("aes", "C610099"), EXPECT_SUCCESS,
		NULL, testcases_aes_crypto_with_givenkey, aes_alist + 2);

/*
C610100 test al_aes_cbc_encrypt and al_aes_cbc_decrypt with random key
expect:
1. no assert
2. al_aes_ctxt_alloc returns non-null
3. The decryption result match the source string
*/
PFM_TEST_DESC_DEF(al_aes_crypto_with_randomkey_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 5),
		pfm_tcase_make_name("aes", "C610100"), EXPECT_SUCCESS,
		NULL, testcases_aes_crypto_with_random, aes_alist + 3);

/*
C610101 test al_aes_cbc_encrypt and al_aes_cbc_decrypt with random iv
expect:
1.no assert
2.al_aes_ctxt_alloc returns non-null
3.The decryption result match the source string
*/
PFM_TEST_DESC_DEF(al_aes_crypto_with_randomiv_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 6),
		pfm_tcase_make_name("aes", "C610101"), EXPECT_SUCCESS,
		NULL, testcases_aes_crypto_with_random, aes_alist + 4);

/*
C610102 test al_aes_cbc_encrypt and al_aes_cbc_decrypt with defferent key
expect:
1.no assert
2.al_aes_ctxt_alloc returns non-null
3.The decryption result do not match the given result
*/
PFM_TEST_DESC_DEF(al_aes_crypto_with_defferentkey_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 7),
		pfm_tcase_make_name("aes", "C610102"), EXPECT_SUCCESS,
		NULL, testcases_aes_crypto_with_defferent, aes_alist + 5);

/*
C610103 test al_aes_cbc_encrypt and al_aes_cbc_decrypt with defferent iv
expect:
1.no assert
2.al_aes_ctxt_alloc returns non-null
3.The decryption result do not match the given result
*/
PFM_TEST_DESC_DEF(al_aes_crypto_with_defferentiv_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 8),
		pfm_tcase_make_name("aes", "C610103"), EXPECT_SUCCESS,
		NULL, testcases_aes_crypto_with_defferent, aes_alist + 6);

/*
C610104 test al_aes_cbc_key_set with invalid ctxt
expect:
1.assert
*/
PFM_TEST_DESC_DEF(al_aes_key_set_invalid_ctxt_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 9),
		pfm_tcase_make_name("aes", "C610104"), EXPECT_FAILURE,
		NULL, testcases_aes_key_set_invalid_ctxt, aes_alist + 7);

/*
C610105 test al_aes_cbc_key_get with invalid ctxt
expect:
1.assert
*/
PFM_TEST_DESC_DEF(al_aes_iv_get_invalid_ctxt_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 10),
		pfm_tcase_make_name("aes", "C610105"), EXPECT_FAILURE,
		NULL, testcases_aes_iv_get_invalid_ctxt, aes_alist + 8);

/*
C610107 test al_aes_iv_get before calling al_aes_cbc_key_set
expect:
1.assert
*/
PFM_TEST_DESC_DEF(al_aes_iv_get_before_set_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 11),
		pfm_tcase_make_name("aes", "C610107"), EXPECT_FAILURE,
		NULL, testcases_aes_iv_get_before_set, aes_alist + 9);

/*
C610108 test al_aes_iv_get after calling al_aes_cbc_key_set
expect:
1.No assert
2.The iv returned by al_aes_iv_get is the same as the given iv
*/
PFM_TEST_DESC_DEF(al_aes_iv_get_after_set_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 12),
		pfm_tcase_make_name("aes", "C610108"), EXPECT_SUCCESS,
		NULL, testcases_aes_iv_get_after_set, aes_alist + 10);

/*
C610109 test al_aes_iv_get after calling al_aes_cbc_key_set
expect:
1.No assert
2.The iv returned by al_aes_iv_get is the same as the given iv
*/
PFM_TEST_DESC_DEF(al_aes_iv_get_after_encrypt_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 13),
		pfm_tcase_make_name("aes", "C610109"), EXPECT_SUCCESS,
		NULL, testcases_aes_iv_get_after_crypto, aes_alist + 11);

/*
C610110 test al_aes_iv_get after calling al_aes_cbc_decrypt
expect:
1.No assert
2.The iv returned by al_aes_iv_get is the same as the given iv
*/
PFM_TEST_DESC_DEF(al_aes_iv_get_after_decrypt_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 14),
		pfm_tcase_make_name("aes", "C610110"), EXPECT_SUCCESS,
		NULL, testcases_aes_iv_get_after_crypto, aes_alist + 12);

/*
C610116 test al_aes_cbc_encrypt with invalid ctxt
expect:
1.assert
*/
PFM_TEST_DESC_DEF(al_aes_encrypt_invalid_ctxt_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 15),
		pfm_tcase_make_name("aes", "C610116"), EXPECT_FAILURE,
		NULL, testcases_aes_encrypt_invalid_ctxt, aes_alist + 13);

/*
C610120 test al_aes_cbc_decrypt with invalid ctxt
expect:
1.assert
*/
PFM_TEST_DESC_DEF(al_aes_decrypt_invalid_ctxt_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 16),
		pfm_tcase_make_name("aes", "C610120"), EXPECT_FAILURE,
		NULL, testcases_aes_decrypt_invalid_ctxt, aes_alist + 14);

/*
C610124 pass decrypt=0 to al_aes_cbc_key_set and test al_aes_cbc_decrypt
expect:
1.assert
*/
PFM_TEST_DESC_DEF(al_aes_encrypt_when_decrypt_1_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 17),
		pfm_tcase_make_name("aes", "C610124"), EXPECT_FAILURE,
		NULL, testcases_aes_crypto_set_wrong_decrypt, aes_alist + 15);

/*
C610125 pass decrypt=1 to al_aes_cbc_key_set and test al_aes_cbc_encrypt
expect:
1.assert
*/
PFM_TEST_DESC_DEF(al_aes_decrypt_when_decrypt_0_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 18),
		pfm_tcase_make_name("aes", "C610125"), EXPECT_FAILURE,
		NULL, testcases_aes_crypto_set_wrong_decrypt, aes_alist + 16);

static void testcases_aes_combi_param_pre(struct pfm_tf_combi_param_info
		*info, int argc, char **argv)
{
	struct tca_aes_arg *p_param;

	p_param = (struct tca_aes_arg *)info->param;
	ASSERT(NULL != p_param);

	p_param->ctxt = al_aes_ctxt_alloc();
	ASSERT(NULL != p_param->ctxt);
}

static void testcases_aes_combi_param_clean(struct pfm_tf_combi_param_info
		*info)
{
	struct tca_aes_arg *p_param;

	p_param = (struct tca_aes_arg *)info->param;
	ASSERT(NULL != p_param);

	if (NULL != p_param && NULL != p_param->ctxt) {
		al_aes_ctxt_free(p_param->ctxt);
		p_param->ctxt = NULL;
	}
}

/*
C610114 test al_aes_key_get with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_VALS(aes_iv_get_invalid_size, size_t, 5, 0,
		(TEST_AES_BLOCK_SIZE-1), (TEST_AES_BLOCK_SIZE+1),
		(TEST_AES_BLOCK_SIZE<<10), (1<<20));

static const void *aes_iv_get_combi_param[] = {
	&invalid_ptr,
	&aes_iv_get_invalid_size,
};
static void testcases_aes_iv_get_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct tca_aes_arg *p_param;

	p_param = (struct tca_aes_arg *)info->param;
	ASSERT(NULL != p_param);

	/* run the test */
	test_printf("testing al_aes_iv_get(%p, %p, %zd) ...",
	    p_param->ctxt, p_param->out, p_param->in_len);
	al_aes_iv_get(p_param->ctxt, p_param->out, p_param->in_len);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(aes_iv_get_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 20),
		pfm_tcase_make_name("aes", "C610114"), 2, EXPECT_FAILURE, NULL,
		&aes_encrypt_arg_dft, sizeof(struct tca_aes_arg),
		aes_iv_get_combi_param,
		testcases_aes_combi_param_pre,
		testcases_aes_iv_get_combi_param,
		testcases_aes_combi_param_clean,
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, out),
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, in_len));

/*
C610115 test al_aes_key_set with invalid parameters
expect:
1.assert
*/
static const void *aes_key_set_combi_param[] = {
	&invalid_ptr,
	&invalid_size,
	&invalid_ptr,
};

static void testcases_aes_key_set_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct tca_aes_arg *p_param;
	int rc;

	p_param = (struct tca_aes_arg *)info->param;
	ASSERT(NULL != p_param);

	/* run the test */
	test_printf("testing al_aes_cbc_key_set(%p, %p, %zd, %p, %d) ...",
	    p_param->ctxt, p_param->key, p_param->key_len, p_param->iv,
	    p_param->decrypt);
	rc = al_aes_cbc_key_set(p_param->ctxt, p_param->key, p_param->key_len,
	    p_param->iv, p_param->decrypt);
	ASSERT(0 == rc);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(aes_key_set_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 21),
		pfm_tcase_make_name("aes", "C610115"), 3, EXPECT_FAILURE,
		NULL, &aes_encrypt_arg_dft, sizeof(struct tca_aes_arg),
		aes_key_set_combi_param,
		testcases_aes_combi_param_pre,
		testcases_aes_key_set_combi_param,
		testcases_aes_combi_param_clean,
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, key),
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, key_len),
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, iv));

/*
C610117 test al_aes_cbc_encrypt with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_VALS(aes_encrypt_invalid_size, size_t, 5, 0,
		(TEST_AES_BLOCK_SIZE-1), (TEST_AES_BLOCK_SIZE+1),
		(TEST_AES_BLOCK_SIZE<<10), (1<<20));
static const void *aes_encrypt_combi_param[] = {
	&invalid_ptr,
	&invalid_ptr,
	&aes_encrypt_invalid_size,
};

static void testcases_aes_encrypt_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct tca_aes_arg *p_param;
	int rc;

	p_param = (struct tca_aes_arg *)info->param;
	ASSERT(NULL != p_param);

	/* run the test */
	test_printf("testing al_aes_cbc_encrypt(%p, %p, %p, %zd) ...",
	    p_param->ctxt, p_param->in, p_param->out, p_param->in_len);
	rc = al_aes_cbc_encrypt(p_param->ctxt, p_param->in, p_param->out,
	    p_param->in_len);
	ASSERT(0 == rc);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(aes_encrypt_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 22),
		pfm_tcase_make_name("aes", "C610117"), 3, EXPECT_FAILURE,
		NULL, &aes_encrypt_arg_dft, sizeof(struct tca_aes_arg),
		aes_encrypt_combi_param,
		testcases_aes_combi_param_pre,
		testcases_aes_encrypt_combi_param,
		testcases_aes_combi_param_clean,
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, in),
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, out),
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, in_len));

/*
C610121 test al_aes_cbc_decrypt with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_VALS(aes_decrypt_invalid_size, size_t, 5, 0,
		(TEST_AES_BLOCK_SIZE-1), (TEST_AES_BLOCK_SIZE+1),
		(TEST_AES_BLOCK_SIZE<<10), (1<<20));
static const void *aes_decrypt_combi_param[] = {
	&invalid_ptr,
	&invalid_ptr,
	&aes_decrypt_invalid_size,
};

static void testcases_aes_decrypt_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct tca_aes_arg *p_param;
	int rc;

	p_param = (struct tca_aes_arg *)info->param;
	ASSERT(NULL != p_param);

	/* run the test */
	test_printf("testing al_aes_cbc_decrypt(%p, %p, %p, %zd) ...",
	    p_param->ctxt, p_param->in, p_param->out, p_param->in_len);
	rc = al_aes_cbc_decrypt(p_param->ctxt, p_param->in, p_param->out,
	    p_param->in_len);
	ASSERT(0 == rc);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(aes_decrypt_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 23),
		pfm_tcase_make_name("aes", "C610121"), 3, EXPECT_FAILURE,
		NULL, &aes_decrypt_arg_dft, sizeof(struct tca_aes_arg),
		aes_decrypt_combi_param,
		testcases_aes_combi_param_pre,
		testcases_aes_decrypt_combi_param,
		testcases_aes_combi_param_clean,
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, in),
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, out),
		PFM_COMBI_PARAM_INFO_ITEM(tca_aes_arg, in_len));

#define TEST_AES_THREAD_NUM	(2)
static void testcases_aes_mthread_crypto_step(
		struct pfm_tf_mthread_info *info)
{
	int count = 100;
	int rc;
	struct al_aes_ctxt *ctxt = NULL;
	size_t out_len;
	char aes_tmp[TEST_AES_BLOCK_SIZE];
	struct tca_aes_arg *aes_arg;
	char *result;

	aes_arg = (struct tca_aes_arg *)(info->param);
	if (NULL == aes_arg) {
		result = "invalid parameter";
		goto on_exit;
	}

	out_len = ((aes_arg->in_len)>>4)<<4;
	if ((aes_arg->in_len) > out_len) {
		out_len += 1<<4;
	}
	ctxt = al_aes_ctxt_alloc();
	if (NULL == ctxt) {
		result = "alloc failed";
		goto on_exit;
	}

	while (count--) {
		memset(aes_tmp, 0x0, sizeof(aes_tmp));
		rc = testcases_aes_crypto_block(ctxt, aes_arg->key,
		    aes_arg->key_len, aes_arg->iv, aes_arg->in,
		    aes_arg->in_len, aes_tmp, aes_arg->decrypt);
		if (0 != rc) {
			result = aes_arg->decrypt ? "decrypt failed"
					: "encrypt failed";
			goto on_exit;
		}

		rc = memcmp(aes_tmp, aes_arg->given, out_len);
		if ((0 != rc && 0 != aes_arg->exp_match)
		    || (0 == rc && 0 == aes_arg->exp_match)) {
			result = aes_arg->decrypt ?
			    "decrypt could not match the given"
			    : "encrypt could not match the given";
			goto on_exit;
		}
		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	if (NULL != ctxt) {
		al_aes_ctxt_free(ctxt);
	}
	pfm_test_frame_mthread_error(info, result);
}

static struct tca_aes_arg aes_mt_alist[] = {
	{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
		.iv = tcdp_aes_iv, .decrypt = 0,
		.in = tcdp_aes_enc_context,
		.given = tcdp_aes_dec_context, .exp_match = 1,
		.in_len = TEST_AES_BLOCK_SIZE},
	{ .key = tcdp_aes_key, .key_len = TEST_AES_BLOCK_SIZE,
		.iv = tcdp_aes_iv, .decrypt = 1,
		.in = tcdp_aes_dec_context,
		.given = tcdp_aes_enc_context, .exp_match = 1,
		.in_len = TEST_AES_BLOCK_SIZE},
};

/*
C610126 calls al_aes_cbc_encrypt in multi-thread mode
expect:
1.no assert
2.the encryption result match the given string
*/
PFM_MTHREAD_TEST_DESC_DEF(al_aes_mt_encrypt,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 24),
		pfm_tcase_make_name("aes", "C610126"),
		TEST_AES_THREAD_NUM, EXPECT_SUCCESS, NULL, aes_mt_alist,
		sizeof(struct tca_aes_arg), NULL,
		testcases_aes_mthread_crypto_step, NULL);

/*
C610127 calls al_aes_cbc_decrypt in multi-thread mode
expect:
1.no assert
2.the decryption result match the given string
*/
PFM_MTHREAD_TEST_DESC_DEF(al_aes_mt_decrypt,
		pfm_tcase_make_order(PFM_TCASE_ORD_AES, 25),
		pfm_tcase_make_name("aes", "C610127"),
		TEST_AES_THREAD_NUM, EXPECT_SUCCESS, NULL, aes_mt_alist + 1,
		sizeof(struct tca_aes_arg), NULL,
		testcases_aes_mthread_crypto_step, NULL);
