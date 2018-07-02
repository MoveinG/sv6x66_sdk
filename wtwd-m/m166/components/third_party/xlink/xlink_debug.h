#ifndef _XLINK_DEBUG_H
#define _XLINK_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#define xlink_sdk_debug    printf

extern int  XLINK_DEBUG_SDK_ON;
extern int  XLINK_DEBUG_CLOUD_ON;
extern int  XLINK_DEBUG_LOCAL_ON;

#define xlink_debug_sdk(x,y...) 	   do{if(XLINK_DEBUG_SDK_ON) xlink_sdk_debug("xlink sdk debug =>fun(%s)line:%d:->"x,__FUNCTION__,__LINE__,##y);}while(0)
#define xlink_debug_cloud(x,y...) 	   do{if(XLINK_DEBUG_CLOUD_ON) xlink_sdk_debug("xlink cloud debug =>fun(%s)line:%d:->"x,__FUNCTION__,__LINE__,##y);}while(0)
#define xlink_debug_local(x,y...) 	   do{if(XLINK_DEBUG_LOCAL_ON) xlink_sdk_debug("xlink local debug =>fun(%s)line:%d:->"x,__FUNCTION__,__LINE__,##y);}while(0)


#define print_array_debug(data, datalength) 	do\
{\
    int i;\
	printf("datalen=%d.\r\n", datalength);\
	for (i = 0; i < datalength; i++) {\
		printf ("%02X ",(unsigned char)data[i]);\
	}\
	printf ("\r\n");\
}while(0)



#ifdef __cplusplus
}
#endif

#endif


