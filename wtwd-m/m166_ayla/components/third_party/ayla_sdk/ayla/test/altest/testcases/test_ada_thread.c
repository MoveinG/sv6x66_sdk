/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <string.h>
#include <ayla/log.h>
#include <ayla/timer.h>
#include <al/al_err.h>
#include <al/al_ada_thread.h>
#include <al/al_clock.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

/* tca = Test Case Arguments */
struct tca_ada_cb_arg {
	struct al_ada_callback *cb;
	void (*func)(void *);
	void *arg;

	int var;
	struct al_thread *tid;
};

struct tca_ada_timer_arg {
	struct timer timer;
	unsigned long ms;

	u64 set_time;
};

/* TCID = Test Case Default Parameter */
static struct al_ada_callback test_ada_cb;
PFM_COMBI_PARAM_VALS(invalid_ptr, void *, 1, NULL);
PFM_COMBI_PARAM_VALS(ada_valid_cb, void *, 1, &test_ada_cb);
PFM_COMBI_PARAM_VALS(ada_valid_arg, void *, 2, &test_ada_cb, NULL);

#define TEST_ADA_THREAD_MAGIC_NUM	(0x12345678)

static void testcases_ada_callback_test_cb(void *arg)
{

}

static struct tca_ada_cb_arg al_ada_callback_init_dft = {
	.cb = &test_ada_cb,
	.func = testcases_ada_callback_test_cb,
	.arg = &al_ada_callback_init_dft,
};

/*
C610074 test al_ada_callback_init with normal parameters
expect:
1.no assert
*/
static const void *al_ada_cb_valid_param[] = {
	&ada_valid_cb,
	&ada_valid_arg,
};

static void testcases_ada_cb_init_checkparam(struct pfm_tf_combi_param_info
		*this)
{
	struct tca_ada_cb_arg *p_param;

	p_param = (struct tca_ada_cb_arg *)this->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_ada_callback_init(%p, %p, %p) ...",
	    p_param->cb, p_param->func, p_param->arg);
	al_ada_callback_init(p_param->cb, p_param->func, p_param->arg);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(al_ada_callback_init_normal_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 1),
		pfm_tcase_make_name("ada_thread", "C610074"), 2, EXPECT_SUCCESS,
		NULL, &al_ada_callback_init_dft, sizeof(struct tca_ada_cb_arg),
		al_ada_cb_valid_param,
		NULL,
		testcases_ada_cb_init_checkparam,
		NULL,
		PFM_COMBI_PARAM_INFO_ITEM(tca_ada_cb_arg, cb),
		PFM_COMBI_PARAM_INFO_ITEM(tca_ada_cb_arg, arg));

/*
C610075 test al_ada_callback_init with invalid parameters
expect:
1.assert
*/
static const void *al_ada_cb_invalid_param[] = {
	&invalid_ptr,
	&invalid_ptr,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(al_ada_callback_init_invalid_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 2),
		pfm_tcase_make_name("ada_thread", "C610075"), 2, EXPECT_FAILURE,
		NULL, &al_ada_callback_init_dft, sizeof(struct tca_ada_cb_arg),
		al_ada_cb_invalid_param,
		NULL,
		testcases_ada_cb_init_checkparam,
		NULL,
		PFM_COMBI_PARAM_INFO_ITEM(tca_ada_cb_arg, cb),
		PFM_COMBI_PARAM_INFO_ITEM(tca_ada_cb_arg, func));

/*
C610078 test al_ada_call in the same thread
expect:
1.al_ada_call returns immediately;
2.the callback should be called in main thread;
3.no assert
*/
static void testcases_ada_call_in_main_cb(void *arg)
{
	char *result;
	struct tca_ada_cb_arg *p_param;
	struct al_thread *tid;

	if (NULL == arg) {
		result = "arg is null";
		goto on_exit;
	}
	p_param = (struct tca_ada_cb_arg *)arg;

	tid = al_os_thread_self();
	if (tid != p_param->tid) {
		result = "thread is not the same";
		goto on_exit;
	}

	result = NULL;
on_exit:
	pfm_test_case_async_finished(result);
}

static struct tca_ada_cb_arg al_ada_call_in_main_dft = {
	.cb = &test_ada_cb,
	.func = testcases_ada_call_in_main_cb,
	.arg = &al_ada_call_in_main_dft,
};

static void testcases_ada_call_func_pre(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_cb_arg *p_param;

	p_param = (struct tca_ada_cb_arg *)pcase->arg;
	ASSERT(NULL != p_param);

	p_param->var = 0;
	p_param->tid = al_os_thread_self();
	al_ada_callback_init(p_param->cb, p_param->func, p_param);
}

