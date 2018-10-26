/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <ayla/log.h>
#include <al/al_clock.h>
#include <al/al_err.h>
#include <al/al_os_thread.h>

#include <platform/pfm_test_cases.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_assert.h>

struct al_os_thread_arg {
	struct al_thread *thread;
	const char *name;
	void *stack;
	size_t stack_size;
	enum al_os_thread_pri pri;
	void (*thread_main)(struct al_thread *thread, void *arg);
	void *arg;
	u32 code;
	int ms;
};

#define TEST_OS_THREAD_NAME		"os_test"
#define TEST_OS_THREAD_STACKSIZE	(8 * 1024)
#define TEST_OS_THREAD_WAITTIME	(5000)
#define TEST_OS_THREAD_MAGIC		0x23456789
#define TEST_OS_THREAD_NUM		(2)

static char test_stack[TEST_OS_THREAD_STACKSIZE];
static int test_var;
static void testcases_thread_dft(struct al_thread *thread, void *arg)
{
	test_var = TEST_OS_THREAD_MAGIC;
}
static struct al_os_thread_arg al_os_thread_dft = {
	.thread = NULL,
	.name = TEST_OS_THREAD_NAME,
	.stack = NULL,
	.stack_size = 0,
	.pri = al_os_thread_pri_normal,
	.thread_main = testcases_thread_dft,
	.arg = 0,
	.code = 0,
	.ms = 0,
};

PFM_COMBI_PARAM_VALS(al_os_thread_invalid_ptr, void *, 1, NULL);
PFM_COMBI_PARAM_VALS(al_os_thread_valid_str, char *, 2,
		"", TEST_OS_THREAD_NAME);

PFM_COMBI_PARAM_VALS(al_os_thread_invalid_stacksize, size_t, 3, 1,
		(4 * 1024 - 1), (1024 * 1024 + 1));
PFM_COMBI_PARAM_VALS(al_os_thread_invalid_pri, enum al_os_thread_pri, 2,
		al_os_thread_pri_high - 1, al_os_thread_pri_low + 1);

static const void *os_thread_invalid_ptr_combi_param[] = {
	&al_os_thread_invalid_ptr,
};

static void testcases_os_thread_combi_param_main(struct al_thread *thread,
		void *arg)
{
	int count = 100;
	while (count-- && !al_os_thread_get_exit_flag(thread)) {
		test_var++;
		al_os_thread_sleep(10);
	}
	al_os_thread_set_exit_code(thread, TEST_OS_THREAD_MAGIC);
}

static void testcases_os_thread_init_combi_param(
		struct pfm_tf_combi_param_info *info, int argc, char **argv)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);
	test_var = 0;

	p_param->thread = al_os_thread_create(p_param->name, p_param->stack,
	    p_param->stack_size, p_param->pri,
	    testcases_os_thread_combi_param_main, p_param->arg);
	ASSERT(NULL != p_param->thread);
}
/*
C610342 test al_os_thread_create with normal parameters
expect:
1.no assert
2.al_os_thread_create return non-null pointer
3.var is set to the magic number
4.al_os_thread_terminate_with_status return zero as default when the thread
don't set the exit code.
*/
PFM_COMBI_PARAM_VALS(al_os_thread_valid_ptr, void *, 1, test_stack);
PFM_COMBI_PARAM_VALS(al_os_thread_valid_stacksize, size_t, 2, 4 * 1024,
		1024 * 1024);
PFM_COMBI_PARAM_VALS(al_os_thread_valid_pri, enum al_os_thread_pri, 3,
		al_os_thread_pri_high, al_os_thread_pri_normal,
		al_os_thread_pri_low);
static const void *os_thread_create_valid_combi_param[] = {
	&al_os_thread_valid_str,
	&al_os_thread_valid_ptr,
	&al_os_thread_valid_stacksize,
	&al_os_thread_valid_pri,
	&al_os_thread_valid_ptr,
};

