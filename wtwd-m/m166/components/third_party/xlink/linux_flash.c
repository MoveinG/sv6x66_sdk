#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
//#include <memory.h>
#include "fsal.h"
#include "xlink_debug.h"

#include "linux_flash.h"

#define XLINKFILE_NAME "xlink.conf"

extern spiffs* fs_handle;
unsigned char flash_data[XLink_PAGE_NUM_MAX * XLink_PAGE_SIZE];

//flash:存储结构实体
//*s:文件路径
//len:文件路径长度
//return:0表示成功，-1表示失败
#if 1
int Xlink_Flash_Init(void)
{
    memset(flash_data, 0, sizeof(flash_data));
    SSV_FILE fd = FS_open(fs_handle, XLINKFILE_NAME, SPIFFS_CREAT | SPIFFS_RDWR, 0);
    xlink_debug_sdk("fd=%d\n", fd);
	if(fd >= 0)
	{
	    FS_read(fs_handle, fd, flash_data, sizeof(flash_data));
		FS_close(fs_handle, fd);
		return 0;
	}
	return -1;
}
#else
int Xlink_Flash_Init(XLINK_FLASH_T *flash,char * s,int slen)
{
	int i,len;
	unsigned char str[XLink_PAGE_SIZE];
	int fd;

	if(s == NULL)return -1;
	if (slen < 2)return -1;

	memset(flash->_gFileName,0,XLink_FILE_NAME_MAXLENGTH);

	memcpy(flash->_gFileName,s,slen);

	for(i = 0; i < XLink_PAGE_NUM_MAX; i++) {
		memset(flash->_gPages[i],0,XLink_PAGE_SIZE);
	}

	fd = open(flash->_gFileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); /* 创建并打开文件 */
	if (fd)
	{
		len = read(fd, str, XLink_PAGE_SIZE);
		i = 0;
		while(len > 0) {
			memcpy(flash->_gPages[i],str,len);
			len = read(fd, str, XLink_PAGE_SIZE);
			i++;
		}
		close(fd);
		return 0;
	}
	else {
		close(fd);
		return -1;
	}
}
#endif

//flash:存储结构实体
//page:叶索引
//offset:偏移量
//data：数据指针
//datalen：数据长度
//return:返回读取字节数，-1表示失败
#if 1
int Xlink_Flash_Read(unsigned char *data, int datalen)
{
	if(data && datalen > 0 && datalen <= sizeof(flash_data))
	{
		memcpy(data, flash_data, datalen);
		return datalen;
	}
	return -1;
}
#else
int Xlink_Flash_Read(XLINK_FLASH_T *flash,int page,int offset,unsigned char *data,int datalen)
{
	if (page < 0) return -1;
	if (page >= XLink_PAGE_NUM_MAX) return -1;
	if (offset >= XLink_PAGE_SIZE) return -1;
	if ((offset + datalen) >= XLink_PAGE_SIZE) return -1;
	if (datalen > 0)
	{
		memcpy(data,&flash->_gPages[page][offset],datalen);
		return datalen;
	}
	else {
		return -1;
	}
}
#endif

//flash:存储结构实体
//page:叶索引
//offset:偏移量
//data：数据指针
//datalen：数据长度
//return:返回写入字节数，-1表示失败
#if 1
int Xlink_Flash_Write(unsigned char *data,int datalen)
{
	if(data == 0 || datalen > sizeof(flash_data))
		return -1;
    SSV_FILE fd = FS_open(fs_handle, XLINKFILE_NAME, SPIFFS_RDWR, 0);
    xlink_debug_sdk("fd=%d\n", fd);
	if(fd >= 0)
	{
		memcpy(flash_data, data, datalen);
	    FS_write(fs_handle, fd, flash_data, sizeof(flash_data));
		FS_close(fs_handle, fd);
		return datalen;
	}
	return -1;
}
#else
int Xlink_Flash_Write(XLINK_FLASH_T *flash,int page,int offset,unsigned char *data,int datalen)
{
	int i = 0,cnt = 0;
	int fd;
	fd = open(flash->_gFileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); /* 创建并打开文件 */
	if (fd)
	{
		if (page < 0) return -1;
		if (page >= XLink_PAGE_NUM_MAX) return -1;
		if (offset >= XLink_PAGE_SIZE) return -1;
		if ((offset + datalen) >= XLink_PAGE_SIZE) return -1;

		if (datalen > 0)
		{
			memcpy(&flash->_gPages[page][offset],data,datalen);
			for(i = 0; i < XLink_PAGE_NUM_MAX; i++) {
				write(fd, flash->_gPages[i],XLink_PAGE_SIZE); /* 写入　Hello, software weekly字符串 */
			}
			close(fd);
			return datalen;
		}
		else {
			return -1;
		}
	}
	else {
		close(fd);
		return -1;
	}
}
#endif