static char *testcases_ada_call_in_main_thread(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_cb_arg *p_param;

	p_param = (struct tca_ada_cb_arg *)pcase->arg;
	ASSERT(p_param);
	testcases_ada_call_func_pre(pcase, argc, argv);

	al_ada_call(p_param->cb);

	pfm_test_case_async_start();
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_call_normal_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 3),
		pfm_tcase_make_name("ada_thread", "C610078"), EXPECT_SUCCESS,
		NULL, testcases_ada_call_in_main_thread,
		&al_ada_call_in_main_dft);

/*
C610079 test al_ada_call in another thread
expect:
1.al_ada_call returns immediately;
2.the callback should be called in main thread;
3.no assert
*/
static void testcases_ada_call_in_another_cb(void *arg)
{
	struct tca_ada_cb_arg *p_param;
	struct al_thread *tid;

	ASSERT(arg);
	p_param = (struct tca_ada_cb_arg *)arg;

	tid = al_os_thread_self();
	ASSERT(tid == p_param->tid);

	p_param->var = TEST_ADA_THREAD_MAGIC_NUM;
}

static struct tca_ada_cb_arg al_ada_call_in_another_dft = {
	.cb = &test_ada_cb,
	.func = testcases_ada_call_in_another_cb,
	.arg = &al_ada_call_in_another_dft,
};

static void testcases_ada_call_mthread_pre(struct pfm_tf_mthread_info
		*this, int argc, char **argv)
{
	struct tca_ada_cb_arg *p_param;

	p_param = (struct tca_ada_cb_arg *)this->param;
	ASSERT(NULL != p_param);

	p_param->var = 0;
	p_param->tid = al_os_thread_self();
	al_ada_callback_init(p_param->cb, p_param->func, p_param);
}
static void testcases_ada_call_in_another_thread(struct pfm_tf_mthread_info
		*this)
{
	struct tca_ada_cb_arg *p_param;
	char *result;

	p_param = (struct tca_ada_cb_arg *)this->param;
	if (NULL == p_param) {
		result = "argument fault";
		goto on_exit;
	}

	al_ada_call(p_param->cb);

	while (0 == p_param->var) {
		al_os_thread_sleep(100);
	}
	if (TEST_ADA_THREAD_MAGIC_NUM != p_param->var) {
		result = "got wrong result";
		goto on_exit;
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(this, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_ada_call_in_single_thread,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 4),
		pfm_tcase_make_name("ada_thread", "C610079"),
		1, EXPECT_SUCCESS, NULL, &al_ada_call_in_another_dft,
		sizeof(struct tca_ada_cb_arg), testcases_ada_call_mthread_pre,
		testcases_ada_call_in_another_thread, NULL);

/*
C610080 test al_ada_call with invalid parameters
expect:
1. assert
*/
static char *testcases_ada_call_invalid_cb(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	al_ada_call(NULL);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_call_invalid_cb_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 5),
		pfm_tcase_make_name("ada_thread", "C610080"), EXPECT_FAILURE,
		NULL, testcases_ada_call_invalid_cb, NULL);

/*
C610082 test al_ada_sync_call in the same thread
expect:
1.al_ada_sync_call blocks until the callback returns;
2.the callback should be called in main thread;
3.no assert
*/
static void testcases_ada_sync_call_in_main_cb(void *arg)
{
	struct tca_ada_cb_arg *p_param;
	struct al_thread *tid;

	if (NULL == arg) {
		return;
	}
	p_param = (struct tca_ada_cb_arg *)arg;

	tid = al_os_thread_self();
	if (tid != p_param->tid) {
		p_param->var = 0;
		return;
	}

	p_param->var = TEST_ADA_THREAD_MAGIC_NUM;
}

static struct tca_ada_cb_arg al_ada_sync_call_in_main_dft = {
	.cb = &test_ada_cb,
	.func = testcases_ada_sync_call_in_main_cb,
	.arg = &al_ada_sync_call_in_main_dft,
};

static char *testcases_ada_sync_call_in_main_thread(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_cb_arg *p_param;

	p_param = (struct tca_ada_cb_arg *)pcase->arg;
	ASSERT(p_param);
	testcases_ada_call_func_pre(pcase, argc, argv);

	al_ada_sync_call(p_param->cb);

	ASSERT(TEST_ADA_THREAD_MAGIC_NUM == p_param->var);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_sync_call_normal_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 7),
		pfm_tcase_make_name("ada_thread", "C610082"), EXPECT_SUCCESS,
		NULL, testcases_ada_sync_call_in_main_thread,
		&al_ada_sync_call_in_main_dft);

/*
C610083 test al_ada_sync_call in another thread
expect:
1.al_ada_sync_call blocks until the callback returns;
2.the callback should be called in main thread;
3.no assert
*/
static void testcases_ada_sync_call_in_another_cb(void *arg)
{
	struct tca_ada_cb_arg *p_param;
	struct al_thread *tid;

	if (NULL == arg) {
		return;
	}
	p_param = (struct tca_ada_cb_arg *)arg;

	tid = al_os_thread_self();
	if (tid != p_param->tid) {
		p_param->var = 0;
		return;
	}

	p_param->var = TEST_ADA_THREAD_MAGIC_NUM;
}

