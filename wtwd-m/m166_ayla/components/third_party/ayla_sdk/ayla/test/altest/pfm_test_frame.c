/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <time.h>
#include <stdarg.h>
#include <malloc.h>

#include <ayla/utypes.h>
#include <ayla/clock.h>
#include <ayla/conf.h>
#include <ayla/log.h>
#include <ayla/mod_log.h>
#include <al/al_ada_thread.h>
#include <al/al_cli.h>
#include <al/al_clock.h>
#include <al/al_err.h>
#include <al/al_log.h>
#include <al/al_os_mem.h>
#include <al/al_os_thread.h>
#include <al/al_persist.h>
#include <platform/pfm_assert.h>
#include <platform/pfm_test_frame.h>
#include <platform/pfm_pwb_linux.h>

#define COMBI_PARAM_SIZE_MAX 128
#define MAX_TEST_VAR 30

#define TEST_VAR_NAME "testvar"
#define TF_TLV_BUF_SIZE 512
struct test_var {
	struct test_var *next;
	const char *name;
	char *buf;
	size_t buf_size;
};

struct test_cntx {
	int  repeat_max; /* Max number of test round */
	u16  flag_test_all:1;
	u16  ignore_fail:1;
	u16  stop_on_fail:1;
	u16  flag_async_test:1;
	int  inp_name_num;

	int  argc_ext;
	char **argv_ext; /* allocated */
	char *arg_buf; /* allocated */

	int  count_total;
	int  count_tested;
	int  count_passed;
	int  count_skipped;

	/* Improvement: use ada_call() to do test one by one */
	int  repeat_cnt; /* Counter of test round */
	int  test_cnt; /* Counter of test in a round */
	const struct pfm_test_desc *p_case; /* Current test-case */
	struct al_ada_callback cb;

	int  idx_max;
	int  idx;	/* index of name */
	char **names;	/* allocated */
	char *name_buf; /* allocated */

	struct timespec t1;
	struct timespec t2;
};

struct test_var_cntx {
	struct test_var *var_list;
	struct test_var var_nodes[MAX_TEST_VAR];
	int var_free_pos;
};

/* Local globals */
static struct test_cntx gv;
static struct test_var_cntx gvc;
static char gs_fail_msg[128];

void pfm_try_catch_final(void)
{
	if (gv_jmp_buf_top > 0) {
		--gv_jmp_buf_top;
	}
}

int assert_jmpbuf_stack_overflow(void)
{
	test_printf("");
	test_printf("Stack pfm_try_catch_jmp_buf[] overflow !\n");
	pfm_pwb_linux_reboot_flag = 0;
	longjmp(pfm_try_catch_jmp_buf[0], 1);
	return -1;
}

/*
 * Here al_assert_handle() is test-mode version. If this function is
 * defined in application level, then the function with the same name
 * defined in al_assert.c is not linked. The test-mode version of
 * al_assert_handle() is used to trap assertion, and to tell the
 * test-framework that an assertion occurs in which file and which line.
*/
void al_assert_handle(const char *file, int line)
{
	snprintf(gs_fail_msg, sizeof(gs_fail_msg),
		"assert in (%s, %d)", file, line);
	longjmp(pfm_try_catch_jmp_buf[gv_jmp_buf_top], 1);
}

static int is_digit_str(const char *str)
{
	for (; *str; ++str) {
		if (!isdigit(*str)) {
			return 0;
		}
	}
	return 1;
}

/* TO DO: Re-implement get_time() and time_diff() if AL system time
 * interfaces are defined.
 */
static void get_time(struct timespec *tv)
{
	clock_gettime(CLOCK_REALTIME, tv);
}

static int time_diff(struct timespec *tv2, struct timespec *tv1)
{
	int t = (tv2->tv_sec - tv1->tv_sec) * 1000 +
		(tv2->tv_nsec - tv1->tv_nsec)/1000000;
	return t;
}

static const struct pfm_test_desc *pfm_altest_find_case(const char *name);

