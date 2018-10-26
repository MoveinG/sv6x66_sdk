/*
 * Copyright 2011-2015 Ayla Networks, Inc.  All rights reserved.
 */
#ifndef __ADA_ADA_CONF_H__
#define __ADA_ADA_CONF_H__

#include <ayla/conf.h>
#ifdef ADA_BUILD_LAN_SUPPORT
#include <ada/ada_lan_conf.h>
#else
#ifndef DISABLE_LAN_OTA
#define DISABLE_LAN_OTA
#endif
#endif

#define CONF_PNAME_CLIENT	"client"    /* The persist name for client */
#define CONF_PNAME_SYS_RESET	"sys_reset" /* The persist name for sys reset */
#define CONF_PNAME_LAN_MODE	"lan_mode"  /* The persist name for lan mode */
#define CONF_PNAME_TIME_ZONE	"time_zone" /* The persist name for time zone*/

#define CLIENT_CONF_PUB_KEY_LEN	300	/* max size of binary RSA public key */
#define CLIENT_CONF_SYMNAME_LEN 40	/* max symbolic name len */
#define CLIENT_CONF_REG_TOK_LEN	8	/* registration token length */
#define CLIENT_CONF_REGION_CODE_LEN 2	/* two character country code */
#define ADA_CONF_VER_LEN	80	/* max length of OTA version string */
#define ADA_CONF_REGION_MAX	4	/**< Max length for region */
#define ADA_CONF_DEV_SN_MAX	20	/**< Max length for DSN */
#define ADA_CONF_PUBKEY_MAX	300	/**< Max length for public key */
#define ADA_CONF_OEM_MAX	20	/**< Max length for OEM strings */
#define ADA_CONF_MODEL_MAX	24	/**< Max length for device ID */
#define ADA_CONF_OEM_KEY_MAX	256	/**< Max length of encrypted OEM key */
#define ADA_CONF_DEV_ID_MAX	20	/**< Max length for device ID */
#define ADA_CONF_MFG_MODEL_MAX	32	/**< Max length for mfg model */
#define ADA_CONF_MAC_ADDR_MAX	6	/**< Max length for MAC address */
#define ADA_CONF_HWID_MAX	32	/**< Max length for hardware ID */
#define ADA_CONF_ADS_HOST_MAX (ADA_CONF_OEM_MAX + 1 +\
	ADA_CONF_MODEL_MAX + 1 + 24)	/**< max ADS hostname length incl NUL*/

extern char conf_sys_model[];		/* System model */
extern const char ada_version[];	/* ADA version */
extern const char ada_version_build[];	/* ADA version and build */

/*
 * Variables provided by platform or app for use by the Ayla client.
 */
extern u8 conf_was_reset;	/* indicates factory reset was done */

/*
 * Flags for client events sent to MCU
 */
enum client_event {
	CLIENT_EVENT_UNREG = BIT(0),
	CLIENT_EVENT_REG = BIT(1)
};

/*
 * The ADA configuration.
 *
 * This is device individual parameters for the ADA, the app should load them
 * from some persist storage where it isn't overwritten during the firmware
 * OTA.
 */
struct ada_conf {
	/*
	 * The parameters come from ada application
	 */
	char region[ADA_CONF_REGION_MAX]; /* Service region, US or CN. */
	char dsn[ADA_CONF_DEV_SN_MAX];	  /* Device DSN. */
	char pubkey[ADA_CONF_PUBKEY_MAX]; /* Device RSA public key. */
	int pubkey_len;			  /* The length of public key */

	char oem[ADA_CONF_OEM_MAX];	  /* OEM string. */
	char model[ADA_CONF_MODEL_MAX];	  /* OEM model. */
	u8 oemkey[ADA_CONF_OEM_KEY_MAX];  /* Encrypted OEM key. */

	char mfg_serial[ADA_CONF_DEV_ID_MAX]; /* Manufacture serial. */
	char mfg_model[ADA_CONF_MFG_MODEL_MAX]; /* Manufacture serial. */

	char conf_server[ADA_CONF_ADS_HOST_MAX];
					  /* Server host name, if override
					    desired. */
	u16 ads_port;			  /* Port of ADS service. */
	u16 poll_interval;		  /* Interval for polling if needed,
					    seconds */

	u8 mac_addr[ADA_CONF_MAC_ADDR_MAX];/* System MAC address for
					    identification */
	char hw_id[ADA_CONF_HWID_MAX];	  /* pointer to unique hardware-ID
					    string */

	u8 get_all:1;			  /* 1 to get all to device
					    properties when the first time
					    sign in the ADS. */

	/*
	 * Items set by platform.
	 */
	u8 lan_disable:1;	/* set to disable LAN mode */
	u8 test_connect:1;	/* don't count as first connect */
	u8 enable;		/* client enabled */
	u8 conf_serv_override;	/* override conf_server, use ads-dev */
	u16 conf_port;		/* server port, if not default */

	/*
	 * Items set by client only, but of interest to the platform.
	 */
	char host_symname[CLIENT_CONF_SYMNAME_LEN];
	char reg_token[CLIENT_CONF_REG_TOK_LEN]; /* user registration token */
	u8 reg_user;		/* a user is registered */
	enum client_event event_mask; /* mask of reportable events */
};

extern struct ada_conf ada_conf;

/*
 * Notify platform that the registration flag changed.
 */
void adap_conf_reg_changed(void);

/*
 * Return string to be reported to the cloud as the module image version.
 * This may return ada_version_build.
 */
const char *adap_conf_sw_build(void);

/*
 * Return string reported to LAN clients with the name, version and build info.
 * This may return ada_version.
 */
const char *adap_conf_sw_version(void);

/*
 * Reset the system, optionally to the factory configuration.
 */
void ada_conf_reset(int factory_reset);

/*
 * Initialize config subsystem.
 */
void ada_conf_init(void);

/*
 * CLI "reset" command.
 */
void ada_reset_cli(int argc, char **argv);
extern const char ada_reset_cli_help[];

/*
 * Reset the platform to the saved startup or factory configuration.
 */
void adap_conf_reset(int factory);

#endif /* __ADA_ADA_CONF_H__ */