static void testcases_os_thread_create_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;
	u32 rc;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);
	test_var = 0;

	if (p_param->name) {
		test_printf("testing al_os_thread_create(\"%s\", %p, %zd, %d, "
		    "%p) ...", p_param->name, p_param->stack,
		    p_param->stack_size, p_param->pri, p_param->arg);
	} else {
		test_printf("testing al_os_thread_create(NULL, %p, %zd, %d, "
		    "%p) ...", p_param->stack, p_param->stack_size,
		    p_param->pri, p_param->arg);
	}
	p_param->thread = al_os_thread_create(p_param->name, p_param->stack,
	    p_param->stack_size, p_param->pri, p_param->thread_main,
	    p_param->arg);
	ASSERT(NULL != p_param->thread);
	rc = al_os_thread_terminate_with_status(p_param->thread);
	ASSERT(0 == rc);
	ASSERT(TEST_OS_THREAD_MAGIC == test_var);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(os_thread_create_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 1),
		pfm_tcase_make_name("os_thread", "C610342"), 5, EXPECT_SUCCESS,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_create_valid_combi_param, NULL,
		testcases_os_thread_create_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, name),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, stack),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, stack_size),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, pri),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, arg));

/*
C610343 test al_os_thread_create with invalid parameters
expect:
1.assert
*/
static const void *os_thread_create_invalid_combi_param[] = {
	&al_os_thread_invalid_ptr,
	&al_os_thread_invalid_stacksize,
	&al_os_thread_invalid_pri,
	&al_os_thread_invalid_ptr,
};

PFM_COMBI_PARAM_TEST_DESC_DEF(os_thread_create_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 2),
		pfm_tcase_make_name("os_thread", "C610343"), 4, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_create_invalid_combi_param, NULL,
		testcases_os_thread_create_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, name),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, stack_size),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, pri),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread_main));

/*
C610344 test al_os_thread_join with normal parameters
expect:
1.no assert
2.al_os_thread_create return non-null pointer
3.al_os_thread_join block until thread is exit
4.exit code should match the return value of al_os_thread_join
*/
static const void *os_thread_join_valid_combi_param[] = {
	&al_os_thread_valid_pri,
};

static void testcases_os_thread_join_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;
	u32 rc;
	u64 start, end;
	int var;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);
	start = al_clock_get_total_ms();

	test_printf("testing al_os_thread_join(%p) ...", p_param->thread);
	rc = al_os_thread_join(p_param->thread);
	end = al_clock_get_total_ms();
	/* checking exit code */
	ASSERT(TEST_OS_THREAD_MAGIC == rc);
	/* checking block time of al_os_thread_join */
	ASSERT(900 <= (end - start) && 1200 >= (end - start));
	/* test_var will not increase */
	var = test_var;
	al_os_thread_sleep(100);
	ASSERT(var == test_var);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(os_thread_join_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 3),
		pfm_tcase_make_name("os_thread", "C610344"), 1, EXPECT_SUCCESS,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_join_valid_combi_param,
		testcases_os_thread_init_combi_param,
		testcases_os_thread_join_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, pri));

/*
C610345 test al_os_thread_join with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_join_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_join(%p) ...", p_param->thread);
	al_os_thread_join(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(os_thread_join_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 4),
		pfm_tcase_make_name("os_thread", "C610345"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_join_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610346 test al_os_thread_terminate with normal parameters
expect:
1.no assert
2.al_os_thread_terminate should return immediately
*/
static const void *os_thread_terminate_valid_combi_param[] = {
	&al_os_thread_valid_pri,
};

static void testcases_os_thread_terminate_valid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;
	u64 start, end;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);
	start = al_clock_get_total_ms();

	test_printf("testing al_os_thread_terminate(%p) ...", p_param->thread);
	al_os_thread_terminate(p_param->thread);
	end = al_clock_get_total_ms();
	/* checking block time of al_os_thread_terminate */
	test_printf("spend(%llu, %llu) ...", end, start);
	ASSERT(50 >= (end - start));

	/* wait the test thread exit before run the next test case */
	al_os_thread_join(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(os_thread_terminate_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 5),
		pfm_tcase_make_name("os_thread", "C610346"), 1, EXPECT_SUCCESS,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_terminate_valid_combi_param,
		testcases_os_thread_init_combi_param,
		testcases_os_thread_terminate_valid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, pri));