static struct tca_ada_cb_arg al_ada_sync_call_in_another_dft = {
	.cb = &test_ada_cb,
	.func = testcases_ada_sync_call_in_another_cb,
	.arg = &al_ada_sync_call_in_another_dft,
};

static void testcases_ada_sync_call_pre(struct pfm_tf_mthread_info
		*this, int argc, char **argv)
{
	struct tca_ada_cb_arg *p_param;

	p_param = (struct tca_ada_cb_arg *)this->param;
	ASSERT(NULL != p_param);

	p_param->var = 0;
	p_param->tid = al_os_thread_self();
	al_ada_callback_init(p_param->cb, p_param->func, p_param);
}
static void testcases_ada_sync_call_in_another_thread(struct pfm_tf_mthread_info
		*this)
{
	struct tca_ada_cb_arg *p_param;
	char *result;
	int count = 100;

	p_param = (struct tca_ada_cb_arg *)this->param;
	if (NULL == p_param) {
		result = "argument fault";
		goto on_exit;
	}

	al_ada_sync_call(p_param->cb);

	while (0 == p_param->var && count-- > 0) {
		al_os_thread_sleep(100);
	}
	if (TEST_ADA_THREAD_MAGIC_NUM != p_param->var) {
		result = "got wrong result";
		goto on_exit;
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(this, result);
}

PFM_MTHREAD_TEST_DESC_DEF(al_ada_sync_call_in_single_thread,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 8),
		pfm_tcase_make_name("ada_thread", "C610083"),
		1, EXPECT_SUCCESS, NULL, &al_ada_sync_call_in_another_dft,
		sizeof(struct tca_ada_cb_arg),
		testcases_ada_sync_call_pre,
		testcases_ada_sync_call_in_another_thread, NULL);

/*
C610084 test al_ada_sync_call with invalid parameters
expect:
1. assert
*/
static char *testcases_ada_sync_call_invalid_cb(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	al_ada_sync_call(NULL);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_sync_call_invalid_cb_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 9),
		pfm_tcase_make_name("ada_thread", "C610084"), EXPECT_FAILURE,
		NULL, testcases_ada_sync_call_invalid_cb, NULL);

/*
C610086 test al_ada_timer_set and al_ada_timer_cancel
expect:
1. no assert
2. callback will be called in time(<10ms)
*/
static struct tca_ada_timer_arg ada_timer_arg_dft = {
		.ms = 500,
};

static void testcases_ada_timer_func_handle(struct timer *timer)
{
	struct tca_ada_timer_arg *p_param;
	u64 cur_time;
	char *result = NULL;

	p_param = (struct tca_ada_timer_arg *)timer;
	if (NULL == p_param) {
		result = "invalid timer";
	}

	cur_time = al_clock_get_total_ms();
	if (cur_time - p_param->set_time > p_param->ms + 10) {
		result = "timeout";
	}
	al_ada_timer_cancel(timer);

	pfm_test_case_async_finished(result);
}

static void testcases_ada_timer_func_pre(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_timer_arg *p_param;

	p_param = (struct tca_ada_timer_arg *)pcase->arg;
	ASSERT(NULL != p_param);

	timer_init(&p_param->timer, testcases_ada_timer_func_handle);
}

static char *testcases_ada_timer_set_and_cancel(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_timer_arg *p_param;

	p_param = (struct tca_ada_timer_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	testcases_ada_timer_func_pre(pcase, argc, argv);

	pfm_test_case_async_start();

	p_param->set_time = al_clock_get_total_ms();
	al_ada_timer_set(&p_param->timer, p_param->ms);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_timer_set_and_cancel_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 11),
		pfm_tcase_make_name("ada_thread", "C610086"), EXPECT_SUCCESS,
		NULL, testcases_ada_timer_set_and_cancel, &ada_timer_arg_dft);

/*
C610088 test al_ada_timer_set with invalid parameters
expect:
1. assert
*/
struct tca_ada_timer_combi_arg {
	struct timer *timer;
	unsigned long ms;
};

static struct tca_ada_timer_combi_arg ada_timer_combi_arg_dft = {
		.timer = &ada_timer_arg_dft.timer,
		.ms = 500,

};

PFM_COMBI_PARAM_VALS(ada_timer_invalid_ms, int, 1, 1<<31);
static const void *al_ada_timer_set_combi_param[] = {
	&invalid_ptr,
	&ada_timer_invalid_ms,
};

static void testcases_ada_timer_set_combi_param_init(
		struct pfm_tf_combi_param_info *info, int argc, char **argv)
{
	struct tca_ada_timer_combi_arg *p_param;

