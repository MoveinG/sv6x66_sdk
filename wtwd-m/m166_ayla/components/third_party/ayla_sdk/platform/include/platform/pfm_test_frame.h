/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_PFM_COMMON_TEST_FRAME_H__
#define __AYLA_PFM_COMMON_TEST_FRAME_H__

/**
 * @file
 * Ayla AL Test Framework.
 */

#include <al/al_ada_thread.h>
#include <platform/pfm_test_cases.h>

#define EXPECT_SUCCESS  0	/**< Success is expected */
#define EXPECT_FAILURE  1	/**< Failure is expected */

/**
 * Structure for test case variable.
 */
struct pfm_test_var {
	const char *name;
	char *buf;
	size_t buf_size;
};

/**
 * Structure for test description.
 */
struct pfm_test_desc {
	int order;		/**< It is sort key for test cases */
	const char *name;	/**< test-case name */
	short expect;		/**< EXPECT_SUCCESS or EXPECT_FAILURE */
	const struct pfm_test_var * const *test_var;
	/**< test case defined variable that can be changed interactively */
	char *(*entry)(const struct pfm_test_desc *pcase,
		int argc, char **argv); /**< entry of test case */
	void *arg;		/**< argument used by test entry */
};

#include <platform/pfm_test_desc.h>
#ifndef PFM_TEST_DESC_DEF
/**
 * Macro for test-case definition.
 *
 * On Linux platform, the definitions are put into a named data segment. the
 * pfm_test_cases_* to collect them into a list and sort it by order or name.
 * If the compiler doesn't support the named data segment, the pfm_test_cases_*
 * functions should collect the definitions from a manual prepared array,
 * it likes the following definition.
 *~~~~~~~~~~~~~~~~~
 *	extern const struct pfm_test_desc my_test_desc1;
 *	extern const struct pfm_test_desc my_test_desc2;
 *
 *	const struct pfm_test_desc * const all_test_cases[] = {
 *		&my_test_desc1,
 *		&my_test_desc2,
 *	};
 *~~~~~~~~~~~~~~~~~
 */
#define PFM_TEST_DESC_DEF(desc, order, name, expect, test_var, entry, arg) \
	const struct pfm_test_desc desc = { order, name, expect, test_var, \
		entry, arg }
#endif

/**
 * Macro for defining a parameter's information.
 *
 * \param _type is the struct type.The same as _type in
 * PFM_DEFINE_COMBI_PARAM_INFO.
 * \param _arg is variable name in the struct _type.
 */
#define PFM_COMBI_PARAM_INFO_ITEM(_type, _arg)			\
	{							\
		.offset = OFFSET_OF(struct _type, _arg),	\
		.size = sizeof(((struct _type *)0)->_arg),	\
	}

/**
 * Macro for defining a set of a parameter.
 *
 * \param _var the name of the parameter set.
 * \param _type the parameter struct type.
 * \param _cnt the number of value in the set.
 * \param ... the value list.
 */
#define PFM_COMBI_PARAM_VALS(_var, _type, _cnt, ...)	\
static const struct {					\
	int cnt;					\
	_type list[(_cnt)];				\
} (_var) = {						\
	.cnt = (_cnt),					\
	.list = { __VA_ARGS__ },			\
}

/**
 * Macro for combined parameter test-case definition.
 *
 * \param _var the name of the parameter set.
 * \param order the order number of test cases.
 * \param name of the test case
 * \param _cnt the number of value in the set.
 * \param _expect is it expect exception.
 *.\param _test_var is test case define variables.
 * \param _param is the base parameters to combine with _combi_param.
 * \param _size size of the structure for parameters.
 * \param _combi_param is an array of parameter data.
 * \param _init callback called before running test.
 * \param _test test callback.
 * \param _final callback called after running test.
 * \param ... the parameter info list.
 */
#define PFM_COMBI_PARAM_TEST_DESC_DEF(_var, order, name, _cnt,		\
		_expect, _test_var, _param, _size, _combi_param,	\
		_init, _test, _final, ...)				\
