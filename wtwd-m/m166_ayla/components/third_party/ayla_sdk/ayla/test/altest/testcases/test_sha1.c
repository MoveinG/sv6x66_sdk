/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_hash_sha1.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_sha1_arg {
	struct al_hash_sha1_ctxt *ctxt;
	void *buf;
	size_t len;
};

#define TEST_SHA1_BUFSIZE	AL_HASH_SHA1_SIZE
static char test_sha1_tmp[TEST_SHA1_BUFSIZE];

/**
 * how to get the test data:
 * 1.get the test string:
 * echo -n "ayla" | hexdump -C
 * 2.get the sum of sha1:
 * echo -n "ayla" | sha1sum
 */
static char test_sha1_long_src[] = {
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
static char test_sha1_long_sum[] = {
	0xad, 0x8d, 0x56, 0xa7, 0x0a, 0x9f, 0x9e, 0x04,
	0x47, 0x68, 0xf6, 0x75, 0x94, 0x35, 0x5c, 0xd1,
	0x3c, 0xaa, 0xe0, 0xc5
};

static char test_sha1_short_src[] = {
	0x61
};

static char test_sha1_short_sum[] = {
	0x86, 0xf7, 0xe4, 0x37, 0xfa, 0xa5, 0xa7, 0xfc,
	0xe1, 0x5d, 0x1d, 0xdc, 0xb9, 0xea, 0xea, 0xea,
	0x37, 0x76, 0x67, 0xb8
};

static const struct al_sha1_arg al_sha1_dft = {
	.buf = test_sha1_tmp,
	.len = sizeof(test_sha1_tmp),
};

PFM_COMBI_PARAM_VALS(invalid_ptr, void *, 1, NULL);
PFM_COMBI_PARAM_VALS(invalid_size, size_t, 2, 0, 1<<31);

/*
C610167 test al_hash_sha1_ctxt_alloc and al_hash_sha1_ctxt_free
expect:
1.no assert
2.al_hash_sha1_ctxt_alloc returns context pointer
*/
static char *testcases_sha1_alloc_and_free(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_hash_sha1_ctxt *ctxt;

	ctxt = al_hash_sha1_ctxt_alloc();
	ASSERT(NULL != ctxt);

	al_hash_sha1_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha1_alloc_and_free,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 1),
		pfm_tcase_make_name("sha1", "C610167"), EXPECT_SUCCESS,
		NULL, testcases_sha1_alloc_and_free, NULL);

/*
C610168 test al_hash_sha1_ctxt_free with invalid parameters
expect:
1.no assert
*/
static char *testcases_sha1_free_invalid_ctx(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	al_hash_sha1_ctxt_free(NULL);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha1_free_invalid_ctx,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 2),
		pfm_tcase_make_name("sha1", "C610168"), EXPECT_SUCCESS,
		NULL, testcases_sha1_free_invalid_ctx, NULL);

/*
C610169 test al_hash_sha1_ctxt_init with normal parameters
expect:
1.no assert
2.al_hash_sha1_ctxt_alloc returns context pointer
*/
static char *testcases_sha1_init_valid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_hash_sha1_ctxt *ctxt;

	ctxt = al_hash_sha1_ctxt_alloc();
	ASSERT(NULL != ctxt);

	al_hash_sha1_ctxt_init(ctxt);
	al_hash_sha1_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha1_init_valid_ctxt,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 3),
		pfm_tcase_make_name("sha1", "C610169"), EXPECT_SUCCESS,
		NULL, testcases_sha1_init_valid_ctxt, NULL);

/*
C610170 test al_hash_sha1_ctxt_init with invalid parameters
expect:
1.assert
*/
static char *testcases_sha1_init_invalid_ctxt(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	al_hash_sha1_ctxt_init(NULL);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha1_init_invalid_ctxt,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 4),
		pfm_tcase_make_name("sha1", "C610170"), EXPECT_FAILURE,
		NULL, testcases_sha1_init_invalid_ctxt, NULL);

/*
C610171 test al_hash_sha1_add and al_hash_sha1_final
expect:
1.no assert
2.al_hash_sha1_ctxt_alloc returns context pointer
3.the output of al_hash_sha1_final match the given result
*/
static char *testcases_sha1_add_and_final(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	struct al_hash_sha1_ctxt *ctxt;

	ctxt = al_hash_sha1_ctxt_alloc();
	ASSERT(NULL != ctxt);

	/* test the long code */
	memset(test_sha1_tmp, 0x0, sizeof(test_sha1_tmp));
	al_hash_sha1_ctxt_init(ctxt);

	al_hash_sha1_add(ctxt, test_sha1_long_src,
	    sizeof(test_sha1_long_src));
	al_hash_sha1_final(ctxt, test_sha1_tmp);
	rc = memcmp(test_sha1_tmp, test_sha1_long_sum,
	    sizeof(test_sha1_long_sum));
	ASSERT(0 == rc);

	/* test the short code */
	memset(test_sha1_tmp, 0x0, sizeof(test_sha1_tmp));
	al_hash_sha1_ctxt_init(ctxt);

	al_hash_sha1_add(ctxt, test_sha1_short_src,
	    sizeof(test_sha1_short_src));
	al_hash_sha1_final(ctxt, test_sha1_tmp);
	rc = memcmp(test_sha1_tmp, test_sha1_short_sum,
	    sizeof(test_sha1_short_sum));
	ASSERT(0 == rc);

	al_hash_sha1_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha1_add_and_final,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 5),
		pfm_tcase_make_name("sha1", "C610171"), EXPECT_SUCCESS,
		NULL, testcases_sha1_add_and_final, NULL);

