/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_random.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

#define TEST_RANDOM_THREAD_NUM	(2)
#define TEST_RANDOM_BUFSIZE	(4*1024)

/* min rate:45.0% */
#define TEST_RANDOM_MIN_RATE	(450)
/* max rate:55.0% */
#define TEST_RANDOM_MAX_RATE	(550)

#define TEST_RANDOM_TRY_NUM	(3)

struct al_random_fill_arg {
	void *buf;
	size_t len;
};

static char random_buffer[TEST_RANDOM_BUFSIZE];
static const struct al_random_fill_arg al_random_fill_dft = {
	.buf = random_buffer,
	.len = sizeof(random_buffer),
};

static u32 testcases_bit_count(u8 x)
{
	u32 count = 0;

	while (x > 0) {
		count += (x % 2);
		x = x >> 1;
	}

	return count;
}

/* how to check if the output data is random:
 * 1.if data length is 1 char, return true if data is non-zero,
 * if failed, try 3 times in total, test false if all tests fail.
 * 2.otherwise we count how many bits of data are 1, return true if
 * the rate is between TEST_RANDOM_MIN_RATE and TEST_RANDOM_MAX_RATE.
 *
 * NOTICE:If C610407 fail, fix random function or try
 * to change TEST_RANDOM_MIN_RATE and TEST_RANDOM_MAX_RATE
 */
static int testcases_is_random(char *buf, size_t len)
{
	u32 one_bits = 0, total_bits, rate;
	char bits;
	int i;

	/* If only 1 char, return 1 if it is non-zero */
	if (1 == len) {
		return (0 != buf[0]);
	}
	for (i = 0; i < len; i++) {
		bits = buf[i];
		one_bits += testcases_bit_count(bits);
	}
	total_bits = len << 3;

	/* if one_bits is too big */
	if (one_bits > (((u32)(-1)) >> 10)) {
		total_bits >>= 10;
	} else {
		one_bits <<= 10;
	}
	rate = one_bits/total_bits;

	if ((rate < TEST_RANDOM_MIN_RATE)
	    || (rate > TEST_RANDOM_MAX_RATE)) {
		test_printf("bit1 rate(%d/%d)=%d", one_bits, total_bits, rate);
		return 0;
	}

	return 1;
}

/*
C610407 test al_random_fill with normal parameters
expect:
1.no assert
2.al_random_fill output random data
*/
PFM_COMBI_PARAM_VALS(al_random_fill_valid_len, size_t, 2, 1,
		sizeof(random_buffer));

static const void *al_random_fill_valid_combi_param[] = {
	&al_random_fill_valid_len,
};

static void testcases_random_fill_valid(struct pfm_tf_combi_param_info
		*info)
{
	struct al_random_fill_arg *p_param;
	int is_random;
	int try_count = 0;

	p_param = (struct al_random_fill_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_random_fill(%p, %zd) ...",
	    p_param->buf, p_param->len);
on_try:
	try_count++;
	memset(p_param->buf, 0x0, p_param->len);
	al_random_fill(p_param->buf, p_param->len);
	is_random = testcases_is_random(p_param->buf, p_param->len);
	/* If get random failed, try again */
	if (!is_random && try_count < TEST_RANDOM_TRY_NUM) {
		goto on_try;
	}
	ASSERT(is_random);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_random_fill_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_RANDOM, 1),
		pfm_tcase_make_name("random", "C610407"), 1, EXPECT_SUCCESS,
		NULL, &al_random_fill_dft, sizeof(struct al_random_fill_arg),
		al_random_fill_valid_combi_param, NULL,
		testcases_random_fill_valid, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_random_fill_arg, len));

/*
C610408 test al_random_fill with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_VALS(al_random_fill_invalid_buf, void *, 1, NULL);
PFM_COMBI_PARAM_VALS(al_random_fill_invalid_len, size_t, 2, 0,
		(TEST_RANDOM_BUFSIZE + 1));

static const void *al_random_fill_invalid_combi_param[] = {
	&al_random_fill_invalid_buf,
	&al_random_fill_invalid_len,
};

static void testcases_random_fill_invalid(struct pfm_tf_combi_param_info
		*info)
{
	struct al_random_fill_arg *p_param;

	p_param = (struct al_random_fill_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_random_fill(%p, %zd) ...",
	    p_param->buf, p_param->len);
	al_random_fill(p_param->buf, p_param->len);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_random_fill_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_RANDOM, 2),
		pfm_tcase_make_name("random", "C610408"), 2, EXPECT_FAILURE,
		NULL, &al_random_fill_dft, sizeof(struct al_random_fill_arg),
		al_random_fill_invalid_combi_param, NULL,
		testcases_random_fill_invalid, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_random_fill_arg, buf),
		PFM_COMBI_PARAM_INFO_ITEM(al_random_fill_arg, len));

/*
C610410 test al_random_fill calling in mutil-thread
expect:
1.no assert
2.al_random_fill output random data
*/
static void testcases_random_fill_mthread(
		struct pfm_tf_mthread_info *info)
{
	int count = 100;
	int is_random = 0;
	struct al_random_fill_arg *p_param;
	char tmp_buf[1024];
	char *result;

	p_param = (struct al_random_fill_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}

	while (count--) {
		memset(tmp_buf, 0x0, sizeof(tmp_buf));
		al_random_fill(tmp_buf, sizeof(tmp_buf));
		is_random += testcases_is_random(tmp_buf, sizeof(tmp_buf));
		if (!is_random) {
			result = "failed to get random data";
			goto on_exit;
		}
		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_random_fill_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_RANDOM, 3),
		pfm_tcase_make_name("random", "C610410"),
		TEST_RANDOM_THREAD_NUM, EXPECT_SUCCESS, NULL,
		&al_random_fill_dft, sizeof(struct al_random_fill_arg), NULL,
		testcases_random_fill_mthread, NULL);
