/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_assert.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_assert_handle_arg {
	const char *file;
	int line;
};

static const struct al_assert_handle_arg al_assert_handle_dft = {
	.file = __FILE__,
	.line = __LINE__,
};

PFM_COMBI_PARAM_VALS(al_assert_handle_files, const char *, 3,
		__FILE__, NULL, "");
PFM_COMBI_PARAM_VALS(al_assert_handle_lines, int, 3,
		__LINE__, 0, -1);

static const void *al_assert_handle_combi_param[] = {
	&al_assert_handle_files,
	&al_assert_handle_lines,
};

/*
C610128 test al_assert_handle with normal parameters
expect:
1.assert
*/
static void testcases_assert_handle(struct pfm_tf_combi_param_info
		*info)
{
	struct al_assert_handle_arg *p_param;

	p_param = (struct al_assert_handle_arg *)info->param;
	ASSERT(NULL != p_param);

	if (p_param->file) {
		test_printf("testing al_assert_handle(\"%s\", %d) ...",
		    p_param->file, p_param->line);
	} else {
		test_printf("testing al_assert_handle(NULL, %d) ...",
		    p_param->line);
	}
	al_assert_handle(p_param->file, p_param->line);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_assert_handle_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_ASSERT, 1),
		pfm_tcase_make_name("assert", "C610128"), 2, EXPECT_FAILURE,
		NULL, &al_assert_handle_dft,
		sizeof(struct al_assert_handle_arg),
		al_assert_handle_combi_param, NULL,
		testcases_assert_handle, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_assert_handle_arg, file),
		PFM_COMBI_PARAM_INFO_ITEM(al_assert_handle_arg, line));
