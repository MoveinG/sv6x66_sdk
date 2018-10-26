/*
 * Copyright 2011 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_TTY_H__
#define __AYLA_TTY_H__

void tty_tx(u8);
int tty_putchar(char);
void tty_puts(const char *);
int tty_rx_intr(u8);
int tty_get_tx(void);
void tty_early_init(void);
void tty_init(void (*)(char *));
void tty_set_raw(void (*)(u8));
int tty_poll(void);
void tty_flush(void);

void putchar_init(int (*putchar)(char));

#endif /* __AYLA_TTY_H__ */
