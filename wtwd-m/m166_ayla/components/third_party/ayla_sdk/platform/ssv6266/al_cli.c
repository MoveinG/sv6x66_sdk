/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <al/al_cli.h>

extern void cli_register(const char *cmd, const char *help,
		void (*handler)(int argc, char **argv));

void al_cli_register(const char *cmd, const char *help,
		void (*handler)(int argc, char **argv))
{
	cli_register(cmd,help,handler);
}

void al_cli_set_prompt(const char *pmpt)
{
	printf(pmpt);
}

void al_cli_print(const char *str)
{
	printf(str);
}
