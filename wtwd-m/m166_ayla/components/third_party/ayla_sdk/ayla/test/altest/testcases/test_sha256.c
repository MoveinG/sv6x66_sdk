/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_hash_sha256.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_sha256_arg {
	struct al_hash_sha256_ctxt *ctxt;
	void *buf;
	size_t len;
};

#define TEST_SHA256_BUFSIZE	AL_HASH_SHA256_SIZE
static char test_sha256_tmp[TEST_SHA256_BUFSIZE];

/**
 * how to get the test data:
 * 1.get the test string:
 * echo -n "ayla" | hexdump -C
 * 2.get the sum of sha256:
 * echo -n "ayla" | sha256sum
 */
static char test_sha256_long_src[] = {
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
	0x61, 0x79, 0x6c, 0x61, 0x61, 0x79, 0x6c, 0x61,
};
static char test_sha256_long_sum[] = {
	0xd2, 0x53, 0x02, 0x82, 0x6d, 0x66, 0xdb, 0x3f,
	0x56, 0x88, 0x4c, 0x66, 0xc8, 0x23, 0x56, 0x1c,
	0xea, 0x00, 0x78, 0x2f, 0xa5, 0x19, 0x03, 0x82,
	0xec, 0x9d, 0xb9, 0x90, 0xc5, 0x52, 0x55, 0x0f
};

static char test_sha256_short_src[] = {
	0x61
};

static char test_sha256_short_sum[] = {
	0xca, 0x97, 0x81, 0x12, 0xca, 0x1b, 0xbd, 0xca,
	0xfa, 0xc2, 0x31, 0xb3, 0x9a, 0x23, 0xdc, 0x4d,
	0xa7, 0x86, 0xef, 0xf8, 0x14, 0x7c, 0x4e, 0x72,
	0xb9, 0x80, 0x77, 0x85, 0xaf, 0xee, 0x48, 0xbb
};

static const struct al_sha256_arg al_sha256_dft = {
	.buf = test_sha256_tmp,
	.len = sizeof(test_sha256_tmp),
};

PFM_COMBI_PARAM_VALS(invalid_ptr, void *, 1, NULL);
PFM_COMBI_PARAM_VALS(invalid_size, size_t, 2, 0, 1<<31);

/*
C610180 test al_hash_sha256_ctxt_alloc and al_hash_sha256_ctxt_free
expect:
1.no assert
2.al_hash_sha256_ctxt_alloc returns context pointer
*/
static char *testcases_sha256_alloc_and_free(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_hash_sha256_ctxt *ctxt;

	ctxt = al_hash_sha256_ctxt_alloc();
	ASSERT(NULL != ctxt);

	al_hash_sha256_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha256_alloc_and_free,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 1),
		pfm_tcase_make_name("sha256", "C610180"), EXPECT_SUCCESS,
		NULL, testcases_sha256_alloc_and_free, NULL);

/*
C610181 test al_hash_sha256_ctxt_free with invalid parameters
expect:
1.no assert
*/
static char *testcases_sha256_free_invalid_ctx(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	al_hash_sha256_ctxt_free(NULL);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha256_free_invalid_ctx,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 2),
		pfm_tcase_make_name("sha256", "C610181"), EXPECT_SUCCESS,
		NULL, testcases_sha256_free_invalid_ctx, NULL);

/*
C610182 test al_hash_sha256_ctxt_init with normal parameters
expect:
1.no assert
2.al_hash_sha256_ctxt_alloc returns context pointer
*/
static char *testcases_sha256_init_valid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_hash_sha256_ctxt *ctxt;

	ctxt = al_hash_sha256_ctxt_alloc();
	ASSERT(NULL != ctxt);

	al_hash_sha256_ctxt_init(ctxt);
	al_hash_sha256_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha256_init_valid_ctxt,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 3),
		pfm_tcase_make_name("sha256", "C610182"), EXPECT_SUCCESS,
		NULL, testcases_sha256_init_valid_ctxt, NULL);

/*
C610183 test al_hash_sha256_ctxt_init with invalid parameters
expect:
1.assert
*/
static char *testcases_sha256_init_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	al_hash_sha256_ctxt_init(NULL);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha256_init_invalid_ctxt,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 4),
		pfm_tcase_make_name("sha256", "C610183"), EXPECT_FAILURE,
		NULL, testcases_sha256_init_invalid_ctxt, NULL);