/*
C610347 test al_os_thread_terminate with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_terminate_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_terminate(%p) ...", p_param->thread);
	al_os_thread_terminate(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(os_thread_terminate_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 6),
		pfm_tcase_make_name("os_thread", "C610347"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_terminate_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610348 test al_os_thread_terminate_with_status with normal parameters
expect:
1.no assert
2.exit code should match the return value of al_os_thread_terminate_with_status
3.al_os_thread_terminate_with_status should block until thread exit
*/
static const void *os_thread_terminate_with_status_valid_combi_param[] = {
	&al_os_thread_valid_pri,
};

static void testcases_os_thread_terminate_with_status_main(
		struct al_thread *thread, void *arg)
{
	int count = 100;
	while (count--) {
		test_var++;
		al_os_thread_sleep(10);
	}
	al_os_thread_set_exit_code(thread, TEST_OS_THREAD_MAGIC);
}

static void testcases_os_thread_terminate_with_status_init_combi_param(
		struct pfm_tf_combi_param_info *info, int argc, char **argv)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);
	test_var = 0;

	p_param->thread = al_os_thread_create(p_param->name, p_param->stack,
	    p_param->stack_size, p_param->pri,
	    testcases_os_thread_terminate_with_status_main,
	    p_param->arg);
	ASSERT(NULL != p_param->thread);
}
static void testcases_os_thread_terminate_with_status_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;
	u32 rc;
	u64 start, end;
	int var;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);
	start = al_clock_get_total_ms();

	test_printf("testing al_os_thread_terminate_with_status(%p) ...",
			p_param->thread);
	rc = al_os_thread_terminate_with_status(p_param->thread);
	end = al_clock_get_total_ms();
	/* checking exit code */
	ASSERT(TEST_OS_THREAD_MAGIC == rc);
	/* checking block time of al_os_thread_terminate_with_status */
	ASSERT(900 <= (end - start) && 1200 >= (end - start));
	/* test_var will not increase */
	var = test_var;
	al_os_thread_sleep(100);
	ASSERT(var == test_var);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_terminate_with_status_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 7),
		pfm_tcase_make_name("os_thread", "C610348"), 1, EXPECT_SUCCESS,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_terminate_with_status_valid_combi_param,
		testcases_os_thread_terminate_with_status_init_combi_param,
		testcases_os_thread_terminate_with_status_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, pri));

/*
C610349 test al_os_thread_terminate_with_status with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_terminate_with_status_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_terminate_with_status(%p) ...",
			p_param->thread);
	al_os_thread_terminate_with_status(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_terminate_with_status_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 8),
		pfm_tcase_make_name("os_thread", "C610349"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_terminate_with_status_invalid_combi_param,
		NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610354 test al_os_thread_suspend in thread_main
expect:
1.no assert
2.al_os_thread_create return non-null pointer
3.thread block after calling al_os_thread_suspend
*/

static void testcases_os_thread_suspend_main(struct al_thread *thread,
		void *arg)
{
	int count = 100;

	al_os_thread_suspend(thread);
	while (count-- && !al_os_thread_get_exit_flag(thread)) {
		test_var++;
		al_os_thread_sleep(10);
	}
	al_os_thread_set_exit_code(thread, TEST_OS_THREAD_MAGIC);
}