	p_param = (struct tca_ada_timer_combi_arg *)info->param;
	ASSERT(NULL != p_param);

	if (NULL != p_param->timer) {
		timer_init(p_param->timer, testcases_ada_timer_func_handle);
	}
}

static void testcases_ada_timer_set_combi_param(struct pfm_tf_combi_param_info
		*this)
{
	struct tca_ada_timer_combi_arg *p_param;

	p_param = (struct tca_ada_timer_combi_arg *)this->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_ada_timer_set(%p, %d) ...",
	    p_param->timer, p_param->ms);
	al_ada_timer_set(p_param->timer, p_param->ms);
 }

PFM_COMBI_PARAM_TEST_DESC_DEF(al_ada_timer_set_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 12),
		pfm_tcase_make_name("ada_thread", "C610088"),
		2, EXPECT_FAILURE, NULL,
		&ada_timer_combi_arg_dft, sizeof(struct tca_ada_timer_arg),
		al_ada_timer_set_combi_param,
		testcases_ada_timer_set_combi_param_init,
		testcases_ada_timer_set_combi_param,
		NULL,
		PFM_COMBI_PARAM_INFO_ITEM(tca_ada_timer_combi_arg, timer),
		PFM_COMBI_PARAM_INFO_ITEM(tca_ada_timer_combi_arg, ms));

/*
C610090 test double call al_ada_timer_set
expect:
1. no assert
2. callback will be called in time(<10ms)
*/
static char *testcases_ada_timer_set_double_call(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_timer_arg *p_param;

	p_param = (struct tca_ada_timer_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	testcases_ada_timer_func_pre(pcase, argc, argv);

	pfm_test_case_async_start();

	p_param->set_time = al_clock_get_total_ms();
	al_ada_timer_set(&p_param->timer, 10000);
	al_ada_timer_set(&p_param->timer, p_param->ms);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_timer_set_double_call_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 14),
		pfm_tcase_make_name("ada_thread", "C610090"), EXPECT_SUCCESS,
		NULL, testcases_ada_timer_set_double_call, &ada_timer_arg_dft);

/*
C610091 test al_ada_timer_cancel when timer never be set
expect:
1. no assert
*/
static char *testcases_ada_timer_cancel_notset(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_timer_arg *p_param;

	p_param = (struct tca_ada_timer_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	testcases_ada_timer_func_pre(pcase, argc, argv);

	al_ada_timer_cancel(&p_param->timer);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_timer_cancel_notset_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 15),
		pfm_tcase_make_name("ada_thread", "C610091"), EXPECT_SUCCESS,
		NULL, testcases_ada_timer_cancel_notset, &ada_timer_arg_dft);

/*
C610092 test al_ada_timer_cancel with invalid parameters
expect:
1. assert
*/
static char *testcases_ada_timer_cancel_invalid_timer(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	al_ada_timer_cancel(NULL);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_timer_cancel_invalid_timer_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 16),
		pfm_tcase_make_name("ada_thread", "C610092"), EXPECT_FAILURE,
		NULL, testcases_ada_timer_cancel_invalid_timer, NULL);

/*
C610094 test double call al_ada_timer_cancel
expect:
1. no assert
*/
static char *testcases_ada_timer_cancel_double_call(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_timer_arg *p_param;

	p_param = (struct tca_ada_timer_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	testcases_ada_timer_func_pre(pcase, argc, argv);

	al_ada_timer_cancel(&p_param->timer);

	al_ada_timer_cancel(&p_param->timer);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_timer_cancel_double_call_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 18),
		pfm_tcase_make_name("ada_thread", "C610094"), EXPECT_SUCCESS,
		NULL, testcases_ada_timer_cancel_double_call,
		&ada_timer_arg_dft);

/*
C610095 test al_ada_timer_set after al_ada_timer_cancel is called
expect:
1. no assert
2. callback will be called in time(<10ms)
*/
static char *testcases_ada_timer_set_after_cancel(const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct tca_ada_timer_arg *p_param;

	p_param = (struct tca_ada_timer_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	testcases_ada_timer_func_pre(pcase, argc, argv);

	pfm_test_case_async_start();

	p_param->set_time = al_clock_get_total_ms();
	al_ada_timer_cancel(&p_param->timer);
	al_ada_timer_set(&p_param->timer, p_param->ms);
	return NULL;
}

PFM_TEST_DESC_DEF(al_ada_timer_set_after_cancel_desc,
		pfm_tcase_make_order(PFM_TCASE_ORD_ADA_THREAD, 19),
		pfm_tcase_make_name("ada_thread", "C610095"), EXPECT_SUCCESS,
		NULL, testcases_ada_timer_set_after_cancel,
		&ada_timer_arg_dft);
