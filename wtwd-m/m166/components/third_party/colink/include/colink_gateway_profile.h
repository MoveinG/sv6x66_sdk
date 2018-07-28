/** 
 * @file     colink_gateway_profile.h
 */
#ifndef __COLINK_GATEWAY_PROFILE_H__
#define __COLINK_GATEWAY_PROFILE_H__

#include <stdint.h>
#include "colink_typedef.h"
#include "colink_error.h"

/**
 * 子设备地址的结构体。
 */
typedef struct
{
    uint8_t mac[8];    /**< 子设备MAC地址 */
    uint32_t port;     /**< 子设备端口 */
}ColinkSubDevAddr;

/**
 * 子设备的结构体。
 */
typedef struct
{
    ColinkSubDevAddr addr;    /**< 子设备地址 */
    uint32_t uuid;            /**< 子设备uuid */
}ColinkSubDevice;

/**
 * @brief 通过deviceid获取子设备的地址。
 *
 * @par 描述:
 * 通过deviceid获取子设备的地址。
 *
 * @param deviceid       [IN] deviceid字符串。
 *
 * @retval 非NULL        获取成功。
 * @retval NULL         获取失败。
 */
const ColinkSubDevAddr *colinkGetAddrByDeviceid(char deviceid[11]);

/**
 * @brief 通过子设备地址获取deviceid。
 *
 * @par 描述:
 * 通过子设备地址获取deviceid。
 *
 * @param sub_dev_addr       [IN] 子设备地址的指针。
 *
 * @retval 非NULL        获取成功。
 * @retval NULL         获取失败。
 */
const char *colinkGetDeviceidByAddr(ColinkSubDevAddr *sub_dev_addr);

/**
 * @brief 注册新子设备。
 *
 * @par 描述:
 * 注册多个新子设备。
 *
 * @param dev_list       [IN] 子设备数组的指针。
 * @param dev_num        [IN] 子设备数组的数量。
 *
 * @retval COLINK_GATEWAT_NO_ERROR          注册成功。
 * @retval COLINK_GATEWAT_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAT_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAT_DEV_TYPE_ERROR    设备不属于网关类型。
 */
ColinkGatewayErrorCode colinkAddSubDev(ColinkSubDevice dev_list[], uint16_t dev_num);

/**
 * @brief 删除子设备。
 *
 * @par 描述:
 * 删除多个子设备。
 *
 * @param dev_list       [IN] 子设备数组的指针。
 * @param dev_num        [IN] 子设备数组的数量。
 *
 * @retval COLINK_GATEWAT_NO_ERROR          删除成功。
 * @retval COLINK_GATEWAT_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAT_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAT_DEV_TYPE_ERROR    设备不属于网关类型。
 */
ColinkGatewayErrorCode colinkDelSubDev(ColinkSubDevice dev_list[], uint16_t dev_num);

/**
 * @brief 通知子设备上线。
 *
 * @par 描述:
 * 通知多个子设备上线。
 *
 * @param dev_list       [IN] 子设备数组的指针。
 * @param dev_num        [IN] 子设备数组的数量。
 *
 * @retval COLINK_GATEWAT_NO_ERROR          操作成功。
 * @retval COLINK_GATEWAT_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAT_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAT_DEV_TYPE_ERROR    设备不属于网关类型。
 */
ColinkGatewayErrorCode colinkOnlineSubDev(ColinkSubDevice dev_list[], uint16_t dev_num);

/**
 * @brief 通知子设备下线。
 *
 * @par 描述:
 * 通知多个子设备下线。
 *
 * @param dev_list       [IN] 子设备数组的指针。
 * @param dev_num        [IN] 子设备数组的数量。
 *
 * @retval COLINK_GATEWAT_NO_ERROR          操作成功。
 * @retval COLINK_GATEWAT_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAT_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAT_DEV_TYPE_ERROR    设备不属于网关类型。
 */
ColinkGatewayErrorCode colinkOfflineSubDev(ColinkSubDevice dev_list[], uint16_t dev_num);

/**
 * @brief 子设备响应服务器的错误码。
 *
 * @par 描述:
 * 子设备响应服务器的错误码。当收到colinkSubDevRecvReqCb回调函数后
 * 需要调用此函数响应给服务器。
 *
 * @param sub_dev       [IN] 子设备的指针。
 * @param error_code    [IN] 发送响应的错误码。
 *
 * @retval COLINK_NO_ERROR              发送成功。
 * @retval COLINK_ARG_INVALID           无效的参数。
 * @retval COLINK_JSON_INVALID          无效的JSON格式。
 * @retval COLINK_JSON_CREATE_ERR       创建JSON对象错误。
 * @retval COLINK_DATA_SEND_ERROR       发送数据出错。
 * @retval COLINK_DEV_TYPE_ERROR        设置设备类型不正确。
 */
ColinkErrorCode colinkSubDevSendRes(ColinkSubDevice *sub_dev, ColinkSubDevResultCode error_code);

/**
 * @brief 子设备发送请求的数据。
 *
 * @par 描述:
 * 子设备发送请求的数据。之后会产生colinkSubDevRecvResCb回调函数来获取
 * 服务器响应的错误码。每调用此函数需要等收到服务器响应才能再次调用。
 *
 * @param sub_dev       [IN] 子设备的指针。
 * @param data          [IN] 发送请求的数据。
 *
 * @retval COLINK_NO_ERROR              发送成功。
 * @retval COLINK_ARG_INVALID           无效的参数。
 * @retval COLINK_JSON_INVALID          无效的JSON格式。
 * @retval COLINK_JSON_CREATE_ERR       创建JSON对象错误。
 * @retval COLINK_DATA_SEND_ERROR       发送数据出错。
 * @retval COLINK_DEV_TYPE_ERROR        设置设备类型不正确。
 */
ColinkErrorCode colinkSubDevSendReq(ColinkSubDevice *sub_dev, char* data);

#endif  /* __COLINK_GATEWAY_PROFILE_H__ */

