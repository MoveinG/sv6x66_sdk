/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_PERSIST_H__
#define __AYLA_AL_PERSIST_H__

#include <al/al_utypes.h>
#include <al/al_err.h>

/**
 * @file
 * Persistent Data Interfaces.
 *
 * These APIs manage data saved in persistent storage (e.g., flash or disk).
 * Typically 16KB is more enough to store all the data required.
 * The storage must be written in so to avoid losing data if a reset occurs
 * at any time.  This usually will require multiple copies.  If flash-erasible
 * memory is used, excessive erasing should be avoided by appending rather
 * than erasing and re-writing data.
 * Care must be taken not to rewrite data that is unchanged.
 */

/**
 * Persistent data section.
 *
 * This designates the type of configuration.  Factory configuration is
 * normally configuration set by the manufacturer of the module and
 * the device and not normally erased after shipment.
 *
 * Startup configuration is data saved by the application and the agent
 * during Wi-Fi setup and normal operation, and is erased by a factory reset.
 */
enum al_persist_section {
	AL_PERSIST_STARTUP = 0,	/*!< save for startup configuration */
	AL_PERSIST_FACTORY = 1	/*!< save for factory configuration */
};

/**
 * Write persistent data.
 *
 * \param section the type of configuration being saved.
 * \param name the name of the data item.
 * \param buf a pointer to the arbitrary binary data.
 * \param len the length of the data, in bytes.  It may be zero
 * to erase the item.
 * \returns zero on success or a negative error number on error.
 * Possible errors are AL_ERR_ERR on a hardware erase or write error.
 */
enum al_err al_persist_data_write(enum al_persist_section section,
				const char *name,
				const void *buf, size_t len);

/**
 * Read persistent data.
 *
 * \param section the type of configuration being read.
 * \param name the name of the data item.
 * \param buf a pointer to the buffer to receive the data.
 * \param len the length of the buffer, in bytes.
 * \returns the length of bytes read, negative means error.
 */
ssize_t al_persist_data_read(enum al_persist_section section,
			const char *name, void *buf, size_t len);

/**
 * Erase all persisted data for a section.
 *
 * \param section the type of configuration being erased.
 * \returns zero on success and a negative error number on error.
 *
 * This is done on a factory reset to forget all data persisted from the
 * cloud and from Wi-Fi setup, etc.
 */
enum al_err al_persist_data_erase(enum al_persist_section section);

#endif /* __AYLA_AL_PERSIST_H__ */
