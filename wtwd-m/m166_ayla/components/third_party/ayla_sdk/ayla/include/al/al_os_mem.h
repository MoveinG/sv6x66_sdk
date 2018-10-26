/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_OS_MEM_H__
#define __AYLA_AL_COMMON_OS_MEM_H__

#include <al/al_utypes.h>

/**
 * @file
 * Platform OS memory management Interfaces
 */

/**
 * Memory allocation type.
 */
enum al_os_mem_type {
	al_os_mem_type_long_period,	/**< Long period memory */
	al_os_mem_type_long_cache,	/**< Cache memory */
};

/**
 * Set memory allocation type of the current thread
 *
 * The implementation can use the information to allocate memory from
 * different pool for the following allocation on the current thread.
 *
 * \param type is allocation type.
 */
void al_os_mem_set_type(enum al_os_mem_type type);

/**
 * Get memory allocation type of the current thread
 *
 * \return allocation type.
 */
enum al_os_mem_type al_os_mem_get_type(void);

/**
 * Allocates memory.
 *
 * The memory will be appropriately aligned for the strictest alignment
 * required by the platform, but at least 4-byte aligned in any case.
 *
 * \param size is memory size required, in bytes.
 *
 * \return NULL if not enough memory of the specified size is available
 */
void *al_os_mem_alloc(size_t size);

/**
 * Allocates memory.
 *
 * Allocates memory just like al_os_mem_alloc but also zeros it.
 *
 * \param size is memory size required, in bytes.
 *
 * \return NULL if not enough memory of the specified size is available
 */
void *al_os_mem_calloc(size_t size);

/**
 * Frees memory.
 *
 * \param mem points to the memory, may be NULL.
 */
void al_os_mem_free(void *mem);

#endif /* __AYLA_AL_COMMON_OS_MEM_H__ */