static char *testcases_os_thread_suspend_in_thread_main(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_os_thread_arg *p_param;
	int var;
	u32 rc;

	p_param = (struct al_os_thread_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	test_var = 0;

	p_param->thread = al_os_thread_create(p_param->name, p_param->stack,
	    p_param->stack_size, p_param->pri,
	    testcases_os_thread_suspend_main, p_param->arg);
	ASSERT(NULL != p_param->thread);

	/* test_var will not increase */
	var = test_var;
	al_os_thread_sleep(100);
	ASSERT(var == test_var);

	al_os_thread_resume(p_param->thread);

	/* test_var will increase */
	var = test_var;
	al_os_thread_sleep(100);
	ASSERT(var != test_var);

	/* exit */
	rc = al_os_thread_terminate_with_status(p_param->thread);
	ASSERT(TEST_OS_THREAD_MAGIC == rc);
	return NULL;
}

PFM_TEST_DESC_DEF(os_thread_suspend_in_thread_main, PFM_TCASE_ORD_MANUAL,
	pfm_tcase_make_name("os_thread", "C610354"), EXPECT_SUCCESS,
	NULL, testcases_os_thread_suspend_in_thread_main, &al_os_thread_dft);

/*
C610355 test al_os_thread_suspend calling in another thread
expect:
1.no assert
2.al_os_thread_create return non-null pointer
3.thread block after calling al_os_thread_suspend
*/

static char *testcases_os_thread_suspend_in_another_main(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_os_thread_arg *p_param;
	int var;
	u32 rc;

	p_param = (struct al_os_thread_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	test_var = 0;

	p_param->thread = al_os_thread_create(p_param->name, p_param->stack,
	    p_param->stack_size, p_param->pri,
	    testcases_os_thread_combi_param_main, p_param->arg);
	ASSERT(NULL != p_param->thread);

	al_os_thread_suspend(p_param->thread);

	/* test_var will not increase */
	var = test_var;
	al_os_thread_sleep(100);
	ASSERT(var == test_var);

	al_os_thread_resume(p_param->thread);

	/* test_var will increase */
	var = test_var;
	al_os_thread_sleep(100);
	ASSERT(var != test_var);

	/* exit */
	rc = al_os_thread_terminate_with_status(p_param->thread);
	ASSERT(TEST_OS_THREAD_MAGIC == rc);
	return NULL;
}

PFM_TEST_DESC_DEF(os_thread_suspend_in_another_main, PFM_TCASE_ORD_MANUAL,
	pfm_tcase_make_name("os_thread", "C610355"), EXPECT_SUCCESS,
	NULL, testcases_os_thread_suspend_in_another_main, &al_os_thread_dft);

/*
C610357 test al_os_thread_suspend with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_suspend_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_suspend(%p) ...", p_param->thread);
	al_os_thread_suspend(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_suspend_invalid_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("os_thread", "C610357"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_suspend_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610358 test al_os_thread_resume with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_resume_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_resume(%p) ...", p_param->thread);
	al_os_thread_resume(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_resume_invalid_combi_param,
		PFM_TCASE_ORD_MANUAL,
		pfm_tcase_make_name("os_thread", "C610358"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_resume_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610359 test al_os_thread_set_priority and al_os_thread_get_priority in
thread_main
expect:
1.no assert
2.al_os_thread_create return non-null pointer
3. al_os_thread_set_priority match that returned by al_os_thread_get_priority
*/

static void testcases_os_thread_set_get_main(struct al_thread *thread,
		void *arg)
{
	enum al_os_thread_pri get_pri, set_pri;

	get_pri = al_os_thread_get_priority(thread);
	set_pri = get_pri + 1;
	set_pri %= 3;

	al_os_thread_set_priority(thread, set_pri);

	get_pri = al_os_thread_get_priority(thread);
	if (get_pri == set_pri) {
		al_os_thread_set_exit_code(thread, TEST_OS_THREAD_MAGIC);
	} else {
		al_os_thread_set_exit_code(thread, 0);
	}
}

static char *testcases_os_thread_set_get_in_thread_main(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_os_thread_arg *p_param;
	u32 rc;

	p_param = (struct al_os_thread_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	test_var = 0;

	p_param->thread = al_os_thread_create(p_param->name, p_param->stack,
	    p_param->stack_size, p_param->pri,
	    testcases_os_thread_set_get_main, p_param->arg);
	ASSERT(NULL != p_param->thread);

	rc = al_os_thread_terminate_with_status(p_param->thread);
	ASSERT(TEST_OS_THREAD_MAGIC == rc);
	return NULL;
}

PFM_TEST_DESC_DEF(os_thread_set_get_in_thread_main,
	pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 13),
	pfm_tcase_make_name("os_thread", "C610359"), EXPECT_SUCCESS,
	NULL, testcases_os_thread_set_get_in_thread_main, &al_os_thread_dft);

/*
C610360 test al_os_thread_set_priority and al_os_thread_get_priority
in another thread
expect:
1.no assert
2.al_os_thread_create return non-null pointer
3. al_os_thread_set_priority match that returned by al_os_thread_get_priority
*/

static char *testcases_os_thread_set_get_in_another_main(
		const struct pfm_test_desc
		*pcase, int argc, char **argv)
{
	struct al_os_thread_arg *p_param;
	enum al_os_thread_pri get_pri, set_pri;
	u32 rc;

	p_param = (struct al_os_thread_arg *)pcase->arg;
	ASSERT(NULL != p_param);
	test_var = 0;

	p_param->thread = al_os_thread_create(p_param->name, p_param->stack,
	    p_param->stack_size, p_param->pri,
	    testcases_os_thread_combi_param_main, p_param->arg);
	ASSERT(NULL != p_param->thread);

	get_pri = al_os_thread_get_priority(p_param->thread);
	set_pri = get_pri + 1;
	set_pri %= 3;

	al_os_thread_set_priority(p_param->thread, set_pri);

	get_pri = al_os_thread_get_priority(p_param->thread);
	ASSERT(get_pri == set_pri);

	rc = al_os_thread_terminate_with_status(p_param->thread);
	ASSERT(TEST_OS_THREAD_MAGIC == rc);
	return NULL;
}

PFM_TEST_DESC_DEF(os_thread_set_get_in_another_main,
	pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 14),
	pfm_tcase_make_name("os_thread", "C610360"), EXPECT_SUCCESS,
	NULL, testcases_os_thread_set_get_in_another_main, &al_os_thread_dft);

/*
C610361 test al_os_thread_get_priority with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_get_priority_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_get_priority(%p) ...",
	    p_param->thread);
	al_os_thread_get_priority(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_get_priority_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 15),
		pfm_tcase_make_name("os_thread", "C610361"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_get_priority_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610362 test al_os_thread_set_priority with invalid parameters
expect:
1.assert
*/
/* arg = NULL means thread should be valid, else thread should be null */
PFM_COMBI_PARAM_VALS(al_os_thread_arg, void *, 1, (void *)1);
static const void *os_thread_set_priority_invalid_combi_param[] = {
	&al_os_thread_arg,
	&al_os_thread_invalid_pri,
};

static void testcases_os_thread_set_priority_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	if (NULL == p_param->arg) {
		p_param->thread = al_os_thread_self();
	} else {
		p_param->thread = NULL;
	}
	test_printf("testing al_os_thread_set_priority(%p, %d) ...",
	    p_param->thread, p_param->pri);
	al_os_thread_set_priority(p_param->thread, p_param->pri);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_set_priority_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 16),
		pfm_tcase_make_name("os_thread", "C610362"), 2, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_set_priority_invalid_combi_param, NULL,
		testcases_os_thread_set_priority_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, arg),
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, pri));