static int  pfm_altest_parse_arg(int argc, char **argv)
{
	int  err_cnt = 0;
	int  i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--repeat") == 0) {
			argv[i] = NULL;
			if (++i >= argc) {
				continue;
			}
			if (is_digit_str(argv[i])) {
				gv.repeat_max = atoi(argv[i]);
			} else {
				printcli("Error: Expecting number "
				    "behind \"--repeat\".\n");
				err_cnt++;
			}
		} else if (strcmp(argv[i], "--stop_on_fail") == 0) {
			gv.stop_on_fail = 1;
		} else if (strcmp(argv[i], "--") == 0) {
			gv.argc_ext = argc - i;
			gv.argv_ext = &argv[i];
			break;
		} else if (!memcmp(argv[i], "--", 2)) {
			printcli("Error: Unknown option \"%s\".\n", argv[i]);
			err_cnt++;;
		} else {
			if (strcmp(argv[i], "all") == 0) {
				gv.flag_test_all = 1;
			} else if (!pfm_altest_find_case(argv[i])) {
				printcli("Error: Name or order \"%s\" not "
				    "found in test-cases.\n", argv[i]);
				err_cnt++;;
			} else {
				gv.inp_name_num++;
				continue;
			}
		}
		argv[i] = NULL;
	}

	if (gv.flag_test_all) {
		gv.count_total = pfm_test_cases_total();
	} else {
		gv.count_total = gv.inp_name_num;
	}
	gv.count_total *= gv.repeat_max;
	gv.count_tested = 0;
	gv.count_passed = 0;
	gv.count_skipped = 0;
	if (err_cnt) {
		printcli("\nUsage:\n"
		    "altest all\n"
		    "altest <test-name> | <test-order> [...]\n"
		    "\t[--repeat <count>]\n"
		    "\t[--stop_on_fail]\n"
		    "\t[-- <case-specific-parameters>]\n");
	}
	return err_cnt;
}

static void pfm_altest_show_case_result(const struct pfm_test_desc *p_case,
	int pass, const char *status, long t)
{
	test_printf("@Result: %s, %ld ms, %s", pass ? "PASS" : "FAIL", t,
	    p_case->name);
	if (status) {
		if (p_case->expect && pass) {
			test_printf("Info: Expected %s", status);
		} else {
			test_printf("Info: %s", status);
		}
	}
	test_printf("@");
}

static void pfm_altest_show_result(void)
{
	int total = gv.count_total;
	if (gv.flag_test_all) {
		total -= gv.count_skipped;
	}
	test_printf("@Result: Total = %d,  Tested = %d,  Passed = %d,  "
	    "Failed = %d.", total, gv.count_tested,
	    gv.count_passed, gv.count_tested - gv.count_passed);
}

static int pfm_altest_the_case(const struct pfm_test_desc *p_case,
	int argc, char **argv)
{
	int ret;
	const char *p;
	long t;
	int pass;

	gv.count_tested++;
	get_time(&gv.t1);
	ret = pfm_try_catch_assert();
	if (ret == 0) {
		test_printf("@Testing %s [0x%04x]...", p_case->name,
		    p_case->order);
		p = p_case->entry(p_case, argc,	argv);
	} else {
		p = (char *)gs_fail_msg;
	}
	pfm_try_catch_final();
	if (p) {
		gv.flag_async_test = 0;
	}

	if (!gv.flag_async_test) {
		pass = (!p && !p_case->expect) ||
			(p && p_case->expect);
		get_time(&gv.t2);
		t = time_diff(&gv.t2, &gv.t1);
		pfm_altest_show_case_result(p_case, pass, p, t);
		if (pass) {
			gv.count_passed++;
		}
		if (gv.stop_on_fail && !pass) {
			return 1;
		}
	} else {
		if (p && gv.stop_on_fail) {
			return 1;
		}
	}
	return 0;
}

