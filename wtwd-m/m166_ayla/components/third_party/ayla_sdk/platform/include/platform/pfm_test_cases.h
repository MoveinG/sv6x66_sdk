/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_PFM_TEST_CASES_H__
#define __AYLA_PFM_TEST_CASES_H__

#include <platform/pfm_test_frame.h>

/**
 * @file
 * Test Cases management
 */

/**
 * Test Case Order base numbers, used for test sequence.
 * These numbers are used for defining the test sequence by module.
 */
enum pfm_test_case_order {
	PFM_TCASE_ORD_ADA_THREAD,	/**< ada thread order start number */
	PFM_TCASE_ORD_AES,		/**< aes order start number */
	PFM_TCASE_ORD_ASSERT,		/**< assert order start number */
	PFM_TCASE_ORD_BASE64,		/**< base64 order start number */
	PFM_TCASE_ORD_CLI,		/**< cli order start number */
	PFM_TCASE_ORD_CLOCK,		/**< clock order start number */
	PFM_TCASE_ORD_ERR,		/**< error order start number */
	PFM_TCASE_ORD_SHA1,		/**< sha1 order start number */
	PFM_TCASE_ORD_SHA256,		/**< sha256 order start number */
	PFM_TCASE_ORD_LOG,		/**< log order start number */
	PFM_TCASE_ORD_NET_ADDR,		/**< net address order start number */
	PFM_TCASE_ORD_NET_DNS,		/**< dns order start number */
	PFM_TCASE_ORD_NET_IF,		/**< network interface order start
					 *   number */
	PFM_TCASE_ORD_NET_STREAM,	/**< stream order start number */
	PFM_TCASE_ORD_NET_UDP,		/**< udp order start number */
	PFM_TCASE_ORD_OS_LOCK,		/**< lock order start number */
	PFM_TCASE_ORD_OS_MEM,		/**< memory order start number */
	PFM_TCASE_ORD_OS_THREAD,	/**< thread order start number */
	PFM_TCASE_ORD_PERSIST,		/**< persist order start number */
	PFM_TCASE_ORD_RANDOM,		/**< random order start number */
	PFM_TCASE_ORD_RSA,		/**< rsa order start number */
	PFM_TCASE_ORD_NET_TCP,		/**< tcp order start number */
	PFM_TCASE_ORD_NET_TLS,		/**< tls order start number */

	/*
	 * Note: when adding new values, insert them before this comment and
	 * update the AL_ERR_STRINGS define below, as well as <ayla/err.h>.
	 */
	PFM_TCASE_ORD_LIMIT,		/**< limit of enums, unused as an error
					 *   code */
};

/**
 * The user-manual order number.If use this order, the test case will not
 * called by the CLI "altest all".
 */
#define PFM_TCASE_ORD_MANUAL		(-1)

/**
 * Give the module base order and the private order, this macro returns
 * the test case order.
 */
#define pfm_tcase_make_order(base, order) \
	(((base) << 8) + (order))

/**
 * Give a module string and the test case id given by test rail, this macro
 * returns the test case name.
 */
#define pfm_tcase_make_name(mod, test_id) (mod "-" test_id)

#endif /* __AYLA_PFM_TEST_CASES_H__ */
