#ifndef __COLINK_SETTING_H__
#define __COLINK_SETTING_H__

void colinkSettingStart(void);

int system_param_save_with_protect(char *domain, int size);
int system_param_load(char *domain, int size);
void system_param_delete(void);

int colink_save_timer(char *buffer, int size);
int colink_load_timer(char **buffer);
void colink_delete_timer(void);

#endif /* __COLINK_SETTING_H__ */