static void pfm_altest_all_next(void *arg)
{
	if (gv.test_cnt == 0) {
		test_printf("------TEST all (Repeat %d/%d)----------",
			gv.repeat_cnt + 1, gv.repeat_max);
	}
	gv.p_case = pfm_test_cases_iter(gv.p_case, 0);
	if (gv.p_case) {
		gv.test_cnt++;
		if (gv.p_case->order < 0) {
			/* Skip internal test case */
			gv.count_skipped++;
			al_ada_call(&gv.cb);
			return;
		}
		/* next test-case in the current round */
		if (pfm_altest_the_case(gv.p_case, gv.argc_ext, gv.argv_ext)) {
			goto end; /* stop */
		}
	} else {
		/* next round of test */
		gv.test_cnt = 0;
		gv.repeat_cnt += 1;
		if (gv.repeat_cnt >= gv.repeat_max) {
			goto end; /* finished */
		}
	}
	if (!gv.flag_async_test) {
		al_ada_call(&gv.cb);
	}
	return;
end:
	if (!gv.flag_async_test) {
		pfm_altest_show_result();
		if (gv.arg_buf) {
			al_os_mem_free(gv.arg_buf);
			gv.arg_buf = NULL;
		}
		if (gv.argv_ext) {
			al_os_mem_free(gv.argv_ext);
			gv.argv_ext = NULL;
		}
	}
}

static void pfm_altest_all(void)
{
	gv.repeat_cnt = 0;
	gv.test_cnt = 0;
	gv.p_case = NULL;
	al_ada_callback_init(&gv.cb, pfm_altest_all_next, NULL);
	al_ada_call(&gv.cb);
}

static const struct pfm_test_desc *pfm_altest_find_case(const char *name)
{
	int is_name;
	int order;
	const struct pfm_test_desc *p_case;

	if (!name || name[0] == '\0') {
		return NULL;
	}

	is_name = isalpha(name[0]);
	order = (!is_name) ? atoi(name) : 0;
	p_case = NULL;
	while (NULL != (p_case = pfm_test_cases_iter(p_case, 0))) {
		if (is_name) {
			if (!strcmp(name, p_case->name)) {
				break;
			}
		} else {
			if (order == p_case->order) {
				break;
			}
		}
	}
	return p_case;
}

static void pfm_altest_byname_next(void *arg)
{
again:
	if (gv.repeat_cnt >= gv.repeat_max) {
		goto end;
	}
	if (gv.idx == 1) {
		test_printf("------TEST by name (Repeat %d/%d)------",
			gv.repeat_cnt + 1, gv.repeat_max);
	}
	if (!gv.names[gv.idx]) {
		gv.idx += 1;
		goto again;
	}
	if (gv.idx >= gv.idx_max || gv.test_cnt >= gv.inp_name_num) {
		gv.test_cnt = 0;
		gv.idx = 1;
		gv.repeat_cnt += 1;
		goto again;
	}

	gv.p_case = pfm_altest_find_case(gv.names[gv.idx]);
	gv.idx++;
	if (!gv.p_case) {
		test_printf("\tTest case (%s, %d) not found.",
			gv.p_case->name, gv.p_case->order);
		goto again;
	}
	gv.test_cnt++;
	if (pfm_altest_the_case(gv.p_case, gv.argc_ext, gv.argv_ext)) {
		goto end; /* stop */
	}
	if (!gv.flag_async_test) {
		al_ada_call(&gv.cb);
	}
	return;
end:
	if (!gv.flag_async_test) {
		pfm_altest_show_result();
		if (gv.names) {
			al_os_mem_free(gv.names);
			gv.names = NULL;
		}
		if (gv.name_buf) {
			al_os_mem_free(gv.name_buf);
			gv.name_buf = NULL;
		}
		if (gv.arg_buf) {
			al_os_mem_free(gv.arg_buf);
			gv.arg_buf = NULL;
		}
		if (gv.argv_ext) {
			al_os_mem_free(gv.argv_ext);
			gv.argv_ext = NULL;
		}
	}
}

