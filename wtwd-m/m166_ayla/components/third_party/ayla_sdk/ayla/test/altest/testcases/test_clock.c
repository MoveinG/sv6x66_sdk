/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_random.h>
#include <al/al_os_thread.h>
#include <al/al_clock.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_clock_arg {
	u32 stamp;
	u32 src;
	enum al_clock_src *p_src;
};

PFM_COMBI_PARAM_VALS(al_clock_valid_stamp, u32, 6, -1, 0, 1,
		1<<10, 1<<20, 1<<30);
PFM_COMBI_PARAM_VALS(al_clock_valid_src, u32, 2, AL_CS_NONE, AL_CS_LIMIT - 1);
PFM_COMBI_PARAM_VALS(al_clock_invalid_src, u32, 3, AL_CS_NONE - 1,
		/* setting lower src would return error */
		AL_CS_LIMIT, AL_CS_LIMIT + 1);
PFM_COMBI_PARAM_VALS(al_clock_invalid_ptr, u32 *, 1, NULL);

static enum al_clock_src test_clock_src;
static struct al_clock_arg al_clock_set_dft = {
		.stamp = 1,
		.src = AL_CS_NONE,
		.p_src = &test_clock_src,
};

/*
C610157 test al_clock_set and al_clock_get with normal parameters
expect:
1.no assert
2.0 < (out_timestamp - in_timestamp) < 10
*/
static const void *al_clock_set_valid_combi_param[] = {
	&al_clock_valid_stamp,
	&al_clock_valid_src,
};

static void testcases_clock_set_and_get_param(
		struct pfm_tf_combi_param_info *info)
{
	int rc;
	struct al_clock_arg *clock_arg;
	u32 clock_stamp;

	clock_arg = (struct al_clock_arg *)(info->param);
	ASSERT(NULL != clock_arg);
	al_clock_reset_src();

	test_printf("al_clock_set(%d, %d)", clock_arg->stamp, clock_arg->src);
	rc = al_clock_set(clock_arg->stamp, clock_arg->src);
	ASSERT(0 == rc);

	clock_stamp = al_clock_get(clock_arg->p_src);
	ASSERT(clock_stamp - clock_arg->stamp >= 0);
	ASSERT(clock_stamp - clock_arg->stamp < 10);
	ASSERT(*(clock_arg->p_src) >= clock_arg->src);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_clock_set_and_get_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_CLOCK, 1),
		pfm_tcase_make_name("clock", "C610157"), 2, EXPECT_SUCCESS,
		NULL, &al_clock_set_dft, sizeof(struct al_clock_arg),
		al_clock_set_valid_combi_param,
		NULL,
		testcases_clock_set_and_get_param,
		NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_clock_arg, stamp),
		PFM_COMBI_PARAM_INFO_ITEM(al_clock_arg, src));

/*
C610158 test al_clock_set with invalid parameters
expect:
1.al_clock_set return non-zero
*/
static const void *al_clock_set_invalid_param[] = {
	&al_clock_invalid_src,
};

static void testcases_clock_set_invalid_param(
		struct pfm_tf_combi_param_info *info)
{
	int rc;
	struct al_clock_arg *clock_arg;

	clock_arg = (struct al_clock_arg *)(info->param);
	ASSERT(NULL != clock_arg);
	al_clock_reset_src();

	test_printf("al_clock_set(%d, %d)", clock_arg->stamp, clock_arg->src);
	rc = al_clock_set(clock_arg->stamp, clock_arg->src);
	ASSERT(0 != rc);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_clock_set_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_CLOCK, 2),
		pfm_tcase_make_name("clock", "C610158"), 1, EXPECT_SUCCESS,
		NULL, &al_clock_set_dft, sizeof(struct al_clock_arg),
		al_clock_set_invalid_param, NULL,
		testcases_clock_set_invalid_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_clock_arg, src));

/*
C610675 test al_clock_set with a lower clock source
expect:
1.al_clock_set return non-zero.
2.after calling al_clock_set, clock source should not be changed.
*/
static char *testcases_clock_set_lower_src(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	int rc;
	u32 clock_stamp;
	enum al_clock_src clock_src;

	/* precondition */
	al_clock_reset_src();
	clock_stamp = al_clock_get(&clock_src);

	clock_src = AL_CS_LIMIT - 1;
	rc = al_clock_set(clock_stamp, clock_src);
	ASSERT(0 == rc);

	/* check if clock source is AL_CS_LIMIT - 1 */
	clock_src = AL_CS_NONE;
	clock_stamp = al_clock_get(&clock_src);
	ASSERT((AL_CS_LIMIT - 1) == clock_src);

	/* testing */
	clock_src = AL_CS_LIMIT - 2;
	rc = al_clock_set(clock_stamp, clock_src);
	ASSERT(0 != rc);

	/* check if clock source is changed */
	clock_src = AL_CS_NONE;
	clock_stamp = al_clock_get(&clock_src);
	ASSERT((AL_CS_LIMIT - 1) == clock_src);

	return NULL;
}

PFM_TEST_DESC_DEF(al_clock_set_lower_src_func,
	pfm_tcase_make_order(PFM_TCASE_ORD_CLOCK, 3),
	pfm_tcase_make_name("clock", "C610675"), EXPECT_SUCCESS,
	NULL, testcases_clock_set_lower_src, NULL);

/*
C610161 test al_clock_get with invalid parameters
expect:
1.no assert
2.0 < (timestamp_2 - timestamp_1) < 10
*/
static const void *al_clock_get_invalid_param[] = {
	&al_clock_invalid_ptr,
};

static void testcases_clock_get_invalid_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_clock_arg *clock_arg;
	u32 clock_stamp;

	clock_arg = (struct al_clock_arg *)(info->param);
	ASSERT(NULL != clock_arg);

	test_printf("al_clock_get(%p)", clock_arg->p_src);
	clock_stamp = al_clock_get(clock_arg->p_src);
	ASSERT(clock_stamp != 0);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_clock_get_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_CLOCK, 4),
		pfm_tcase_make_name("clock", "C610161"), 1, EXPECT_SUCCESS,
		NULL, &al_clock_set_dft, sizeof(struct al_clock_arg),
		al_clock_get_invalid_param, NULL,
		testcases_clock_get_invalid_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_clock_arg, p_src));

/*
C610162 test al_clock_get_total_ms
expect:
1.no assert
2.al_clock_get_total_ms before and after a sleep for 1s,
900 < (timestamp_2 - timestamp_1) < 1100
*/
static char *testcases_clock_get_total_ms_func(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	u64 timestamp_1, timestamp_2;

	timestamp_1 = al_clock_get_total_ms();
	ASSERT(timestamp_1 > 0);
	al_os_thread_sleep(1000);
	timestamp_2 = al_clock_get_total_ms();
	ASSERT(timestamp_2 > 0);

	ASSERT(timestamp_2 - timestamp_1 > 900);
	ASSERT(timestamp_2 - timestamp_1 < 1100);
	return NULL;
}

PFM_TEST_DESC_DEF(al_clock_get_total_ms_func,
	pfm_tcase_make_order(PFM_TCASE_ORD_CLOCK, 5),
	pfm_tcase_make_name("clock", "C610162"), EXPECT_SUCCESS,
	NULL, testcases_clock_get_total_ms_func, NULL);
