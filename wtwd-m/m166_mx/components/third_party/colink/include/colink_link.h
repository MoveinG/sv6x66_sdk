/** 
 * @file     colink_link.h
 */
#ifndef __COLINK_LINK_H__
#define __COLINK_LINK_H__

#include <stdint.h>
#include "colink_typedef.h"
#include "colink_error.h"

/**
 * 配网结果枚举类型。
 */
typedef enum
{
    COLINK_LINK_INIT_INVALID        = 0,    /**< 未复位配网初始状态 */
    COLINK_LINK_RES_DEV_INFO_OK     = 1,    /**< 响应设备信息成功 */
    COLINK_LINK_RES_DEVICEID_ERROR  = 2,    /**< 响应设备信息失败 */
    COLINK_LINK_GET_INFO_OK         = 3,    /**< 获取配网信息成功 */
    COLINK_LINK_GET_INFO_ERROR      = 4,    /**< 获取配网信息失败 */
} CoLinkLinkState;

/**
 * 配网信息结构体。
 */
typedef struct
{
    char ssid[32];              /**< 路由器ssid */
    char password[64];          /**< 路由器密码 */
    char distor_domain[32];     /**< 分配服务器域名 */
    uint16_t distor_port;       /**< 分配服务器端口 */
}ColinkLinkInfo;

/**
 * @brief 初始化配网。
 *
 * @par 描述:
 * 在colinkLinkReset之前调用初始化。
 *
 * @param deviceid      [IN] 设备ID。
 * @param apikey        [IN] 设备密钥。
 *
 * @retval COLINK_LINK_NO_ERROR             初始化成功。
 * @retval COLINK_LINK_ARG_INVALID          传入参数错误。
 *
 */
ColinkLinkErrorCode colinkLinkInit(char *deviceid, char *apikey);

/**
 * @brief 复位配网阶段到初始状态。
 *
 * @par 描述:
 * 每次进入配网阶段前都需要调用colinkLinkReset，
 * 使配网进入初始状态。
 *
 * @param 无。
 *
 */
void colinkLinkReset(void);

/**
 * @brief 用在与APP交互阶段，负责解析并输出响应APP的数据。
 *
 * @par 描述:
 * 在每次进入配网流程前需要调用colinkLinkReset。每收到来自APP的数据，就调用一次
 * colinkLinkParse，并将输出数据发送给APP。正常流程会依次返回结果
 * COLINK_LINK_RES_DEV_INFO_OK和COLINK_LINK_GET_INFO_OK，当返回
 * COLINK_LINK_GET_INFO_OK时即可结束配网流程。
 *
 * @param in            [IN] 输入数据的指针。
 * @param in_len        [IN] 输入数据的长度。
 * @param out           [IN] 输出数据的指针。
 * @param out_buf_len   [IN] 输出数据缓冲区的长度。
 * @param out_len       [IN] 输出数据的的长度。
 *
 * @retval COLINK_LINK_RES_DEV_INFO_OK      响应设备信息成功。
 * @retval COLINK_LINK_RES_DEVICEID_ERROR   当入参错误或数据异常会导致响应设备信息失败。
 * @retval COLINK_LINK_GET_INFO_OK          获取配网信息成功。
 * @retval COLINK_LINK_GET_INFO_ERROR       当入参错误或数据异常会导致获取配网信息失败。
 *
 */
CoLinkLinkState colinkLinkParse(uint8_t *in, 
            uint16_t in_len, 
            uint8_t *out, 
            uint16_t out_buf_len, 
            uint16_t *out_len);

/**
 * @brief 配网成功后获取分配服务器信息。
 *
 * @par 描述:
 * 当colinkLinkParse返回COLINK_LINK_GET_INFO_OK后，直接调用
 * 此函数获取分配服务器信息。否则返回COLINK_LINK_OPERATION_ERROR错误类型。
 *
 * @param info         [OUT] 输出配对结果信息的指针。
 *
 * @retval COLINK_LINK_NO_ERROR             获取信息成功。
 * @retval COLINK_LINK_ARG_INVALID          传入参数错误。
 * @retval COLINK_LINK_OPERATION_ERROR      操作错误。未按流程操作。
 *
 */
ColinkLinkErrorCode colinkLinkGetInfo(ColinkLinkInfo *info);

#endif    /* __COLINK_LINK_H__ */