static void pfm_altest_by_name(int idx_max, char **names)
{
	int len;
	int i;
	int n;

	gv.repeat_cnt = 0;
	gv.p_case = NULL;
	gv.idx_max = idx_max;
	gv.idx = 1;
	gv.test_cnt = 0;

	for (len = 0, i = 0; i < idx_max; i++) {
		if (!names[i])
			continue;
		len += strlen(names[i]) + 2;
	}
	gv.names = al_os_mem_calloc((idx_max + 1) * (sizeof(char *)));
	gv.name_buf = al_os_mem_calloc(len);
	for (n = 0, i = 0; i < idx_max; i++) {
		if (!names[i])
			continue;
		strcpy(gv.name_buf + n, names[i]);
		gv.names[i] = gv.name_buf + n;
		n += strlen(names[i]) + 1;
	}
	al_ada_callback_init(&gv.cb, pfm_altest_byname_next, NULL);
	al_ada_call(&gv.cb);
}

static void pfm_test_insert_variable(const struct pfm_test_var *var)
{
	struct test_var **vars;
	int cmp_result;

	ASSERT(NULL != var->name && '\0' != var->name[0]);

	for (vars = &gvc.var_list; *vars; vars = &(*vars)->next) {
		cmp_result = strcmp(var->name, (*vars)->name);
		if (cmp_result == 0) {
			if (var->buf == (*vars)->buf &&
			    var->buf_size == (*vars)->buf_size) {
				return;
			}
			/* Duplicate variable name */
			ASSERT_NOTREACHED();
		} else if (cmp_result < 0) {
			break;
		}
	}

	ASSERT(gvc.var_free_pos < ARRAY_LEN(gvc.var_nodes));
	gvc.var_nodes[gvc.var_free_pos].name = var->name;
	gvc.var_nodes[gvc.var_free_pos].buf = var->buf;
	gvc.var_nodes[gvc.var_free_pos].buf_size = var->buf_size;
	gvc.var_nodes[gvc.var_free_pos].next = *vars;
	*vars = &gvc.var_nodes[gvc.var_free_pos++];
}

static int pfm_test_set_test_var(const char *name, const char *val)
{
	struct test_var *var;

	var = gvc.var_list;
	while (var) {
		if (strcasecmp(name, var->name) == 0) {
			strncpy(var->buf, val, var->buf_size);
			return 1;
		}
		var = var->next;
	}
	return 0;
}

static int pfm_test_persist_var(void)
{
	enum al_err err;
	u8 tlv_buf[TF_TLV_BUF_SIZE];
	size_t tlv_len;
	size_t tlv_used;
	enum conf_error c_err;
	struct test_var **vars;

	tlv_used = 0;
	for (vars = &gvc.var_list; *vars; vars = &(*vars)->next) {
		tlv_len = sizeof(tlv_buf) - tlv_used;
		c_err = tlv_import(tlv_buf + tlv_used, &tlv_len,
		    ATLV_UTF8, (const u8 *)(*vars)->name,
		    strlen((*vars)->name));
		if (CONF_ERR_NONE != c_err) {
			return -1;
		}
		tlv_used += tlv_len;

		tlv_len = sizeof(tlv_buf) - tlv_used;
		c_err = tlv_import(tlv_buf + tlv_used, &tlv_len,
		    ATLV_UTF8, (const u8 *)(*vars)->buf, strlen((*vars)->buf));
		if (CONF_ERR_NONE != c_err) {
			return -2;
		}
		tlv_used += tlv_len;

	}

	if (tlv_used > 0) {
		err = al_persist_data_write(AL_PERSIST_FACTORY,
		    TEST_VAR_NAME, tlv_buf, tlv_used);
		if (AL_ERR_OK != err) {
			return -3;
		}
	}
	return 0;
}

