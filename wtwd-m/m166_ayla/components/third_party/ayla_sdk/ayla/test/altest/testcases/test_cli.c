/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_random.h>
#include <al/al_os_thread.h>
#include <al/al_cli.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_cli_arg {
	const char *pmpt;
};

#define TEST_CLI_CMD_UPPER_CASE	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define TEST_CLI_CMD_LOWER_CASE	"abcdefghijklmnopqrstuvwxyz"
#define TEST_CLI_CMD_NUMBER	"1234567890"
#define TEST_CLI_CMD_PUNC	"~!@#$%^&*()_+{}|:\"<>?`[]\\\',./"

PFM_COMBI_PARAM_VALS(al_cli_invalid_str, const char *, 1, NULL);
PFM_COMBI_PARAM_VALS(al_cli_valid_str, const char *, 5, "",
		TEST_CLI_CMD_UPPER_CASE,
		TEST_CLI_CMD_LOWER_CASE,
		TEST_CLI_CMD_NUMBER,
		TEST_CLI_CMD_PUNC);

static struct al_cli_arg al_cli_dft = {
		.pmpt = TEST_CLI_CMD_UPPER_CASE,
};

static void testcases_cli_set_prompt_final(
		struct pfm_tf_combi_param_info *info)
{
	al_cli_set_prompt("PWB ALTest> ");
}
/*
C610152 test al_cli_set_prompt with normal parameters
expect:
1.no assert
2.show the pmpt in cli
*/
static const void *al_cli_set_prompt_valid_param[] = {
	&al_cli_valid_str,
};

static void testcases_cli_set_prompt_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_cli_arg *cli_arg;

	cli_arg = (struct al_cli_arg *)(info->param);
	ASSERT(NULL != cli_arg);

	if (NULL == cli_arg->pmpt) {
		test_printf("al_cli_set_prompt(NULL)");
	} else {
		test_printf("al_cli_set_prompt(\"%s\")", cli_arg->pmpt);
	}
	al_cli_set_prompt(cli_arg->pmpt);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_cli_set_prompt_valid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_CLI, 3),
		pfm_tcase_make_name("cli", "C610152"), 1, EXPECT_SUCCESS,
		NULL, &al_cli_dft, sizeof(struct al_cli_arg),
		al_cli_set_prompt_valid_param,
		NULL,
		testcases_cli_set_prompt_combi_param,
		testcases_cli_set_prompt_final,
		PFM_COMBI_PARAM_INFO_ITEM(al_cli_arg, pmpt));

/*
C610153 test al_cli_set_prompt with invalid parameters
expect:
1.assert
*/
static const void *al_cli_set_prompt_invalid_param[] = {
	&al_cli_invalid_str,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_cli_set_prompt_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_CLI, 4),
		pfm_tcase_make_name("cli", "C610153"), 1, EXPECT_FAILURE,
		NULL, &al_cli_dft, sizeof(struct al_cli_arg),
		al_cli_set_prompt_invalid_param, NULL,
		testcases_cli_set_prompt_combi_param,
		testcases_cli_set_prompt_final,
		PFM_COMBI_PARAM_INFO_ITEM(al_cli_arg, pmpt));
