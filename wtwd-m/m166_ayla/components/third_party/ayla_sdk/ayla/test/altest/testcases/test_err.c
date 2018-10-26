/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */

#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_err_arg {
	enum al_err err;
};

static const struct al_err_arg al_err_string_dft = {
	.err = AL_ERR_OK,
};

PFM_COMBI_PARAM_VALS(al_err_string_valid_errs, enum al_err, 3,
		AL_ERR_OK, AL_ERR_BUSY, AL_ERR_CERT_EXP);
PFM_COMBI_PARAM_VALS(al_err_string_invalid_errs, enum al_err, 3,
		AL_ERR_OK - 1, AL_ERR_COUNT, AL_ERR_COUNT + 1);

/*
C610163 test al_err_string with normal parameters
expect:
1.no assert
2.return valid error string
*/
static const void *al_err_string_valid_param[] = {
	&al_err_string_valid_errs,
};

static void testcases_err_string_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_err_arg *p_param;
	const char *str;

	p_param = (struct al_err_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_err_string(%d) ...", p_param->err);
	str = al_err_string(p_param->err);
	ASSERT(NULL != str);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_err_string_valid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_ERR, 1),
		pfm_tcase_make_name("err", "C610163"), 1, EXPECT_SUCCESS,
		NULL, &al_err_string_dft, sizeof(struct al_err_arg),
		al_err_string_valid_param, NULL,
		testcases_err_string_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_err_arg, err));

/*
C610164 test al_err_string with invalid parameters
expect:
1.assert
*/
static const void *al_err_string_invalid_param[] = {
	&al_err_string_invalid_errs,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_err_string_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_ERR, 2),
		pfm_tcase_make_name("err", "C610164"), 1, EXPECT_FAILURE,
		NULL, &al_err_string_dft, sizeof(struct al_err_arg),
		al_err_string_invalid_param, NULL,
		testcases_err_string_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_err_arg, err));

/*
C610166 test al_err_string calling in mutil-thread
expect:
1.no assert
*/
#define TEST_ERR_THREAD_NUM	(2)
static void testcases_err_string_mthread(
		struct pfm_tf_mthread_info *info)
{
	int count = 100;
	struct al_err_arg *p_param;
	const char *str;
	char *result;

	p_param = (struct al_err_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}

	while (count--) {
		str = al_err_string(p_param->err);
		if (NULL == str) {
			result = "cannot get error string";
			goto on_exit;
		}

		p_param->err++;
		p_param->err %= AL_ERR_COUNT;

		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_err_string_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_ERR, 3),
		pfm_tcase_make_name("err", "C610166"),
		TEST_ERR_THREAD_NUM, EXPECT_SUCCESS, NULL, &al_err_string_dft,
		sizeof(struct al_err_arg), NULL,
		testcases_err_string_mthread, NULL);