static int pfm_test_load_var(void)
{
	ssize_t val_len;
	u8 name[32];
	u8 val[128];
	u8 tlv_buf[TF_TLV_BUF_SIZE];
	size_t tlv_len;
	size_t tlv_used;
	enum conf_error c_err;

	val_len = al_persist_data_read(AL_PERSIST_FACTORY,
	    TEST_VAR_NAME, tlv_buf, sizeof(tlv_buf));
	if (val_len <= 0) {
		return -1;
	}

	tlv_len = 0;
	tlv_used = 0;
	while (tlv_used < val_len) {
		tlv_len = sizeof(name);
		c_err = tlv_export(ATLV_UTF8, name,
		    &tlv_len, tlv_buf + tlv_used,
		    sizeof(tlv_buf) - tlv_used);
		if (CONF_ERR_NONE != c_err) {
			break;
		}
		name[tlv_len] = '\0';
		tlv_used += tlv_info(tlv_buf + tlv_used, NULL, NULL, NULL);

		tlv_len = sizeof(val);
		c_err = tlv_export(ATLV_UTF8, val,
		    &tlv_len, tlv_buf + tlv_used,
		    sizeof(tlv_buf) - tlv_used);
		if (CONF_ERR_NONE != c_err) {
			break;
		}
		tlv_used += tlv_info(tlv_buf + tlv_used, NULL, NULL, NULL);

		pfm_test_set_test_var((char *)name, (char *)val);
	}
	return 0;
}

static void pfm_testvar_main(int argc, char **argv)
{
	struct test_var *var;

	if (argc == 2 && strcasecmp(argv[1], "show") == 0) {
		var = gvc.var_list;
		while (var) {
			printcli("  %s = \"%s\"", var->name, var->buf);
			var = var->next;
		}
		return;
	} else if (argc == 4 && strcasecmp(argv[1], "set") == 0) {
		if (pfm_test_set_test_var(argv[2], argv[3])) {
			if (pfm_test_persist_var()) {
				printcli("The change is not persisted");
			}
		} else {
			printcli("Can not find the variable %s", argv[2]);
		}
		return;
	}

	printcli("Usage:");
	printcli("\t%s show", TEST_VAR_NAME);
	printcli("\t%s set <name> <value>", TEST_VAR_NAME);
}

static void pfm_altest_main(int argc, char **argv)
{
	int i;
	int len;
	char **argv_ext;
	const struct pfm_test_desc *desc;
	if (argc == 1) {
		desc = NULL;
		printcli("All test cases:");
		while (NULL != (desc = pfm_test_cases_iter(desc, 1))) {
			if (desc->order <= PFM_TCASE_ORD_MANUAL) {
				printcli("  name:%s, manual",
				    desc->name);
			} else {
				printcli("  name:%s, order:%d",
				    desc->name, desc->order);
			}
		}
		return;
	}

	/* Last test by name may not finished. The free memory should be
	 * freed here.
	 */
	if (gv.names) {
		al_os_mem_free(gv.names);
	}
	if (gv.name_buf) {
		al_os_mem_free(gv.name_buf);
	}
	if (gv.arg_buf) {
		al_os_mem_free(gv.arg_buf);
	}
	if (gv.argv_ext) {
		al_os_mem_free(gv.argv_ext);
	}

	memset(&gv, 0, sizeof(gv));
	gv.repeat_max = 1;
	if (pfm_altest_parse_arg(argc, argv)) {
		return;
	}

	/* gv.argv_ext is not persistent, make it persistent */
	for (len = 0, i = 0; i < gv.argc_ext; i++) {
		if (gv.argv_ext[i]) {
			len += strlen(gv.argv_ext[i]) + 1;
		}
	}
	if (len) {
		gv.arg_buf = al_os_mem_calloc(len+4);
		ASSERT(gv.arg_buf);
		argv_ext = al_os_mem_calloc(gv.argc_ext * sizeof(char *));
		ASSERT(gv.argv_ext);
		for (len = 0, i = 0; i < gv.argc_ext; i++) {
			if (gv.argv_ext[i]) {
				strcpy(gv.arg_buf + len, gv.argv_ext[i]);
				argv_ext[i] = gv.arg_buf + len;
				len += strlen(gv.argv_ext[i]) + 1;
			}
		}
		gv.argv_ext = argv_ext;
	}

	/* Start testing */
	if (gv.flag_test_all) {
		pfm_altest_all();
	} else {
		pfm_altest_by_name(argc, argv);
	}
}

