/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_log.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_log_arg {
	const char *line;
	u8 mod_id;
};

static const struct al_log_arg al_log_arg_dft = {
	.line = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456\n",
	.mod_id = LOG_MOD_NONE,
};

/*
C610193 test al_log_print with normal parameters
expect:
1.no assert
2.show string in cli
*/
/* NULL is invalid parameter, but should not cause system crash,
 * so i put it with valid parameter for saving code. */
PFM_COMBI_PARAM_VALS(al_log_print_valid_lines, const char *, 6,
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"abcdefghijklmnopqrstuvwxyz",
		"1234567890",
		"~!@#$%^&*()_+{}|:\"<>?`[]\\\',./",
		"", NULL);

static const void *al_log_print_valid_combi_param[] = {
	&al_log_print_valid_lines,
};

static void testcases_log_print_test_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_log_arg *p_param;

	p_param = (struct al_log_arg *)info->param;
	ASSERT(NULL != p_param);

	if (p_param->line) {
		test_printf("testing al_log_print(\"%s\") ...", p_param->line);
	} else {
		test_printf("testing al_log_print(NULL) ...");
	}
	al_log_print(p_param->line);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_log_print_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_LOG, 1),
		pfm_tcase_make_name("log", "C610193"), 1, EXPECT_SUCCESS,
		NULL, &al_log_arg_dft, sizeof(struct al_log_arg),
		al_log_print_valid_combi_param, NULL,
		testcases_log_print_test_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_log_arg, line));

/*
C610194 test al_log_get_mod_name with normal parameters
expect:
1.no assert
*/
PFM_COMBI_PARAM_VALS(al_log_get_mod_name_valid_modid, u8, 2,
		LOG_MOD_APP_BASE, LOG_MOD_DEFAULT);

static const void *al_log_get_mod_name_valid_combi_param[] = {
	&al_log_get_mod_name_valid_modid,
};

static void testcases_log_get_mod_name_test_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_log_arg *p_param;
	const char *mod_str;

	p_param = (struct al_log_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_log_get_mod_name(%d) ...", p_param->mod_id);
	mod_str = al_log_get_mod_name(p_param->mod_id);
	ASSERT(NULL != mod_str);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_log_get_mod_name_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_LOG, 2),
		pfm_tcase_make_name("log", "C610194"), 1, EXPECT_SUCCESS,
		NULL, &al_log_arg_dft, sizeof(struct al_log_arg),
		al_log_get_mod_name_valid_combi_param, NULL,
		testcases_log_get_mod_name_test_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_log_arg, mod_id));

/*
C610195 test al_log_get_mod_name with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_VALS(al_log_get_mod_name_invalid_modid, u8, 3,
		LOG_MOD_NONE - 1, LOG_MOD_CT, LOG_MOD_CT + 1);

static const void *al_log_get_mod_name_invalid_combi_param[] = {
	&al_log_get_mod_name_invalid_modid,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_log_get_mod_name_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_LOG, 3),
		pfm_tcase_make_name("log", "C610195"), 1, EXPECT_FAILURE,
		NULL, &al_log_arg_dft, sizeof(struct al_log_arg),
		al_log_get_mod_name_invalid_combi_param, NULL,
		testcases_log_get_mod_name_test_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_log_arg, mod_id));

/*
C610196 test al_log_print and al_log_get_mod_name in mutil thread
expect:
1.no assert
2.show string in cli
*/
#define TEST_LOG_THREAD_NUM	(2)
static void testcases_log_test_mthread(
		struct pfm_tf_mthread_info *info)
{
	u8 count = 100;
	struct al_log_arg *log_arg;
	char *result;

	log_arg = (struct al_log_arg *)(info->param);
	if (NULL == log_arg) {
		result = "invalid parameter";
		goto on_exit;
	}

	while (count--) {
		al_log_get_mod_name(count);
		al_log_print(log_arg->line);
		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_log_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_LOG, 4),
		pfm_tcase_make_name("log", "C610196"),
		TEST_LOG_THREAD_NUM, EXPECT_SUCCESS, NULL, &al_log_arg_dft,
		sizeof(struct al_log_arg), NULL,
		testcases_log_test_mthread, NULL);
