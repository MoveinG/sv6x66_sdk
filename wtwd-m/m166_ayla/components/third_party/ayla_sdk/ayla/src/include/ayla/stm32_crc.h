/*
 * Copyright 2011 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_STM32_CRC_H__
#define __AYLA_STM32_CRC_H__

static inline void stm32_crc_reset(void)
{
	CRC->CR = CRC_CR_RESET;
}

static inline u32 stm32_crc_read(void)
{
	return CRC->DR;
}

void stm32_crc_add(const void *buf, size_t len);

u32 stm32_crc(const void *buf, size_t len);

#endif /* __AYLA_STM32_CRC_H__ */
