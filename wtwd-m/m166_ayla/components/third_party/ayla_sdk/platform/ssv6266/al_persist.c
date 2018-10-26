/*
 * Copyright 2018 Ayla Networks, Inc.  All rights reserved.
 */
#include <stdlib.h>
#include <al/al_persist.h>


#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

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
	return -1;
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
