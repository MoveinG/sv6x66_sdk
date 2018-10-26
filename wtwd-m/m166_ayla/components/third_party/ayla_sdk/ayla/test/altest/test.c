/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <ayla/log.h>

#include <al/al_ada_thread.h>
#include <al/al_cli.h>
#include <platform/pfm_test_frame.h>
#include <ada/ada.h>
#include <ada/task_label.h>

void ada_demo_main(void)
{
	al_cli_set_prompt("PWB ALTest> ");
	log_init();
	log_mask_init_min(BIT(LOG_SEV_INFO), LOG_DEFAULT|BIT(LOG_SEV_DEBUG));
	log_thread_id_set(TASK_LABEL_CLIENT);

	pfm_test_frame_init();
	al_ada_main_loop();
	pfm_test_frame_final();

	log_final();
}