static struct {								\
	int expect_exception;						\
	const void *param;						\
	void (*tf_init)(struct pfm_tf_combi_param_info *info, int argc,	\
		char **argv);						\
	void (*tf_test)(struct pfm_tf_combi_param_info			\
		*info);							\
	void (*tf_final)(struct pfm_tf_combi_param_info			\
		*info);							\
	int num;							\
	int total_size;							\
	const void * const *combi_param;				\
	struct pfm_tf_combi_param_info infos[_cnt];			\
} (_var##_desc) = {							\
	.expect_exception = (_expect),					\
	.param = (_param),						\
	.tf_init = (_init),						\
	.tf_test = (_test),						\
	.tf_final = (_final),						\
	.num = (_cnt),							\
	.total_size = (_size),						\
	.combi_param = (_combi_param),					\
	.infos = { __VA_ARGS__ },					\
};									\
PFM_TEST_DESC_DEF(testcase_##_var##_desc,				\
	order,								\
	name,								\
	EXPECT_SUCCESS,							\
	_test_var,							\
	pfm_test_frame_combi_param_call,				\
	&(_var##_desc))

/**
 * Macro for mutil-thread test-case definition.
 * NOTICE!Calling ASSERT in tf_test is forbidden.
 * NOTICE!This framework only can print the first error message.
 *
 * \param _var the name of the parameter set.
 * \param order the order number of test cases.
 * \param name of the test case
 * \param _cnt the number of threads.
 * \param _expect is it expect exception.
 *.\param _test_var is test case define variables.
 * \param _param the default parameters.
 * \param _init callback called before running test.
 * \param _test test callback.
 * \param _final callback called after running test.
 */
#define PFM_MTHREAD_TEST_DESC_DEF(_var, order, name, _cnt,	\
		_expect, _test_var, _param, _size, _init, _test,\
		_final)						\
static struct {							\
	int expect_exception;					\
	const void *param;					\
	void (*tf_init)(struct pfm_tf_mthread_info *info,	\
		int argc, char **argv);				\
	void (*tf_test)(struct pfm_tf_mthread_info *info);	\
	void (*tf_final)(struct pfm_tf_mthread_info *info);	\
	int num;						\
	int total_size;						\
	int finished;						\
	char *result;						\
	struct pfm_tf_mthread_info				\
		mt_info[_cnt];					\
} (_var##_desc) = {						\
	.expect_exception = 0,					\
	.param = (_param),					\
	.tf_init = (_init),					\
	.tf_test = (_test),					\
	.tf_final = (_final),					\
	.num = (_cnt),						\
	.total_size = (_size),					\
};								\
PFM_TEST_DESC_DEF(testcase_##_var##_desc,			\
	order,							\
	name,							\
	EXPECT_SUCCESS,						\
	_test_var,						\
	pfm_test_frame_mthread_call,				\
	&(_var##_desc))

/**
 * Initilize the test frame.
 */
void pfm_test_frame_init(void);

/**
 * Finalize the test frame.
 */
void pfm_test_frame_final(void);

/**
 * Structure for combined parameters' information.
 */
struct pfm_tf_combi_param_info{
	void *param;		/**< temp buffer for parameter */
	void *desc;		/**< pointe to the pfm_tf_combi_param_desc */
	int offset;		/**< offset of variable in the structure */
	int size;		/**< size of variable */
};

/**
 * Structure for combined parameters' description.
 */
struct pfm_tf_combi_param_desc {
	int expect_exception;		/**< is it expect exception */
	const void *param;		/**< the normal parameters */

	void (*tf_init)(struct pfm_tf_combi_param_info *info, int argc,
		char **argv);		/**< called before testing */
	void (*tf_test)(struct pfm_tf_combi_param_info
		*info);			/**< called for testing */
	void (*tf_final)(struct pfm_tf_combi_param_info
		*info);			/**< called after testing */

	int num;			/**< number of parameters */
	int total_size;			/**< size of the structure for parameters */
	const void * const *combi_param;/**< an array of parameter data */
	struct pfm_tf_combi_param_info infos[1];/**< information of parameters */
};

/**
 * Checking combined parameter
 */
char *pfm_test_frame_combi_param_call(const struct pfm_test_desc *pcase,
	int argc, char **argv);

/**
 * Structure for test thread information.
 */
struct pfm_tf_mthread_info{
	char name[32];		/**< thread name */
	int id;			/**< id for mapping thread info */
	void *tid;		/**< thread handle */
	struct al_ada_callback tf_exit;	/**< used for exit */
	char *result;		/**< result, return zero when all thread
				return success, else return error string */
	void *desc;		/**< pointe to the pfm_tf_mthread_desc */
	void *param;		/**< pointe to private parameter */
};
/**
 * Structure for mutil-thread test's description.
 */
struct pfm_tf_mthread_desc {
	int expect_exception;	/**< is it expect exception */
	const void *param;	/**< test parameters */

	void (*tf_init)(struct pfm_tf_mthread_info *info, int argc,
		char **argv);	/**< called before testing */
	void (*tf_test)(struct pfm_tf_mthread_info
		*info);		/**< called for testing */
	void (*tf_final)(struct pfm_tf_mthread_info
		*info);		/**< called after testing */

	int num;		/**< number of threads */
	int total_size;		/**< size of the structure for parameters */
	int finished;		/**< number of finish thread */
	char *result;		/**< result, return zero when all thread
				return success, else return error string */

	struct pfm_tf_mthread_info mt_info[1];	/**< array of thread info */
};

/**
 * Run mutil-thread test
 */
char *pfm_test_frame_mthread_call(const struct pfm_test_desc *pcase,
		int argc, char **argv);

/**
 * finish mutil-thread test, return the result to the test framework
 */
void pfm_test_frame_mthread_error(struct pfm_tf_mthread_info *mthread_info,
		char *result);

/**
 * This function is used to tell the test frame that the current case is started
 * as an asynchronous operation. It is called by test-case function. After this
 * function is called, the current test-case function should return immediately.
 */
void pfm_test_case_async_start(void);

/**
 * This function is used to tell the test frame that the current asynchronous
 * test-case is finished. It is called by test-case entry.
 *
 * \param status is a pointer that points to a error message, NULL means
 * success.
 */
void pfm_test_case_async_finished(char *status);

/**
 * Function test_printf() is used by the test-framework to output test result
 * on console.
 */
void test_printf(const char *fmt, ...);

/* The following functions should be ported when the complier doesn't support
 * named data segment
 */

/**
 * Initialize the test cases
 */
void pfm_test_cases_init(void);

/**
 * Finalize the test cases
 */
void pfm_test_cases_final(void);

/**
 * Get the total amount of test cases.
 */
u32 pfm_test_cases_total(void);

/**
 * iterate the test cases.
 *
 * \param cur is the current test cases, NULL wil return the first test case.
 * \param by_name is no zero, the test cases are sorted by test order, otherwise
 * the test cases are sorted by name
 *
 * \return points to a test case, NULL is not more test case.
 */
const struct pfm_test_desc *pfm_test_cases_iter(
		const struct pfm_test_desc *cur, int by_name);

#endif /* __AYLA_PFM_COMMON_TEST_FRAME_H__ */