void pfm_test_frame_init(void)
{
	const struct pfm_test_desc *desc = NULL;
	const struct pfm_test_var * const *vars;

	pfm_test_cases_init();
	gvc.var_list = NULL;
	gvc.var_free_pos = 0;
	desc = NULL;
	while (NULL != (desc = pfm_test_cases_iter(desc, 0))) {
		if (desc->test_var == NULL) {
			continue;
		}
		vars = desc->test_var;
		while (*vars) {
			pfm_test_insert_variable(*vars);
			vars++;
		}
	}
	pfm_test_load_var();

	al_cli_register(TEST_VAR_NAME, "Test variables maintain",
	    pfm_testvar_main);
	al_cli_register("altest", "AL test", pfm_altest_main);
}

void pfm_test_frame_final(void)
{
	pfm_test_cases_final();
}

void test_printf(const char *fmt, ...)
{
	va_list ap;
	u32 msec;
	struct clock_info tm;
	int size = strlen(fmt) + 48;
	char *new_fmt = alloca(size);
	char buf[256];
	const char *indent;

	if (fmt[0] == '@') {
		fmt++;
		indent = "";
	} else {
		indent = "  ";
	}
	msec = al_clock_get_total_ms() % 1000;
	clock_fill_details(&tm, al_clock_get(NULL));
	snprintf(new_fmt, size,
	    "%4lu-%02u-%02uT%02u:%02u:%02u.%03u ALTEST %s%s\n",
	    tm.year, tm.month, tm.days, tm.hour, tm.min, tm.sec,
	    (unsigned int)msec, indent, fmt);
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), new_fmt, ap);
	va_end(ap);
	al_log_print(buf);
}

struct pfm_test_frame_combi_params {
	int cnt;
	u8 param;
};

char *pfm_test_frame_combi_param_call(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	struct pfm_tf_combi_param_desc *desc;
	struct pfm_tf_combi_param_info *info;
	u8 param_buf[COMBI_PARAM_SIZE_MAX];
	const struct pfm_test_frame_combi_params *combi_param;
	const void *param;
	int i;
	int j;
	int rc;
	char *ret = NULL;

	desc = (struct pfm_tf_combi_param_desc *)pcase->arg;
	ASSERT(desc);
	ASSERT(desc->total_size <= ARRAY_LEN(param_buf));

	for (i = 0; i < desc->num; i++) {
		combi_param = (const struct pfm_test_frame_combi_params *)
		    desc->combi_param[i];
		info = desc->infos + i;
		info->desc = desc;
		param = &combi_param->param;

		for (j = 0; j < combi_param->cnt; j++) {
			memcpy(param_buf, desc->param,
			    desc->total_size);
			memcpy(param_buf + info->offset, param, info->size);
			info->param = param_buf;
			param = ((u8 *)param) + info->size;
			if (desc->tf_init) {
				desc->tf_init(info, argc, argv);
			}
			rc = pfm_try_catch_assert();
			if (!rc) {
				desc->tf_test(info);
				ret = NULL;
			} else {
				ret = gs_fail_msg;
			}
			pfm_try_catch_final();

			if (desc->tf_final) {
				desc->tf_final(info);
			}
			if ((ret && !desc->expect_exception)
			    || (!ret && desc->expect_exception)) {
				return gs_fail_msg;
			}
		}
	}
	return NULL;
}

static void pfm_test_frame_mthread_main(struct al_thread *thread, void *arg)
{
	struct pfm_tf_mthread_desc *mthread_desc;
	struct pfm_tf_mthread_info *mthread_info;

	mthread_info = (struct pfm_tf_mthread_info *)arg;
	if (NULL == mthread_info) {
		mthread_info->result = "argument fault";
		goto on_exit;
	}
	mthread_desc = (struct pfm_tf_mthread_desc *)mthread_info->desc;
	if (NULL == mthread_desc->tf_test) {
		mthread_info->result = "description fault";
		goto on_exit;
	}
	mthread_desc->tf_test(mthread_info);

	return;
on_exit:
	al_ada_call(&mthread_info->tf_exit);
}

