/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */

#include <string.h>
#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_log.h>
#include <al/al_os_mem.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

#define TEST_MEM_THREAD_NUM	(2)
#define TEST_MEM_THREAD_COUNT	(1000)

#define TEST_MEM_SET_CONTENT	(0x0a)

struct al_mem_arg {
	enum al_os_mem_type type;
	size_t size;
	void *mem;
};

static const struct al_mem_arg al_mem_arg_dft = {
	.type = al_os_mem_type_long_period,
	.size = 1,
	.mem = NULL,
};

PFM_COMBI_PARAM_VALS(al_mem_valid_type, enum al_os_mem_type, 2,
		al_os_mem_type_long_period, al_os_mem_type_long_cache);
PFM_COMBI_PARAM_VALS(al_mem_valid_size, size_t, 3, 1, (1<<10), (1<<20));

PFM_COMBI_PARAM_VALS(al_mem_invalid_type, enum al_os_mem_type, 2,
		al_os_mem_type_long_period - 1, al_os_mem_type_long_cache + 1);
PFM_COMBI_PARAM_VALS(al_mem_invalid_size, size_t, 2, 0, (0x80000000UL));
PFM_COMBI_PARAM_VALS(al_mem_invalid_mem, void *, 1, NULL);

static int testcases_check_mem(const char *buf, int c, size_t len)
{
	int i;
	char val;

	val = 0xff & c;
	for (i = 0; i < len; i++) {
		if (buf[i] != val) {
			return -1;
		}
	}
	return 0;
}
/*
C610331 test al_os_mem_set_type and al_os_mem_get_type with normal parameters
expect:
1.no assert
2.set type equals the type return by al_os_mem_get_type
*/
static const void *al_mem_valid_type_combi_param[] = {
	&al_mem_valid_type,
};

static void testcases_mem_set_and_get_type(struct pfm_tf_combi_param_info
		*info)
{
	struct al_mem_arg *p_param;
	enum al_os_mem_type type;

	p_param = (struct al_mem_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_mem_set_type(%d) ...", p_param->type);
	al_os_mem_set_type(p_param->type);
	type = al_os_mem_get_type();

	ASSERT(type == p_param->type);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_mem_set_and_get_type,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 1),
		pfm_tcase_make_name("mem", "C610331"), 1, EXPECT_SUCCESS,
		NULL, &al_mem_arg_dft, sizeof(struct al_mem_arg),
		al_mem_valid_type_combi_param, NULL,
		testcases_mem_set_and_get_type, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, type));

/*
C610332 test al_os_mem_set_type with invalid parameters
expect:
1.assert
*/
static const void *al_mem_invalid_type_combi_param[] = {
	&al_mem_invalid_type,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_mem_set_invalid_type,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 2),
		pfm_tcase_make_name("mem", "C610332"), 1, EXPECT_FAILURE,
		NULL, &al_mem_arg_dft, sizeof(struct al_mem_arg),
		al_mem_invalid_type_combi_param, NULL,
		testcases_mem_set_and_get_type, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, type));

/*
C610334 test al_os_mem_alloc and al_os_mem_free
expect:
1.no assert
2.al_os_mem_alloc returns non-null pointer
*/
static const void *al_mem_alloc_valid_combi_param[] = {
	&al_mem_valid_type,
	&al_mem_valid_size,
};

static void testcases_mem_alloc_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	int rc;
	struct al_mem_arg *p_param;

	p_param = (struct al_mem_arg *)info->param;
	ASSERT(NULL != p_param);

	al_os_mem_set_type(p_param->type);
	test_printf("testing type(%d) al_os_mem_alloc(%zu) ...",
	    p_param->type, p_param->size);
	p_param->mem = al_os_mem_alloc(p_param->size);
	ASSERT(NULL != p_param->mem);

	memset(p_param->mem, TEST_MEM_SET_CONTENT, p_param->size);
	rc = testcases_check_mem(p_param->mem, TEST_MEM_SET_CONTENT,
	    p_param->size);
	ASSERT(0 == rc);

	al_os_mem_free(p_param->mem);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(mem_alloc_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 3),
		pfm_tcase_make_name("mem", "C610334"), 2, EXPECT_SUCCESS,
		NULL, &al_mem_arg_dft, sizeof(struct al_mem_arg),
		al_mem_alloc_valid_combi_param, NULL,
		testcases_mem_alloc_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, type),
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, size));

/*
C610335 test al_os_mem_alloc with invalid parameters
expect:
1.assert
*/

static const void *al_mem_alloc_invalid_combi_param[] = {
	&al_mem_invalid_size,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(mem_alloc_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 4),
		pfm_tcase_make_name("mem", "C610335"), 1, EXPECT_FAILURE,
		NULL, &al_mem_arg_dft, sizeof(struct al_mem_arg),
		al_mem_alloc_invalid_combi_param, NULL,
		testcases_mem_alloc_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, size));

/*
C610336 test al_os_mem_alloc calling in mutil-thread
expect:
1.no assert
2.al_os_mem_alloc return non-null pointer
*/

static void testcases_mem_alloc_in_mthread(
		struct pfm_tf_mthread_info *info)
{
	int count = TEST_MEM_THREAD_COUNT;
	int rc;
	struct al_mem_arg *p_param;
	char *result;

