#ifndef _SERIAL_H
#define _SERIAL_H

#include "soc_defs.h"
#include "attrs.h"

void serial_init(int BaudRate);
void serial_tx(int) ATTRIBUTE_SECTION_OTA_FBOOT;
int serial_rx_ready(void) ATTRIBUTE_SECTION_OTA_FBOOT;
int serial_rx() ATTRIBUTE_SECTION_OTA_FBOOT;

void serial_uart1_init(int BaudRate);
void serial_uart1_tx(int);
int serial_uart1_rx_ready(void);
int serial_uart1_rx(void);
#endif
