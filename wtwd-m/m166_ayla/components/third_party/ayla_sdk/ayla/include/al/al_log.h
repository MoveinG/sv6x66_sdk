/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_LOG_H__
#define __AYLA_AL_COMMON_LOG_H__

/**
 * @file
 * Platform Logging Interfaces
 */

/**
 * This prints a single line on the console.
 *
 * \param line string to print.
 *
 * The supplied single line will be NUL-terminated and will not have a
 * newline character (‘\\n’) at the end.
 *
 * The output will be terminated with a carriage return and line feed,
 * as appropriate.
 *
 * Where possible, the adaptation layer or SDK should buffer output but
 * if buffers are full, it should block the thread (and the entire system
 * if necessary) until the output can be added to the buffer.
 *
 * Discussion:
 * This would be called only from the logging system in lib/ayla.
 * The port may prefix the log line with something like "[ada]" or
 * just emit it unchanged, or it may not emit logs at all under
 * it's own control, on the final product.
 * Ideally the port would never drop log lines.
 */
void al_log_print(const char *line);

/**
 * Get the log mod name.
 *
 * \param mod_id is log module ID.
 */
const char *al_log_get_mod_name(u8 mod_id);

#endif /* __AYLA_AL_COMMON_LOG_H__ */