/*
C610184 test al_hash_sha256_add and al_hash_sha256_final
expect:
1.no assert
2.al_hash_sha256_ctxt_alloc returns context pointer
3.the output of al_hash_sha256_final match the given result
*/
static char *testcases_sha256_add_and_final(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	struct al_hash_sha256_ctxt *ctxt;

	ctxt = al_hash_sha256_ctxt_alloc();
	ASSERT(NULL != ctxt);

	/* test the long code */
	memset(test_sha256_tmp, 0x0, sizeof(test_sha256_tmp));
	al_hash_sha256_ctxt_init(ctxt);

	al_hash_sha256_add(ctxt, test_sha256_long_src,
	    sizeof(test_sha256_long_src));
	al_hash_sha256_final(ctxt, test_sha256_tmp);
	rc = memcmp(test_sha256_tmp, test_sha256_long_sum,
	    sizeof(test_sha256_long_sum));
	ASSERT(0 == rc);

	/* test the short code */
	memset(test_sha256_tmp, 0x0, sizeof(test_sha256_tmp));
	al_hash_sha256_ctxt_init(ctxt);

	al_hash_sha256_add(ctxt, test_sha256_short_src,
	    sizeof(test_sha256_short_src));
	al_hash_sha256_final(ctxt, test_sha256_tmp);
	rc = memcmp(test_sha256_tmp, test_sha256_short_sum,
	    sizeof(test_sha256_short_sum));
	ASSERT(0 == rc);

	al_hash_sha256_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha256_add_and_final,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 5),
		pfm_tcase_make_name("sha256", "C610184"), EXPECT_SUCCESS,
		NULL, testcases_sha256_add_and_final, NULL);

/*
C610185 test al_hash_sha256_add with invalid parameters
expect:
1.assert
*/
static const void *al_sha256_add_invalid_param[] = {
	&invalid_ptr,
	&invalid_ptr,
	&invalid_size,
};

static void testcases_sha256_add_invalid_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_sha256_arg *p_param;

	p_param = (struct al_sha256_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_hash_sha256_add(%p, %p, %zd) ...",
	    p_param->ctxt, p_param->buf, p_param->len);
	al_hash_sha256_add(p_param->ctxt, p_param->buf, p_param->len);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_sha256_add_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 6),
		pfm_tcase_make_name("sha256", "C610185"), 3, EXPECT_FAILURE,
		NULL, &al_sha256_dft, sizeof(struct al_sha256_arg),
		al_sha256_add_invalid_param, NULL,
		testcases_sha256_add_invalid_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_sha256_arg, ctxt),
		PFM_COMBI_PARAM_INFO_ITEM(al_sha256_arg, buf),
		PFM_COMBI_PARAM_INFO_ITEM(al_sha256_arg, len));

/*
C610189 test al_hash_sha256_final with invalid parameters
expect:
1.assert
*/
static const void *al_sha256_final_invalid_param[] = {
	&invalid_ptr,
	&invalid_ptr,
};

static void testcases_sha256_final_invalid_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_sha256_arg *p_param;

	p_param = (struct al_sha256_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_hash_sha256_final(%p, %p) ...",
	    p_param->ctxt, p_param->buf);
	al_hash_sha256_final(p_param->ctxt, p_param->buf);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_sha256_final_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 7),
		pfm_tcase_make_name("sha256", "C610189"), 2, EXPECT_FAILURE,
		NULL, &al_sha256_dft, sizeof(struct al_sha256_arg),
		al_sha256_final_invalid_param, NULL,
		testcases_sha256_final_invalid_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_sha256_arg, ctxt),
		PFM_COMBI_PARAM_INFO_ITEM(al_sha256_arg, buf));

/*
C610191 test al_hash_sha256_add and al_hash_sha256_final in mutil-thread
expect:
1.no assert
2.sha256 encode success
*/
#define TEST_SHA256_THREAD_NUM	(2)
static void testcases_sha256_mthread_crypto(
		struct pfm_tf_mthread_info *info)
{
	int rc;
	int count = 100;
	struct al_hash_sha256_ctxt *ctxt = NULL;
	char *result;

	ctxt = al_hash_sha256_ctxt_alloc();
	if (NULL == ctxt) {
		result = "al_hash_sha256_ctxt_alloc failed";
		goto on_exit;
	}


	while (count--) {
		al_hash_sha256_ctxt_init(ctxt);
		al_hash_sha256_add(ctxt, test_sha256_long_src,
		    sizeof(test_sha256_long_src));
		al_hash_sha256_final(ctxt, test_sha256_tmp);
		rc = memcmp(test_sha256_tmp, test_sha256_long_sum,
		    sizeof(test_sha256_long_sum));
		if (0 != rc) {
			result = "al_hash_sha256 data error";
			goto on_exit;
		}
		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	if (NULL != ctxt) {
		al_hash_sha256_ctxt_free(ctxt);
	}
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_sha256_mt_crypt,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 8),
		pfm_tcase_make_name("sha256", "C610191"),
		TEST_SHA256_THREAD_NUM, EXPECT_SUCCESS, NULL, &al_sha256_dft,
		sizeof(struct al_sha256_arg), NULL,
		testcases_sha256_mthread_crypto, NULL);

/*
C610192 test al_hash_sha256_final without calling al_hash_sha256_add
expect:
1.no assert
*/
static char *testcases_sha256_final_without_add(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_hash_sha256_ctxt *ctxt;

	ctxt = al_hash_sha256_ctxt_alloc();
	ASSERT(NULL != ctxt);

	al_hash_sha256_ctxt_init(ctxt);

	al_hash_sha256_final(ctxt, test_sha256_tmp);

	al_hash_sha256_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha256_final_without_add,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA256, 9),
		pfm_tcase_make_name("sha256", "C610192"), EXPECT_SUCCESS,
		NULL, testcases_sha256_final_without_add, NULL);
