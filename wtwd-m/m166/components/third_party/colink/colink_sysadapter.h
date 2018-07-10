#ifndef __COLINK_SYSADAPTER_H__
#define __COLINK_SYSADAPTER_H__

#include <stdint.h>
#include "colink_error.h"

/**
 * @brief 获取系统运行时间。
 *
 * @par 描述:
 * 获取系统运行时间，以毫秒为单位。
 *
 * @param ms     [OUT]   当前时间
 *
 * @retval COLINK_NO_ERROR    获取成功
 * @retval -1                 获取失败
 */
int colinkGettime(uint32_t* ms);

/**
 * @brief 获取网络状态。
 *
 * @par 描述:
 * 获取网络是否可以与外网通信的状态。当设备连上路由器且分配IP，
 * state为1.当未连接路由器或已连接路由器但未分配IP，state为0.
 *
 * @param state  [OUT]    网络状态,取值范围为[0,1]
 *
 * @retval 0    获取成功
 * @retval 非0  获取失败
 */
int colinkNetworkState(int* state);

/**
 * @brief 随机数发生器的初始化函数。
 *
 * @par 描述:
 * 根据系统提供的种子值，产生随机数。
 *
 * @param 无
 *
 * @retval 随机数
 */
unsigned long colinkRand(void);

/**
 * @brief 计算给定字符串的长度。
 *
 * @par 描述:
 * 计算给定字符串的（unsigned int型）长度，不包括'\0'在内。
 *
 * @param src   [IN]     字符串的首地址
 *
 * @retval 字符串的长度
 */
unsigned int colinkStrlen(const char* src);

/**
 * @brief 复制字符串中的内容。
 *
 * @par 描述:
 * 把src所指向的字符串中以src地址开始复制到dst所指的数组中，并返回dst。
 *
 * @param dst   [IN]     目标字符数组
 * @param src   [IN]     源字符串的起始位置
 *
 * @retval dst 目标字符数组的首地址
 */
char* colinkStrcpy(char* dst, const char* src);

/**
 * @brief 复制字符串中的内容。
 *
 * @par 描述:
 * 把src所指向的字符串中以src地址开始的前len个字节复制到dst所指的数组中，并返回dst。
 *
 * @param dst   [IN]     目标字符数组
 * @param src   [IN]     源字符串的起始位置
 * @param len   [IN]     要复制的字节数
 *
 * @retval dst 目标字符数组的首地址
 */
char* colinkStrncpy(char* dst, const char* src, unsigned int len);

/**
 * @brief 连接字符串。
 *
 * @par 描述:
 * 把src所指字符串添加到dst结尾处实现字符串的连接，连接过程覆盖dst结尾处的'/0'。
 *
 * @param dst   [IN]     目标字符串的指针
 * @param src   [IN]     源字符串的指针
 *
 * @retval dst  目标字符串的指针
 */
char* colinkStrcat(char* dst, const char* src);

/**
 * @brief 连接字符串。
 *
 * @par 描述:
 * 把src所指字符串添加到dst结尾处实现字符串的连接，连接过程覆盖dst结尾处的'/0'。
 *
 * @param dst   [IN]     目标字符串的指针
 * @param src   [IN]     源字符串的指针
 * @param len   [IN]     字符串的长度
 *
 * @retval dst  目标字符串的指针
 */
char* colinkStrncat(char* dst, const char* src, unsigned int len);

/**
 * @brief 查找字符串第一次出现的位置。
 *
 * @par 描述:
 * 在字符串s1中查找第一次出现字符串s2的位置，不包含终止符 '\0'。
 *
 * @param s1   [IN]     字符串1的首地址
 * @param s2   [IN]     字符串2的首地址
 *
 * @retval 非NULL   找到第一次出现的s1字符串位置。
 * @retval NULL     未找到。
 */
char *colinkStrstr(const char *s1, const char *s2);

/**
 * @brief 比较两个字符串的大小。
 *
 * @par 描述:
 * 根据用户提供的字符串首地址及长度，比较两个字符串的大小。指针必须指向合法内存。
 *
 * @param str1   [IN]     字符串1的首地址
 * @param str2   [IN]     字符串2的首地址
 *
 * @retval 等于0   str1等于str2
 * @retval 小于0   str1小于str2
 * @retval 大于0   str1大于str2
 */
int colinkStrcmp(const char* str1, const char* str2);

/**
 * @brief 比较两个字符串的大小。
 *
 * @par 描述:
 * 根据用户提供的字符串首地址及长度，比较两个字符串的大小。指针必须指向合法内存。
 *
 * @param str1   [IN]     字符串1的首地址
 * @param str2   [IN]     字符串2的首地址
 * @param len    [IN]     字符串的长度
 *
 * @retval 等于0   str1等于str2
 * @retval 小于0   str1小于str2
 * @retval 大于0   str1大于str2
 */
int colinkStrncmp(const char* str1, const char* str2, unsigned int len);

/**
 * @brief 查找一个字符在一个字符串中首次出现的位置。
 *
 * @par 描述:
 * 查找一个字符ch在另一个字符串str中末次出现的位置，并返回这个位置的地址。
 * 如果未能找到指定字符，那么函数将返回NULL。
 *
 * @param str    [IN]    字符串的首地址
 * @param ch     [IN]    要查找的字符
 *
 * @retval Null,未能找到指定字符
 * @retval 非Null,字符首次出现位置的地址
 */
char* colinkStrchr(char* str, int ch);

