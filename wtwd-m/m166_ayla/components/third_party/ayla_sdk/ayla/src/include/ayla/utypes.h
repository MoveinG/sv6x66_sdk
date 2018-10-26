/*
 * Copyright 2011 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_UTYPES_H__
#define __AYLA_UTYPES_H__

#ifdef HAVE_UTYPES
#undef AL_HAVE_UTYPES
#define AL_HAVE_UTYPES
#endif

#ifdef HAVE_UTYPES_64
#undef AL_HAVE_UTYPES_64
#define AL_HAVE_UTYPES_64
#endif

#include <al/al_utypes.h>

#ifndef BIT
#define BIT(bit) (1U << (bit))
#endif

typedef u16		le16;
typedef u16		be16;
typedef u32		le32;
typedef u32		be32;

#define MAX_U8		0xffU
#define MAX_U16		0xffffU
#define MAX_U32		0xffffffffU
#define MAX_U64		0xffffffffffffffffU

#define MAX_S8		0x7f
#define MAX_S16		0x7fff
#define MAX_S32		0x7fffffff
#define MAX_S64		0x7fffffffffffffff

#define MIN_S8		((s8)0x80)
#define MIN_S16		((s16)0x8000)
#define MIN_S32		((s32)0x80000000)
#define MIN_S64		((s64)0x8000000000000000)

#define OFFSET_OF(type, field) ((unsigned)(&((type *)0)->field))
#define CONTAINER_OF(type, field, var) \
	((type *)((char *)(var) - OFFSET_OF(type, field)))

#ifndef ARRAY_LEN
#define ARRAY_LEN(x) (sizeof(x) / sizeof(*(x)))
#endif /* ARRAY_LEN */

#define TSTAMP_CMP(cmp, a, b)   ((s32)((a) - (b)) cmp 0)

#define TSTAMP_LT(a, b)		((s32)((a) - (b)) < 0)
#define TSTAMP_GT(a, b)		((s32)((a) - (b)) > 0)
#define TSTAMP_LEQ(a, b)	((s32)((a) - (b)) <= 0)
#define TSTAMP_GEQ(a, b)	((s32)((a) - (b)) >= 0)

#ifndef ABS
#define ABS(a)			(((s32)(a) < 0) ? -(a) : (a))
#endif /* ABS */

#endif /* __AYLA_UTYPES_H__ */
