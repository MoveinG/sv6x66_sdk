/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_persist.h>
#include <ayla/assert.h>
#include <ayla/base64.h>
#include <fsal/fsal.h>
#include <jsmn.h>

#if PERSIST_CONFIG_FILE

char *peersist_file_buf = " \
  \"factory\": {  \
    \"device_dsn\": \"U0MwMDBXMDAwMDM0NDUw\", \
    \"device_key\": \"MIIBCgKCAQEAiVR9MWHpUxF6T68BHlTHCKle01o7p1JT6zY4SpzhwaEHePytkvwIOrWSYtFlH7GKNAgnTr5UWxiNum5lVUJ5f+Wbr9xNuMIIyvSIYTw+re1kq1Hil0/rnJhvWgaEyag9ukdSMPSlm9+Q/rcV8HdI2qT3Df4S3ju2kSTWnWpSchPcHsjIXujcBs5D6UYTynuQIWWh2r5kD+DbHNkwEsoF11TDUemPdn2Gbh1VYHvnnyl5oZqbuvPkEK6ybxMD/ZiLyVadeboe5DVHLNylZrHKDg3l8FhO7LNmrYwFVZWf/rUjBUqUjM6hg8q8oRs8e1oVIzhNppvJZqLOF3YZDX2qVwIDAQAB\", \
    \"oem_id\": \"MzQ4YTFhYzc=\",\
    \"oem_key\": \"aH1ghasvKKGx0bpOMI95uYi7H4+DtJ9vpr5GD4STihh3qoHJllPIe//rKMWAnEBwEG9SmrGA0sA3etC3hCl5VfS4NF6yxRQ0UPPgl8i1aI7hqUR8JLZMd8h4DRE48jdf17gIRYsQQaXRRYeGW2eJ69Jvtd4C51cTNeWvFfllPE/8VpMRCgQMw0cEtxjHWdkMdZ4fQnigtOHB9zpx63pHPEAL2GVlrN5RO/SApmX3IKDYp/u7fhFzAzi03ZzPG4urV4n4HLG7Vnlopk0tLpvyiKL95UjPQOTiKehDeivVrO3qVU0w5Ul5j48xZj2i78MuKT5LYi9RKr3ef78cnZR1cw==\", \
    \"oem_model\": \"aWNvbW1sZWQ=\",\
    \"service_region\": \"Q04=\"\
  },\
  \"startup\": {}";

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))
#define ARRAY_LEN(x) (sizeof(x) / sizeof(*(x)))

#define MAX_NAME_LEN  32
#define NAX_DATA_LEN  1024
#define MAX_PERSIST_ITEMS 30
#define MAX_BASE64_BUF_SIZE  (BASE64_LEN_EXPAND(NAX_DATA_LEN)+1)

static int persist_loaded;
static char persist_buf[24 * 1024];
static jsmn_parser persist_parser;
static jsmntok_t persist_toks[MAX_PERSIST_ITEMS * 2 * 2 + 2];
static const char * const persist_section[] = {
	[AL_PERSIST_STARTUP] = "startup",
	[AL_PERSIST_FACTORY] = "factory",
};
static char *persist_filename = ".pwb.conf";
static struct al_lock *gp_mutex;

extern SSV_FS fs_handle; 

static char *get_persist_file_name(void)
{
	return persist_filename;
}

static int persist_read_file(const char *filename)
{
	FILE *fp;
	int ret = -1;
	int size;
	SSV_FILE_STAT stat;

	if (persist_loaded) {
		return 0;
	}

	memset(&stat,0,sizeof(SSV_FILE_STAT));
	FS_stat(fs_handle, filename, &stat);
	size = stat.size;
	// fseek(fp, 0, SEEK_END);
	// size = ftell(fp) + 1;
	if (size >= sizeof(persist_buf)) {
		goto exit;
	}

	persist_buf[0] = '\0';
	fp = FS_open(fs_handle, filename, SPIFFS_RDWR, 0);
	if (fp == NULL) {
		goto exit;
	}

	FS_lseek(fs_handle,fp, 0, SPIFFS_SEEK_SET);
	size = FS_read(fs_handle,fp,persist_buf, size);
	if (size <= 0) {
		persist_buf[0] = '\0';
	} else {
		persist_buf[size] = '\0';
		ret = 0;
	}

exit:
	if (fp) {
		FS_close(fs_handle, fp);
	}

	persist_loaded = 1;
	jsmn_init_parser(&persist_parser, persist_buf, persist_toks,
	    ARRAY_LEN(persist_toks));
	if (jsmn_parse(&persist_parser) != JSMN_SUCCESS) {
		ret = -2;
	}

	return ret;
}