static void pfm_test_frame_mthread_exit(void *arg)
{
	struct pfm_tf_mthread_desc *mthread_desc;
	struct pfm_tf_mthread_info *mthread_info;

	mthread_info = (struct pfm_tf_mthread_info *)arg;
	ASSERT(NULL != mthread_info);
	mthread_desc = (struct pfm_tf_mthread_desc *)mthread_info->desc;
	ASSERT(NULL != mthread_desc);

	mthread_desc->finished++;
	if (NULL == mthread_desc->result) {
		mthread_desc->result = mthread_info->result;
	}

	/* release resource */
	if (NULL != mthread_desc->tf_final) {
		mthread_desc->tf_final(mthread_info);
	}
	if (NULL != mthread_info->param) {
		al_os_mem_free(mthread_info->param);
		mthread_info->param = NULL;
	}

	/* report the result */
	if (mthread_desc->finished == mthread_desc->num) {
		pfm_test_case_async_finished(mthread_desc->result);
	}
}

char *pfm_test_frame_mthread_call(const struct pfm_test_desc *pcase,
	int argc, char **argv)
{
	int i;
	struct pfm_tf_mthread_desc *mthread_desc;
	struct pfm_tf_mthread_info *mthread_info;

	mthread_desc = (struct pfm_tf_mthread_desc *)pcase->arg;
	ASSERT(NULL != mthread_desc);
	pfm_test_case_async_start();
	mthread_desc->finished = 0;

	/* precondition */
	for (i = 0; i < mthread_desc->num; i++) {
		mthread_info = mthread_desc->mt_info + i;
		if (mthread_desc->total_size > 0) {
			mthread_info->param = al_os_mem_calloc(
			    mthread_desc->total_size);
			ASSERT(NULL != mthread_info->param);
			memcpy(mthread_info->param, mthread_desc->param,
			    mthread_desc->total_size);
		} else {
			mthread_info->param = NULL;
		}

		snprintf(mthread_info->name, sizeof(mthread_info->name),
		    "%s-%d", pcase->name, i);
		mthread_info->id = i;
		mthread_info->desc = mthread_desc;
		al_ada_callback_init(&mthread_info->tf_exit,
		    pfm_test_frame_mthread_exit, mthread_info);

		if (NULL != mthread_desc->tf_init) {
			mthread_desc->tf_init(mthread_info, argc, argv);
		}

		mthread_info->tid = al_os_thread_create(mthread_info->name,
		    NULL, (1<<16), al_os_thread_pri_normal,
		    pfm_test_frame_mthread_main, mthread_info);
		ASSERT(NULL != mthread_info->tid);
	}
	return NULL;
}

void pfm_test_frame_mthread_error(struct pfm_tf_mthread_info *mthread_info,
		char *result)
{
	mthread_info->result = result;
	al_ada_call(&mthread_info->tf_exit);
}

void pfm_test_case_async_start(void)
{
	gv.flag_async_test = 1;
}

void pfm_test_case_async_finished(char *status)
{
	long t;
	const struct pfm_test_desc *p_case = gv.p_case;
	int pass;

	if (!gv.flag_async_test) {
		return;
	}
	gv.flag_async_test = 0;
	pass = (!status && !p_case->expect) ||
		  (status && p_case->expect);
	get_time(&gv.t2);
	t = time_diff(&gv.t2, &gv.t1);

	pfm_altest_show_case_result(p_case, pass, status, t);

	if (pass) {
		gv.count_passed++;
	}
	if (gv.stop_on_fail && !pass) {
		pfm_altest_show_result();
		return;
	}
	if (!gv.flag_async_test) {
		al_ada_call(&gv.cb);
	}
}