/*
C610369 test al_os_thread_set_exit_code with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_set_exit_code_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_set_exit_code(%p, %d) ...",
	    p_param->thread, p_param->code);
	al_os_thread_set_exit_code(p_param->thread, p_param->code);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_set_exit_code_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 17),
		pfm_tcase_make_name("os_thread", "C610369"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_set_exit_code_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610372 test al_os_thread_get_exit_flag with invalid parameters
expect:
1.assert
*/
static void testcases_os_thread_get_exit_flag_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_get_exit_flag(%p) ...",
	    p_param->thread);
	al_os_thread_get_exit_flag(p_param->thread);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_get_exit_flag_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 18),
		pfm_tcase_make_name("os_thread", "C610372"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_invalid_ptr_combi_param, NULL,
		testcases_os_thread_get_exit_flag_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, thread));

/*
C610379 test al_os_thread_self
expect:
1.no assert
2.the thread returned by al_os_thread_self match these thread handles
*/
static void testcases_os_thread_self_mthread(
		struct pfm_tf_mthread_info *info)
{
	struct al_thread *thread;
	char *result;

	thread = al_os_thread_self();
	test_printf("thread(%p), info->tid(%p) ...",
			thread, info->tid);
	if (thread != info->tid) {
		result = "thread does not match";
		goto on_exit;
	}

