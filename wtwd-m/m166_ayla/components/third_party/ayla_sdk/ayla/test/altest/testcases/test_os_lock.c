/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */

#include <ayla/log.h>
#include <al/al_err.h>
#include <al/al_log.h>
#include <al/al_os_lock.h>
#include <al/al_clock.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_lock_arg {
	struct al_lock *lock;
};

static const struct al_lock_arg al_lock_arg_dft = {
	.lock = NULL,
};

PFM_COMBI_PARAM_VALS(al_lock_invalid_arg, struct al_lock *, 1, NULL);

static const void *al_lock_invalid_combi_param[] = {
	&al_lock_invalid_arg,
};

/*
C610326 test al_os_lock_create and al_os_lock_destroy
expect:
1.no assert
2.al_os_lock_create returns non-null pointer
*/
static char *testcases_lock_create_and_destroy(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_lock *lock;

	lock = al_os_lock_create();
	ASSERT(NULL != lock);
	al_os_lock_destroy(lock);

	return NULL;
}

PFM_TEST_DESC_DEF(al_lock_create_and_destroy_func,
	pfm_tcase_make_order(PFM_TCASE_ORD_OS_LOCK, 1),
	pfm_tcase_make_name("lock", "C610326"), EXPECT_SUCCESS,
	NULL, testcases_lock_create_and_destroy, NULL);

/*
C610327 test al_os_lock_destroy with invalid parameters
expect:
1.no assert
*/

static void testcases_lock_destory_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_lock_arg *p_param;

	p_param = (struct al_lock_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_lock_destroy(%p) ...", p_param->lock);
	al_os_lock_destroy(p_param->lock);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_lock_destory_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_LOCK, 2),
		pfm_tcase_make_name("lock", "C610327"), 1, EXPECT_SUCCESS,
		NULL, &al_lock_arg_dft, sizeof(struct al_lock_arg),
		al_lock_invalid_combi_param, NULL,
		testcases_lock_destory_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_lock_arg, lock));

/*
C610328 test al_os_lock_lock and al_os_lock_unlock
expect:
1.no assert
2.al_os_lock_create returns non-null pointer
3.thread should be blocked for (TEST_LOCK_WAIT)
*/
#define TEST_LOCK_WAIT		(1000)
static void testcases_lock_lock_thread(struct al_thread *thread, void *arg)
{
	struct al_lock *lock;
	u32 ret;
	u64 start_tm, finish_tm;

	lock = (struct al_lock *)arg;
	if (NULL == lock) {
		ret = 0;
		goto on_exit;
	}
	start_tm = al_clock_get_total_ms();
	al_os_lock_lock(lock);
	al_os_lock_unlock(lock);
	finish_tm = al_clock_get_total_ms();
	ret = finish_tm - start_tm;

on_exit:
	al_os_thread_set_exit_code(thread, ret);
}
static char *testcases_lock_lock_mthread(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_thread *tid;
	static struct al_lock *test_lock;
	u32 rc;

	test_lock = al_os_lock_create();
	ASSERT(NULL != test_lock);
	al_os_lock_lock(test_lock);

	tid = al_os_thread_create("lock_test", NULL, (8*1024),
	    al_os_thread_pri_normal, testcases_lock_lock_thread,
	    test_lock);
	ASSERT(NULL != tid);

	/* wait for running test thread */
	test_printf("sleep for %u ms", TEST_LOCK_WAIT);
	al_os_thread_sleep(TEST_LOCK_WAIT);
	al_os_lock_unlock(test_lock);

	rc = al_os_thread_terminate_with_status(tid);
	test_printf("has locked for %u ms", rc);
	al_os_lock_destroy(test_lock);
	test_lock = NULL;

	ASSERT(rc >= (TEST_LOCK_WAIT - 10) && rc <= (TEST_LOCK_WAIT + 10));

	return NULL;
}

PFM_TEST_DESC_DEF(al_lock_in_same_thread,
	pfm_tcase_make_order(PFM_TCASE_ORD_OS_LOCK, 3),
	pfm_tcase_make_name("lock", "C610328"), EXPECT_SUCCESS,
	NULL, testcases_lock_lock_mthread, NULL);

/*
C610329 test al_os_lock_lock with invalid parameters
expect:
1.assert
*/

static void testcases_lock_lock_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_lock_arg *p_param;

	p_param = (struct al_lock_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_lock_lock(%p) ...", p_param->lock);
	al_os_lock_lock(p_param->lock);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_lock_lock_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_LOCK, 5),
		pfm_tcase_make_name("lock", "C610329"), 1, EXPECT_FAILURE,
		NULL, &al_lock_arg_dft, sizeof(struct al_lock_arg),
		al_lock_invalid_combi_param, NULL,
		testcases_lock_lock_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_lock_arg, lock));

/*
C610330 test al_os_lock_unlock with invalid parameters
expect:
1.assert
*/

static void testcases_lock_unlock_combi_param(struct pfm_tf_combi_param_info
		*info)
{
	struct al_lock_arg *p_param;

	p_param = (struct al_lock_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_lock_unlock(%p) ...", p_param->lock);
	al_os_lock_unlock(p_param->lock);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_lock_unlock_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_LOCK, 6),
		pfm_tcase_make_name("lock", "C610330"), 1, EXPECT_FAILURE,
		NULL, &al_lock_arg_dft, sizeof(struct al_lock_arg),
		al_lock_invalid_combi_param, NULL,
		testcases_lock_unlock_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_lock_arg, lock));
