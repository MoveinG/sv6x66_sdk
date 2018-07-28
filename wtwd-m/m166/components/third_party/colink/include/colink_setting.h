#ifndef __COLINK_SETTING_H__
#define __COLINK_SETTING_H__

void colinkSettingStart(void);

int system_param_save_with_protect(char *domain, int size);
int system_param_load(char *domain, int size);

#endif /* __COLINK_SETTING_H__ */