	result = NULL;
on_exit:
	pfm_test_frame_mthread_error(info, result);
}

PFM_MTHREAD_TEST_DESC_DEF(os_thread_self_mthread,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 19),
		pfm_tcase_make_name("os_thread", "C610379"),
		TEST_OS_THREAD_NUM, EXPECT_SUCCESS, NULL, &al_os_thread_dft,
		sizeof(struct al_os_thread_arg), NULL,
		testcases_os_thread_self_mthread, NULL);

/*
C610380 test al_os_thread_sleep with normal parameters
expect:
1.no assert
2.sleep time match the used time
*/

PFM_COMBI_PARAM_VALS(al_os_thread_valid_tm, int, 3,
		0, 1, TEST_OS_THREAD_WAITTIME);
static const void *os_thread_sleep_valid_combi_param[] = {
	&al_os_thread_valid_tm,
};
static void testcases_os_thread_sleep_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;
	u64 start, end, use_tm;
	u64 low_limit, high_limit;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_sleep(%d) ...",
	    p_param->ms);
	start = al_clock_get_total_ms();
	al_os_thread_sleep(p_param->ms);
	end = al_clock_get_total_ms();
	use_tm = end - start;

	low_limit = (p_param->ms < 10) ? (0) : (p_param->ms - 10);
	high_limit = (p_param->ms < 1000) ? (p_param->ms + 10)
	    : (p_param->ms + 50);
	ASSERT(low_limit <= use_tm && high_limit >= use_tm);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_sleep_valid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 20),
		pfm_tcase_make_name("os_thread", "C610380"), 1, EXPECT_SUCCESS,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_sleep_valid_combi_param, NULL,
		testcases_os_thread_sleep_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, ms));

/*
C610381 test al_os_thread_sleep with invalid parameters
expect:
1.assert
*/
PFM_COMBI_PARAM_VALS(al_os_thread_invalid_tm, int, 1, -1);
static const void *os_thread_sleep_invalid_combi_param[] = {
	&al_os_thread_invalid_tm,
};
static void testcases_os_thread_sleep_invalid_combi_param(
		struct pfm_tf_combi_param_info *info)
{
	struct al_os_thread_arg *p_param;

	p_param = (struct al_os_thread_arg *)info->param;
	ASSERT(NULL != p_param);

	test_printf("testing al_os_thread_sleep(%p) ...",
	    p_param->thread);
	al_os_thread_sleep(p_param->ms);
}

PFM_COMBI_PARAM_TEST_DESC_DEF(
		os_thread_os_thread_sleep_invalid_combi_param,
		pfm_tcase_make_order(PFM_TCASE_ORD_OS_THREAD, 21),
		pfm_tcase_make_name("os_thread", "C610381"), 1, EXPECT_FAILURE,
		NULL, &al_os_thread_dft, sizeof(struct al_os_thread_arg),
		os_thread_sleep_invalid_combi_param, NULL,
		testcases_os_thread_sleep_invalid_combi_param, NULL,
		PFM_COMBI_PARAM_INFO_ITEM(al_os_thread_arg, ms));