static void persist_copy_data(FILE *fp, enum al_persist_section sec,
		enum al_persist_section section, const char *name)
{
	jsmntok_t *parent;
	jsmntok_t *tok;
	jsmntok_t *tok_name;
	int name_len;

	parent = jsmn_get_val(&persist_parser, persist_toks,
	    persist_section[sec]);
	if (parent == NULL || parent->type != JSMN_OBJECT) {
		return;
	}

	name_len = strlen(name);
	for (tok = parent + 1; tok < persist_parser.tokens +
	    persist_parser.num_tokens; tok++) {
		if (tok->start > parent->end) {
			break;
		}
		tok_name = tok++;
		if (tok->start > parent->end) {
			break;
		}
		if (tok_name->type != JSMN_STRING ||
		    tok->type != JSMN_STRING) {
			continue;
		}

		if (sec == section &&
		    name_len == tok_name->end - tok_name->start &&
		    !memcmp(persist_buf + tok_name->start, name, name_len)) {
			continue;
		}
		fprintf(fp, "    \"");
		fwrite(persist_buf + tok_name->start, sizeof(char),
		    tok_name->end - tok_name->start, fp);
		fprintf(fp, "\": \"");
		fwrite(persist_buf + tok->start, sizeof(char),
		    tok->end - tok->start, fp);
		fprintf(fp, "\",\n");
	}
}

enum al_err al_persist_data_write(enum al_persist_section section,
		const char *name,
		const void *buf, size_t len)
{
	FILE *fp;
	char *filename;
	char b64buf[MAX_BASE64_BUF_SIZE];
	size_t size;
	enum al_err err;
	ASSERT(section >= AL_PERSIST_STARTUP && section <= AL_PERSIST_FACTORY);
	ASSERT(name);
	if (name[0] == '\0' || strlen(name) > MAX_NAME_LEN) {
		return AL_ERR_INVAL_VAL;
	}
	if (buf && len) {
		ASSERT(BASE64_LEN_EXPAND(len) < sizeof(b64buf) - 1);
	} else {
		len = 0;
	}

	al_os_lock_lock(gp_mutex);
	if (len) {
		size = sizeof(b64buf);
		ayla_base64_encode(buf, len, b64buf, &size);
	}
	filename = get_persist_file_name();
	persist_read_file(filename);

	fp = FS_open(fs_handle, filename, SPIFFS_RDWR, 0);
	if (fp == NULL) {
		err = AL_ERR_ERR;
		goto last;
	}

	fprintf(fp,
	    "{\n"
	    "  \"factory\": {\n");
	persist_copy_data(fp, AL_PERSIST_FACTORY, section, name);
	if (len && section == AL_PERSIST_FACTORY) {
		fprintf(fp, "    \"%s\": \"%s\",\n", name, b64buf);
	}
	fprintf(fp,
	    "  },\n"
	    "  \"startup\": {\n");
	persist_copy_data(fp, AL_PERSIST_STARTUP, section, name);
	if (len && section == AL_PERSIST_STARTUP) {
		fprintf(fp, "    \"%s\": \"%s\",\n", name, b64buf);
	}
	fprintf(fp,
	    "  }\n"
	    "}");

	FS_close(fs_handle,fp);
	persist_loaded = 0;
	err = AL_ERR_OK;
last:
	al_os_lock_unlock(gp_mutex);
	return err;
}

ssize_t al_persist_data_read(enum al_persist_section section,
		const char *name, void *buf, size_t len)
{
	jsmntok_t *tok;
	char *data;
	ssize_t size;
	ssize_t rc;
	ASSERT(section >= AL_PERSIST_STARTUP && section <= AL_PERSIST_FACTORY);
	ASSERT(name);
	ASSERT(buf);
	if (name[0] == '\0' || strlen(name) > MAX_NAME_LEN) {
		return -1;
	}
	al_os_lock_lock(gp_mutex);
	if (persist_read_file(get_persist_file_name())) {
		rc = -2;
		goto last;
	}

	tok = jsmn_get_val(&persist_parser, persist_toks,
	    persist_section[section]);
	if (tok == NULL) {
		rc = -3;
		goto last;
	}

	size = jsmn_get_string_ptr(&persist_parser, tok, name, &data);
	if (size < 0) {
		rc = -4;
		goto last;
	}

	if (ayla_base64_decode(data, size, buf, &len)) {
		rc = -5;
		goto last;
	}
	rc = len;

	
last:
	al_os_lock_unlock(gp_mutex);
	return rc;
}

