#ifndef _XLINK_FLASH_H
#define _XLINK_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#define  XLink_PAGE_NUM_MAX  2
#define  XLink_PAGE_SIZE  1024
#define  XLink_FILE_NAME_MAXLENGTH  256

#if 1
extern int Xlink_Flash_Init(void);
extern int Xlink_Flash_Read(unsigned char *data,int datalen);
extern int Xlink_Flash_Write(unsigned char *data,int datalen);
#else
typedef struct
{
    unsigned char _gPages[XLink_PAGE_NUM_MAX][XLink_PAGE_SIZE];
    char _gFileName[XLink_FILE_NAME_MAXLENGTH];
}XLINK_FLASH_T;
//flash:存储结构实体
//*s:文件路径
//len:文件路径长度
//return:0表示成功，-1表示失败
extern int Xlink_Flash_Init(XLINK_FLASH_T *flash,char * s,int len);
//flash:存储结构实体
//page:叶索引
//offset:偏移量
//data：数据指针
//datalen：数据长度
//return:返回读取字节数，-1表示失败
extern int Xlink_Flash_Read(XLINK_FLASH_T *flash,int page,int offset,unsigned char *data,int datalen);
//flash:存储结构实体
//page:叶索引
//offset:偏移量
//data：数据指针
//datalen：数据长度
//return:返回写入字节数，-1表示失败
extern int Xlink_Flash_Write(XLINK_FLASH_T *flash,int page,int offset,unsigned char *data,int datalen);
#endif

#ifdef __cplusplus
}
#endif

#endif
