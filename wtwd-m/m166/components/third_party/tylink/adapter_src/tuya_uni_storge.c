/***********************************************************
*  File: uni_storge.c 
*  Author: nzy
*  Date: 20170920
***********************************************************/
#define __UNI_STORGE_GLOBALS
#include "tuya_uni_storge.h"
#include "fsal.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#define PARTITION_SIZE (1 << 12) /* 4KB */
#define FLH_BLOCK_SZ PARTITION_SIZE

// flash map
#define SIMPLE_FLASH_START 0x301F5000
#define SIMPLE_FLASH_SIZE 0x8000 // 32K
    
#define SIMPLE_FLASH_SWAP_START (SIMPLE_FLASH_START+SIMPLE_FLASH_SIZE)
#define SIMPLE_FLASH_SWAP_SIZE 0x2000 // 8K

#define ty_spi_printf(...)
#define TY_USE_SPIFFS

#ifdef TY_USE_SPIFFS
#define TYLINK_FILENAME "tylink.flh"
extern spiffs* fs_handle;
#endif
/***********************************************************
*************************variable define********************
***********************************************************/
//STATIC flash_t obj;

STATIC UNI_STORGE_DESC_S storge = {
    SIMPLE_FLASH_START,
    SIMPLE_FLASH_SIZE,
    FLH_BLOCK_SZ,
    SIMPLE_FLASH_SWAP_START,
    SIMPLE_FLASH_SWAP_SIZE
};

/***********************************************************
*************************function define********************
***********************************************************/

/***********************************************************
*  Function: uni_flash_read
*  Input: addr size
*  Output: dst
*  Return: none
***********************************************************/
OPERATE_RET tuya_uni_flash_read(IN CONST UINT_T addr, OUT BYTE_T *dst, IN CONST UINT_T size)
{
	ty_spi_printf("%s addr=0x%x, size=%d\n", __func__, addr, size);
#ifndef TY_USE_SPIFFS
    flash_init();

    OS_EnterCritical();
    //UINT_T faddr = addr & 0xFFFFFF;
    //flash_fast_read(faddr, size, dst);
    memcpy(dst, (void*)addr, size);
    OS_ExitCritical();
#else
    if(NULL == fs_handle || NULL == dst || addr < SIMPLE_FLASH_START || (addr+size) > (SIMPLE_FLASH_SWAP_START+SIMPLE_FLASH_SWAP_SIZE))
        return OPRT_INVALID_PARM;

	INT_T pos1, pos2;
    SSV_FILE fd = FS_open(fs_handle, TYLINK_FILENAME, SPIFFS_RDONLY, 0);
	if(fd >= 0)
	{
		pos1 = addr-SIMPLE_FLASH_START;
		pos2 = FS_lseek(fs_handle, fd, pos1, SPIFFS_SEEK_SET);

		ty_spi_printf("read 0x%x lseek=0x%x\n", pos1, pos2);
		if(pos1 == pos2) {
			FS_read(fs_handle, fd, dst, size);
		}
		FS_close(fs_handle, fd);
    	return OPRT_OK;
	}
#endif
	return OPRT_COM_ERROR;
}

/***********************************************************
*  Function: uni_flash_write
*  Input: addr src size
*  Output: none
*  Return: none
***********************************************************/
OPERATE_RET tuya_uni_flash_write(IN CONST UINT_T addr, IN CONST BYTE_T *src, IN CONST UINT_T size)
{
	ty_spi_printf("%s addr=0x%x, size=%d\n", __func__, addr, size);
#ifndef TY_USE_SPIFFS
    flash_init();

    INT_T faddr = addr & 0xFFFFFF;
    INT_T start_page = (faddr/FLASH_PAGE_SIZE);
    INT_T end_page = ((faddr+size-1)/FLASH_PAGE_SIZE);

    OS_EnterCritical();
    INT_T i=0;
    for(i=start_page; i<=end_page; i++) {
   	    flash_page_program(FLASH_PAGE_SIZE*i, FLASH_PAGE_SIZE, src+FLASH_PAGE_SIZE*(i-start_page));
	}
    OS_ExitCritical();
#else
    if(NULL == fs_handle || NULL == src || addr < SIMPLE_FLASH_START || (addr+size) > (SIMPLE_FLASH_SWAP_START+SIMPLE_FLASH_SWAP_SIZE))
        return OPRT_INVALID_PARM;

	INT_T pos1, pos2;
    SSV_FILE fd = FS_open(fs_handle, TYLINK_FILENAME, SPIFFS_CREAT | SPIFFS_RDWR, 0);
	if(fd >= 0)
	{
		pos1 = addr-SIMPLE_FLASH_START;
		pos2 = FS_lseek(fs_handle, fd, 0, SPIFFS_SEEK_END);

		ty_spi_printf("0x%x write lseek=0x%x\n", pos1, pos2);
		if(pos1 > pos2) {
			FS_write(fs_handle, fd, (BYTE_T*)0x300F0000, pos1-pos2); //write dummy
		}

		pos2 = FS_lseek(fs_handle, fd, pos1, SPIFFS_SEEK_SET);
		ty_spi_printf("write 0x%x lseek=0x%x\n", pos1, pos2);

		if(pos1 == pos2) {
			FS_write(fs_handle, fd, (void*)src, size);
		}
		//FS_flush(fs_handle, fd);
		FS_close(fs_handle, fd);
    	return OPRT_OK;
	}
#endif
    return OPRT_COM_ERROR;
}

/***********************************************************
*  Function: uni_flash_erase
*  Input: addr size
*  Output: 
*  Return: none
***********************************************************/
OPERATE_RET tuya_uni_flash_erase(IN CONST UINT_T addr, IN CONST UINT_T size)
{
	ty_spi_printf("%s addr=0x%x, size=%d\n", __func__, addr, size);
#ifndef TY_USE_SPIFFS
    flash_init();

    INT_T faddr = addr & 0xFFFFFF;
    INT_T start_sec = (faddr/PARTITION_SIZE);
    INT_T end_sec = ((faddr+size-1)/PARTITION_SIZE);

    OS_EnterCritical();
    INT_T i=0;
    for(i=start_sec; i<=end_sec; i++) {
        flash_sector_erase(PARTITION_SIZE*i);
    }
    OS_ExitCritical();
#endif
    return OPRT_OK;
}

/***********************************************************
*  Function: uni_get_storge_desc
*  Input: none
*  Output: none
*  Return: UNI_STORGE_DESC_S
***********************************************************/
UNI_STORGE_DESC_S *tuya_uni_get_storge_desc(VOID)
{
    return &storge;
}

