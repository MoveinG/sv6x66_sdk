/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_ASSERT_H__
#define __AYLA_AL_COMMON_ASSERT_H__

/**
 * @file
 * Assert Interfaces.
 */

/**
 * Assert handle
 *
 * It's called when the assert occurs. Normally it should show an alert message
 * and then reboot after a short delay.
 *
 * \param file is the file name where the assert occurs.
 * \param line is the line number where the assert occurs.
 */
void al_assert_handle(const char *file, int line);

#endif /* __AYLA_AL_COMMON_ASSERT_H__ */