/*
C610172 test al_hash_sha1_add with invalid parameters
expect:
1.assert
*/
static const void *al_sha1_add_invalid_param[] = {
	&invalid_ptr,
	&invalid_ptr,
	&invalid_size,
};

static void testcases_sha1_add_invalid_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_sha1_arg *p_param;

	p_param = (struct al_sha1_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_hash_sha1_add(%p, %p, %zd) ...",
	    p_param->ctxt, p_param->buf, p_param->len);
	al_hash_sha1_add(p_param->ctxt, p_param->buf, p_param->len);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_sha1_add_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 6),
		pfm_tcase_make_name("sha1", "C610172"), 3, EXPECT_FAILURE,
		NULL, &al_sha1_dft, sizeof(struct al_sha1_arg),
		al_sha1_add_invalid_param, NULL,
		testcases_sha1_add_invalid_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_sha1_arg, ctxt),
		PFM_COMBI_PARAM_INFO_ITEM(al_sha1_arg, buf),
		PFM_COMBI_PARAM_INFO_ITEM(al_sha1_arg, len));

/*
C610176 test al_hash_sha1_final with invalid parameters
expect:
1.assert
*/
static const void *al_sha1_final_invalid_param[] = {
	&invalid_ptr,
	&invalid_ptr,
};

static void testcases_sha1_final_invalid_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_sha1_arg *p_param;

	p_param = (struct al_sha1_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_hash_sha1_final(%p, %p) ...",
	    p_param->ctxt, p_param->buf);
	al_hash_sha1_final(p_param->ctxt, p_param->buf);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_sha1_final_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 7),
		pfm_tcase_make_name("sha1", "C610176"), 2, EXPECT_FAILURE,
		NULL, &al_sha1_dft, sizeof(struct al_sha1_arg),
		al_sha1_final_invalid_param, NULL,
		testcases_sha1_final_invalid_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_sha1_arg, ctxt),
		PFM_COMBI_PARAM_INFO_ITEM(al_sha1_arg, buf));

/*
C610178 test al_hash_sha1_add and al_hash_sha1_final in mutil-thread
expect:
1.no assert
2.sha1 encode success
*/
#define TEST_SHA1_THREAD_NUM	(2)
static void testcases_sha1_mthread_crypto(
		struct pfm_tf_mthread_info *info)
{
	int rc;
	int count = 100;
	struct al_hash_sha1_ctxt *ctxt = NULL;
	char *result;

	ctxt = al_hash_sha1_ctxt_alloc();
	if (NULL == ctxt) {
		result = "al_hash_sha1_ctxt_alloc failed";
		goto on_exit;
	}


	while (count--) {
		al_hash_sha1_ctxt_init(ctxt);
		al_hash_sha1_add(ctxt, test_sha1_long_src,
		    sizeof(test_sha1_long_src));
		al_hash_sha1_final(ctxt, test_sha1_tmp);
		rc = memcmp(test_sha1_tmp, test_sha1_long_sum,
		    sizeof(test_sha1_long_sum));
		if (0 != rc) {
			result = "al_hash_sha1 data error";
			goto on_exit;
		}
		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	if (NULL != ctxt) {
		al_hash_sha1_ctxt_free(ctxt);
	}
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_sha1_mt_crypt,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 8),
		pfm_tcase_make_name("sha1", "C610178"),
		TEST_SHA1_THREAD_NUM, EXPECT_SUCCESS, NULL, &al_sha1_dft,
		sizeof(struct al_sha1_arg), NULL,
		testcases_sha1_mthread_crypto, NULL);

/*
C610179 test al_hash_sha1_final without calling al_hash_sha1_add
expect:
1.no assert
*/
static char *testcases_sha1_final_without_add(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_hash_sha1_ctxt *ctxt;

	ctxt = al_hash_sha1_ctxt_alloc();
	ASSERT(NULL != ctxt);

	al_hash_sha1_ctxt_init(ctxt);

	al_hash_sha1_final(ctxt, test_sha1_tmp);

	al_hash_sha1_ctxt_free(ctxt);
	return NULL;
}

PFM_TEST_DESC_DEF(al_sha1_final_without_add,
		pfm_tcase_make_order(PFM_TCASE_ORD_SHA1, 9),
		pfm_tcase_make_name("sha1", "C610179"), EXPECT_SUCCESS,
		NULL, testcases_sha1_final_without_add, NULL);