/**
 * @brief 查找一个字符在一个字符串中末次出现的位置。
 *
 * @par 描述:
 * 查找一个字符c在另一个字符串str中末次出现的位置，并返回这个位置的地址。
 * 如果未能找到指定字符，那么函数将返回NULL。
 *
 * @param str    [IN]    字符串的首地址
 * @param c      [IN]    要查找的字符
 *
 * @retval Null     未能找到指定字符
 * @retval 非Null   字符末次出现位置的地址
 */
char* colinkStrrchr(const char* str, char c);

/**
 * @brief 把字符串转换成整型数。
 *
 * @par 描述:
 * 如果第一个非空格字符存在，是数字或者正负号则开始做类型转换，
 * 之后检测到非数字(包括结束符 \0) 字符时停止转换。
 *
 * @param str    [IN]    字符串的首地址
 *
 * @retval 整型数
 */
int colinkAtoi(const char* str);

/**
 * @brief 字符串格式化函数。
 *
 * @par 描述:
 * 把格式化的数据写入某个字符串缓冲区。
 *
 * @param buf        [IN/OUT]    指向将要写入的字符串的缓冲区
 * @param format     [IN]        格式控制信息
 * @param ...        [IN]        可选参数，可以是任何类型的数据
 *
 * @retval 小于0       写入失败
 * @retval 大于等于0   写入的字符串长度
 */
int colinkSprintf(char* buf, const char* format, ...);

/**
 * @brief 字符串格式化函数。
 *
 * @par 描述:
 * 把格式化的数据写入某个字符串缓冲区。
 *
 * @param buf        [IN/OUT]    指向将要写入的字符串的缓冲区
 * @param size       [IN]        要写入的字符的最大数目(含'\0')，超过size会被截断,末尾自动补'\0'
 * @param format     [IN]        格式控制信息
 * @param ...        [IN]        可选参数，可以是任何类型的数据
 *
 * @retval 小于0       写入失败
 * @retval 大于等于0   成功则返回欲写入的字符串长度
 */
int colinkSnprintf(char* buf, unsigned int size, const char* format, ...);

/**
 * @brief 格式化输出函数。
 *
 * @par 描述:
 * 格式化输出，一般用于向标准输出设备按规定格式输出信息。
 *
 * @param    format  [IN]     格式控制信息
 * @param     ...     [IN]     可选参数，可以是任何类型的数据
 *
 * @retval 小于0       输出失败
 * @retval 大于等于0   输出的长度
 */
int colinkPrintf(const char* format, ...);

/**
 * @brief 在一段内存块中填充某个给定的值。
 *
 * @par 描述:
 * 将dst中前len个字节用c替换并返回dst。
 *
 * @param dst   [IN/OUT]     目标的起始位置
 * @param c     [IN]         要填充的值
 * @param len   [IN]         要填充的值字节数
 *
 */
void* colinkMemset(void* dst, int c, unsigned int len);

/**
 * @brief 复制内存中的内容。
 *
 * @par 描述:
 * 从源src所指的内存地址的起始位置开始复制len个字节到目标dst所指的内存地址的起始位置中。
 *
 * @param dst   [IN/OUT]     目标内存的起始位置
 * @param src   [IN]         源内存的起始位置
 * @param len   [IN]         要复制的字节数
 *
 */
void* colinkMemcpy(void* dst, const void* src, unsigned int len);

/**
 * @brief 比较两块内存区域。
 *
 * @par 描述:
 * 根据用户提供的内存首地址及长度，比较两块内存的前len个字节。指针必须指向合法内存。
 *
 * @param buf1   [IN]     内存1的首地址
 * @param buf2   [IN]     内存2的首地址
 * @param len    [IN]     要比较的字节数
 *
 * @retval 等于0   buf1等于buf2
 * @retval 小于0   buf1小于buf2
 * @retval 大于0   buf1大于buf2
 */
int colinkMemcmp(const void* buf1, const void* buf2, unsigned int len);

/**
 * @brief 获取所需的内存空间。
 *
 * @par 描述:
 * 分配所需的内存空间，并返回一个指向它的指针。指针必须指向合法内存。
 *
 * @param n   [IN]     需要分配内存大小。
 *
 * @retval NULL    分配成功
 * @retval 非NULL  分配失败
 *
 */
void* colinkMalloc(unsigned long n);

/**
 * @brief 释放指针pt所占用的内存空间。
 *
 * @par 描述:
 * 根据用户提供的指针pt，释放其所占用的空间。指针必须指向合法内存。
 *
 * @param pt   [IN]     指针
 *
 * @retval 无
 *
 */
void colinkFree(void* pt);

/**
 * @brief 将主机字节顺序转换为网络字节顺序。
 *
 * @par 描述:
 * 把用户提供的主机字节顺序的16位数转换为网络字节顺序表示的16位数。
 *
 * @param ns   [IN]     主机字节顺序表示的16位数
 *
 * @retval 网络字节顺序表示的16位数
 */
unsigned short colinkHtons(unsigned short hs);

/**
 * @brief 将网络字节顺序转换为主机字节顺序。
 *
 * @par 描述:
 * 把用户提供的网络字节顺序的16位数转换为主机字节顺序表示的16位数。
 *
 * @param ns   [IN]     网络字节顺序表示的16位数
 *
 * @retval 主机字节顺序表示的16位数
 */
unsigned short colinkNtohs(unsigned short ns);

#endif
