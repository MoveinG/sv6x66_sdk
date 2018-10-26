/*
 * Copyright 2011-2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_ASSERT_H__
#define __AYLA_ASSERT_H__

#include <al/al_assert.h>

#define AYLA_ASSERT(expr)				\
	do {						\
		if (!(expr)) {				\
			al_assert_handle(__FILE__, __LINE__);	\
		}					\
	} while (0)

#ifndef ASSERT
#define ASSERT AYLA_ASSERT
#endif

#ifdef DEBUG
#define AYLA_ASSERT_DEBUG(expr) AYLA_ASSERT(expr)
#else
#define AYLA_ASSERT_DEBUG(expr) do { (void)(expr); } while (0)
#endif /* DEBUG */

#ifndef ASSERT_DEBUG
#define ASSERT_DEBUG		AYLA_ASSERT_DEBUG
#endif

#define AYLA_ASSERT_NOTREACHED()			\
	do {						\
		al_assert_handle(__FILE__, __LINE__);	\
	} while (0)

#ifndef ASSERT_NOTREACHED
#define ASSERT_NOTREACHED	AYLA_ASSERT_NOTREACHED
#endif

/*
 * Force a compile error if an expression is false or can't be evalutated.
 */
#define ASSERT_COMPILE(name, expr) \
	extern char __ASSERT_##__name[(expr) ? 1 : -1]

/*
 * Force a compile error if size of type is not as expected.
 */
#define ASSERT_SIZE(kind, name, size) \
	ASSERT_COMPILE(kind ## name, sizeof(kind name) == (size))

#endif /* __AYLA_ASSERT_H__ */