enum al_err al_persist_data_erase(enum al_persist_section section)
{
	enum al_err err;
	FILE *fp;
	char *filename;

	ASSERT(section >= AL_PERSIST_STARTUP && section <= AL_PERSIST_FACTORY);

	al_os_lock_lock(gp_mutex);
	filename = get_persist_file_name();
	persist_read_file(filename);
	fp = FS_open(fs_handle, filename, SPIFFS_RDWR, 0);
	if (fp == NULL) {
		err = AL_ERR_ERR;
		goto last;
	}
	fprintf(fp,
	    "{\n"
	    "  \"factory\": {\n");
	if (section != AL_PERSIST_FACTORY) {
		persist_copy_data(fp, AL_PERSIST_FACTORY, section, "");
	}
	fprintf(fp,
	    "  },\n"
	    "  \"startup\": {\n");
	if (section != AL_PERSIST_STARTUP) {
		persist_copy_data(fp, AL_PERSIST_STARTUP, section, "");
	}
	fprintf(fp,
	    "  }\n"
	    "}");
	FS_close(fs_handle,fp);
	persist_loaded = 0;
	err = AL_ERR_OK;
last:
	al_os_lock_unlock(gp_mutex);
	return err;
}

void persist_file_init(void){
	int fd = 0;
	int rlt = 0;
	fd = FS_open(fs_handle, get_persist_file_name(), SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        printf("info file create error\n");
        rlt = FS_errno(fs_handle);
        printf("rlt = %d\n", rlt);
        return -1;
    }
	FS_write(fs_handle, fd, peersist_file_buf, sizeof(peersist_file_buf));
	FS_close(fs_handle, fd);
}

void al_persist_init(void)
{
	SSV_FILE_STAT stat;
	gp_mutex = al_os_lock_create();

	memset(&stat,0,sizeof(SSV_FILE_STAT));
	FS_stat(fs_handle, get_persist_file_name(), &stat);
	if(stat.size <= 0){
		persist_file_init();
	}
}

void al_persist_final(void)
{
	al_os_lock_destroy(gp_mutex);
	gp_mutex = NULL;
	persist_loaded = 0;
	// persist_filename[0] = '\0';
}

#else


// ./conf-gen.py --region CN SC000W000034174.xml 348a1ac7 icommled --oem-key e5ee8fe314fbcc726c58eef53b3d1597