	p_param = (struct al_mem_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}


	while (count--) {
		p_param->mem = al_os_mem_alloc(p_param->size);
		if (NULL == p_param->mem) {
			result = "mem alloc failed";
			goto on_exit;
		}

		memset(p_param->mem, info->id, p_param->size);
		rc = testcases_check_mem(p_param->mem, info->id,
		    p_param->size);
		if (0 != rc) {
			result = "mem write data error";
			goto on_exit;
		}

		al_os_mem_free(p_param->mem);
		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(mem_alloc_in_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 5),
		pfm_tcase_make_name("mem", "C610336"),
		TEST_MEM_THREAD_NUM, EXPECT_SUCCESS, NULL, &al_mem_arg_dft,
		sizeof(struct al_mem_arg), NULL,
		testcases_mem_alloc_in_mthread, NULL);

/*
C610337 test al_os_mem_free with invalid parameters
expect:
1.no assert
*/
static const void *al_mem_free_invalid_combi_param[] = {
	&al_mem_valid_type,
	&al_mem_invalid_mem,
};

static void testcases_mem_free_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_mem_arg *p_param;

	p_param = (struct al_mem_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing type(%d) al_os_mem_free(%p) ...", p_param->type,
	    p_param->mem);
	al_os_mem_set_type(p_param->type);
	al_os_mem_free(p_param->mem);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(mem_free_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 6),
		pfm_tcase_make_name("mem", "C610337"), 2, EXPECT_SUCCESS,
		NULL, &al_mem_arg_dft, sizeof(struct al_mem_arg),
		al_mem_free_invalid_combi_param, NULL,
		testcases_mem_free_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, type),
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, mem));

/*
C610338 test al_os_mem_calloc and al_os_mem_free
expect:
1.no assert
2.al_os_mem_alloc returns non-null pointer
*/
static const void *al_mem_calloc_valid_combi_param[] = {
	&al_mem_valid_type,
	&al_mem_valid_size,
};

static void testcases_mem_calloc_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_mem_arg *p_param;
	int rc;

	p_param = (struct al_mem_arg *)info->param;
	ASSERT(NULL != p_param);

	al_os_mem_set_type(p_param->type);
	test_printf("testing type(%d) al_os_mem_alloc(%zu) ...",
	    p_param->type, p_param->size);
	p_param->mem = al_os_mem_calloc(p_param->size);
	ASSERT(NULL != p_param->mem);

	rc = testcases_check_mem(p_param->mem, 0x0, p_param->size);
	ASSERT(0 == rc);

	memset(p_param->mem, TEST_MEM_SET_CONTENT, p_param->size);
	rc = testcases_check_mem(p_param->mem, TEST_MEM_SET_CONTENT,
	    p_param->size);
	ASSERT(0 == rc);

	al_os_mem_free(p_param->mem);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(mem_calloc_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 7),
		pfm_tcase_make_name("mem", "C610338"), 2, EXPECT_SUCCESS,
		NULL, &al_mem_arg_dft, sizeof(struct al_mem_arg),
		al_mem_calloc_valid_combi_param, NULL,
		testcases_mem_calloc_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, type),
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, size));

/*
C610339 test al_os_mem_calloc with invalid parameters
expect:
1.assert
*/

static const void *al_mem_calloc_invalid_combi_param[] = {
	&al_mem_invalid_size,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(mem_calloc_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 8),
		pfm_tcase_make_name("mem", "C610339"), 1, EXPECT_FAILURE,
		NULL, &al_mem_arg_dft, sizeof(struct al_mem_arg),
		al_mem_calloc_invalid_combi_param, NULL,
		testcases_mem_calloc_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_mem_arg, size));

/*
C610340 test al_os_mem_calloc calling in mutil-thread
expect:
1.no assert
*/

static void testcases_mem_calloc_in_mthread(
		struct pfm_tf_mthread_info *info)
{
	int count = TEST_MEM_THREAD_COUNT;
	struct al_mem_arg *p_param;
	char *result;
	int rc;

	p_param = (struct al_mem_arg *)(info->param);
	if (NULL == p_param) {
		result = "invalid parameter";
		goto on_exit;
	}


	while (count--) {
		p_param->mem = al_os_mem_calloc(p_param->size);
		if (NULL == p_param->mem) {
			result = "mem calloc failed";
			goto on_exit;
		}
		rc = testcases_check_mem(p_param->mem, 0x0, p_param->size);
		if (0 != rc) {
			result = "mem is not cleaned";
			goto on_exit;
		}

		memset(p_param->mem, info->id, p_param->size);
		rc = testcases_check_mem(p_param->mem, info->id,
		    p_param->size);
		if (0 != rc) {
			result = "mem write data error";
			goto on_exit;
		}
		al_os_mem_free(p_param->mem);
		al_os_thread_sleep(info->id);
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(mem_calloc_in_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_MEM, 9),
		pfm_tcase_make_name("mem", "C610340"),
		TEST_MEM_THREAD_NUM, EXPECT_SUCCESS, NULL, &al_mem_arg_dft,
		sizeof(struct al_mem_arg), NULL,
		testcases_mem_calloc_in_mthread, NULL);
