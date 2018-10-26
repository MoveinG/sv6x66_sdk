/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_UTYPES_H__
#define __AYLA_AL_COMMON_UTYPES_H__

#include <al/al_compiler.h>

/**
 * @file
 * Platform types.
 *
 * Portable code should use <ayla/utypes.h> which includes this file.
 * The platform may override this with its own header.
 *
 * TBD: we will change u32/s32 to be int eventually.
 */
#ifndef AL_HAVE_UTYPES
typedef unsigned char	u8;		/**< unsigned 8-bit integer */
typedef unsigned short	u16;		/**< unsigned 16-bit integer */
typedef unsigned long	u32;		/**< unsigned 32-bit integer */

typedef signed char	s8;		/**< signed 8-bit integer */
typedef short		s16;		/**< signed 16-bit integer */
typedef long		s32;		/**< signed 32-bit integer */
#endif

#ifndef AL_HAVE_UTYPES_64
typedef unsigned long long u64;		/**< unsigned 64-bit integer */
typedef long long	s64;		/**< signed 64-bit integer */
#endif

/*
 * size_t and ssize_t guards are based on linux header files.
 */
#ifndef __size_t
#ifndef AL_HAVE_SIZE_T
typedef unsigned long size_t;		/**< unsigned size type */
#endif
#endif

#ifndef __ssize_t
#ifndef AL_HAVE_SSIZE_T
typedef long ssize_t;			/**< signed size type */
#endif
#endif

#endif /* __AYLA_AL_COMMON_UTYPES_H__ */