//rsa public key
char *public_key = "MIIBCgKCAQEAiVR9MWHpUxF6T68BHlTHCKle01o7p1JT6zY4SpzhwaEHePytkvwIOrWSYtFlH7GKNAgnTr5UWxiNum5lVUJ5f+Wbr9xNuMIIyvSIYTw+re1kq1Hil0/rnJhvWgaEyag9ukdSMPSlm9+Q/rcV8HdI2qT3Df4S3ju2kSTWnWpSchPcHsjIXujcBs5D6UYTynuQIWWh2r5kD+DbHNkwEsoF11TDUemPdn2Gbh1VYHvnnyl5oZqbuvPkEK6ybxMD/ZiLyVadeboe5DVHLNylZrHKDg3l8FhO7LNmrYwFVZWf/rUjBUqUjM6hg8q8oRs8e1oVIzhNppvJZqLOF3YZDX2qVwIDAQAB";
char *dsn_val = "SC000W000034450";
// char *public_key = "MIIBCgKCAQEAt/0oLIGTjqF0kL2C/SbYulEfA2Y770XKlM+ZRyY65xaHwCuakpK16WE893WAOfXAytM4bt06sCUWf18/FsW1X2vmvvrdLHE4OS9ReEX953R7MRsal1ATjk0tAWfx/l0dxMWdXWlifFSIrFuuGwtDZnmeqv2ewg2lTRuOXGFmjRC0CZK5I9EIGpXhJucIk23kjhKZduI1g3relqMdQucQ5s4KYavlMQCQz3SXQA09xdcNPViaZXhUzHKknBtrIDy5Iw1W+SveZAfF/bp05xnQQRS9FGZjSnxzwOOZrxf4p9NNdmQAP1l7+2Nhq6SKPG/uFrdUaikRgkQocWx4XpwliwIDAQAB";
// char *dsn_val = "SC000W000034174";
char *oem_id = "348a1ac7";
// char *oem_key = "SSQVpIvGFrpWqm+ej/zPdQL34m7PeWhE4BydEMykw6Smz/YeTlDrujplXGDKwGGEFU3rELh2z/9GnKeDRMZjnQMwf8fMVrASnicp3Qx2o3WdwpuuT70D63Ek20+EHrrFoxCBYTtxY+jE3wGZOTknaeVss0ZUItv2Pwl4tzEMnW2+tb1X+BVIobx9miquITrYanhIsevFkRl0VFVljdVt9YwIC+Hfn+1Lvzzy6oxYws6Tl9PlcNhQy3LSYq/8ROLACVm/g+Ouo8ReFSTOOKwrW6QQjUG7292VlybmnqWugULLQ+Q7hQRAyMWDg9x1o8Ne9bil9ofiYH3atARW/GI3Ug==";
char *oem_key = "aH1ghasvKKGx0bpOMI95uYi7H4+DtJ9vpr5GD4STihh3qoHJllPIe//rKMWAnEBwEG9SmrGA0sA3etC3hCl5VfS4NF6yxRQ0UPPgl8i1aI7hqUR8JLZMd8h4DRE48jdf17gIRYsQQaXRRYeGW2eJ69Jvtd4C51cTNeWvFfllPE/8VpMRCgQMw0cEtxjHWdkMdZ4fQnigtOHB9zpx63pHPEAL2GVlrN5RO/SApmX3IKDYp/u7fhFzAzi03ZzPG4urV4n4HLG7Vnlopk0tLpvyiKL95UjPQOTiKehDeivVrO3qVU0w5Ul5j48xZj2i78MuKT5LYi9RKr3ef78cnZR1cw==";

char *oem_host_version = "1.0.0.1";
char *oem_model = "icommled";
char *region = "CN";


typedef struct startup_cfg__ {
	char name[32];
	char cfg[64];
	int len;
} startup_cfg;

static startup_cfg start_cfg[8];
static int idx = 0;
enum al_err al_persist_data_write(enum al_persist_section section,
				const char *name,
				const void *buf, size_t len)
{
	
	strcpy(start_cfg[idx].name,name);
	memcpy(start_cfg[idx].cfg,buf,len);
	start_cfg[idx].len = len; 
	idx++;
	return 0;
}

ssize_t al_persist_data_read(enum al_persist_section section,
			const char *name, void *buf, size_t len)
{
	int i = 0;
	int read_len = 0;
	if(name == NULL ){
		return 0;
	}
	if(section == AL_PERSIST_FACTORY){
		if(strcmp(name,"device_key") == 0){
			read_len = len;
			if (ayla_base64_decode(public_key, strlen(public_key), buf, &read_len)) {
				return -1;
			}
		}else if(strcmp(name,"oem_id") == 0){
			strcpy(buf,oem_id);
			read_len = strlen(oem_id);
		}else if(strcmp(name,"oem_key") == 0){
			read_len = len;
			if (ayla_base64_decode(oem_key, strlen(oem_key), buf, &read_len)) {
				return -1;
			}
		}else if(strcmp(name,"oem_model") == 0){
			strcpy(buf,oem_model);
			read_len = strlen(oem_model);
		}else if(strcmp(name,"device_dsn") == 0){
			strcpy(buf,dsn_val);
			read_len = strlen(dsn_val);
		}else if(strcmp(name,"service_region") == 0){
			strcpy(buf,region);
			read_len = strlen(region);
		} 
	}else{
		for(i = 0; i < idx; i++){
			if(strcmp(name,start_cfg[i].name) == 0){
				read_len = MIN(start_cfg[idx].len,len);
				memcpy(buf,start_cfg[i].cfg,read_len);
			}
		}
	}
	
	// int i = 0;
	// printf("name:%s\n",name);
	// for(i = 0; i < read_len; i++)
	// 	printf("%2x ",((unsigned char *)buf)[i]);
	// printf("\n");
	
	return read_len;
}

enum al_err al_persist_data_erase(enum al_persist_section section)
{
	memset(start_cfg,0,sizeof(start_cfg));
	return AL_ERR_OK;
}

#endif