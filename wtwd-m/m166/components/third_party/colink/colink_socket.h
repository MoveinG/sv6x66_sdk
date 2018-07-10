#ifndef __COLINK_SOCKET_H__
#define __COLINK_SOCKET_H__

#include <stdint.h>
#include "colink_typedef.h"
#include "colink_error.h"

/**
 * @brief 创建tcp连接
 *
 * @par 描述:
 * 创建非阻塞模式tcp连接
 *
 * @param dst      [IN] 接收方ip地址。
 * @param port     [IN] 接收方端口号。
 *
 * @retval COLINK_TCP_ARG_INVALID           det为空。
 * @retval COLINK_TCP_CREATE_CONNECT_ERR    创建连接失败。
 * @retval 大于0     连接成功,返回tcp套接字，然后使用colinkTcpState判断连接是否完全建立。
 */
int32_t colinkTcpConnect(const char* dst, uint16_t port);

/**
 * @brief tcp连接状态
 *
 * @par 描述:
 * 查询tcp连接状态
 *
 * @param fd      [IN] tcp套接字。
 *
 * @retval COLINK_NO_ERROR   连接成功。
 * @retval COLINK_TCP_CONNECTING   TCP连接中。
 * @retval COLINK_TCP_CONNECT_ERR  TCP连接失败。
 */
ColinkTcpErrorCode colinkTcpState(int32_t fd);

/**
 * @brief 断开tcp连接。
 *
 * @par 描述:
 * 断开tcp连接。
 *
 * @param fd   [IN] colinkTcpConnect创建的套接字。
 *
 * @retval 无。
 */
void colinkTcpDisconnect(int32_t fd);

/**
 * @brief 发送tcp数据
 *
 * @par 描述:
 * 非阻塞发送tcp数据
 *
 * @param fd      [IN] tcp套接字。
 * @param buf     [IN] 指向待发送数据缓冲区的指针。
 * @param len     [IN] 待发送数据的长度，范围为[0，512)。
 *
 * @retval COLINK_TCP_ARG_INVALID 无效的参数。
 * @retval COLINK_TCP_SEND_ERR    发送出现错误。
 * @retval 大于0     发送成功，返回发送数据的字节数。
 */
int32_t colinkTcpSend(int32_t fd, const uint8_t* buf, uint16_t len);

/**
 * @brief 读取tcp数据
 *
 * @par 描述:
 * 非阻塞读取tcp数据
 *
 * @param fd      [IN] tcp套接字。
 * @param buf     [IN] 指向存放接收数据缓冲区的指针。
 * @param len     [IN] 存放接收数据缓冲区的最大长度，范围为[0，512)。
 *
 * @retval COLINK_TCP_ARG_INVALID 无效的参数。
 * @retval COLINK_TCP_READ_ERR    读取失败错误。
 * @retval 0         未收到任何数据。
 * @retval 大于0     读取成功，返回读取数据的字节数。
 */
int32_t colinkTcpRead(int32_t fd, uint8_t* buf, uint16_t len);

/**
 * @brief 获取远端主机ip地址。
 *
 * @par 描述:
 * 通过DNS域名解析，获取远端主机ip地址，此接口实现为非阻塞。
 *
 * @param hostname    [IN] 远端主机名称。
 * @param ip_list     [OUT] 存放远端主机ip地址列表的数组。
 * @param num         [IN] 存放远端主机ip地址列表的数组的大小，范围为[1，4]。
 *
 * @retval COLINK_NO_ERROR              获取成功。
 * @retval COLINK_TCP_ARG_INVALID       无效的参数。
 * @retval COLINK_GET_HOSTBYNAME_ERR    获取域名失败。
 */
ColinkTcpErrorCode colinkGethostbyname(char* hostname, char ip_list[][20], uint8_t num);

#endif
