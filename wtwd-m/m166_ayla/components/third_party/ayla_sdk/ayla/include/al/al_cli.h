/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __AYLA_AL_COMMON_CLI_H__
#define __AYLA_AL_COMMON_CLI_H__

/**
 * @file
 * Platform cli registration Interfaces
 */

/**
 * It registers CLI command handler into platform CLI system
 *
 * \param cmd is command string.
 * \param help is the hit information.
 * \param handler is the command handler.
 *
 * Note: the cli registration should be before enter the main loop, otherwise
 * an assert is raised
 */
void al_cli_register(const char *cmd, const char *help,
		void (*handler)(int argc, char **argv));

/**
 * It changes the CLI command prompt
 *
 * It is not necessary, most module can stub this function.
 *
 * \param pmpt is propmpt.
 */
void al_cli_set_prompt(const char *pmpt);

/**
 * This Prints string on console for CLI commands.
 *
 * If no \\n is present, the line should get no \\n or \\r added by the
 * platform.
 * The platform should ensure the str is shown out even if the \\n is not
 * provided.
 *
 * \param str is a line to output.
 */
void al_cli_print(const char *str);

#endif /* __AYLA_AL_COMMON_CLI_H__ */
