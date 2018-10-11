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
    uint8_t mac[8];    /**< 子设备MAC地址,仅支持6字节和8字节mac */
    uint32_t port;     /**< 子设备端口 */
}ColinkSubDevAddr;

/**
 * 子设备的结构体。
 */
typedef struct
{
    ColinkSubDevAddr addr;    /**< 子设备地址 */
    uint32_t uiid;            /**< 子设备uiid */
}ColinkSubDevice;

/**
 * 子设备列表的结构体。
 */
typedef struct
{
    ColinkSubDevice dev;    /**< 子设备 */
    char deviceid[11];      /**< 子设备id */
}ColinkSubDeviceList;

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
 * @retval COLINK_GATEWAY_NO_ERROR          注册成功。
 * @retval COLINK_GATEWAY_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAY_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAY_DEV_TYPE_ERROR    设备不属于网关类型。
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
 * @retval COLINK_GATEWAY_NO_ERROR          删除成功。
 * @retval COLINK_GATEWAY_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAY_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAY_DEV_TYPE_ERROR    设备不属于网关类型。
 */
ColinkGatewayErrorCode colinkDelSubDev(ColinkSubDevice dev_list[], uint16_t dev_num);

/**
 * @brief 上报子设备在线。
 *
 * @par 描述:
 * 上报多个子设备在线。
 *
 * @param dev_list       [IN] 子设备数组的指针。
 * @param dev_num        [IN] 子设备数组的数量。
 *
 * @retval COLINK_GATEWAY_NO_ERROR          操作成功。
 * @retval COLINK_GATEWAY_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAY_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAY_DEV_TYPE_ERROR    设备不属于网关类型。
 */
ColinkGatewayErrorCode colinkOnlineSubDev(ColinkSubDevice dev_list[], uint16_t dev_num);

/**
 * @brief 上报子设备离线。
 *
 * @par 描述:
 * 上报多个子设备离线。
 *
 * @param dev_list       [IN] 子设备数组的指针。
 * @param dev_num        [IN] 子设备数组的数量。
 *
 * @retval COLINK_GATEWAY_NO_ERROR          操作成功。
 * @retval COLINK_GATEWAY_ARG_INVALID       传递参数无效。
 * @retval COLINK_GATEWAY_NET_ERROR         网络异常。
 * @retval COLINK_GATEWAY_DEV_TYPE_ERROR    设备不属于网关类型。
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
 * @param req_sequence  [IN] 传入colinkSubDevRecvReqCb获取的req_sequence。
 *
 * @retval COLINK_NO_ERROR              发送成功。
 * @retval COLINK_ARG_INVALID           无效的参数。
 * @retval COLINK_JSON_INVALID          无效的JSON格式。
 * @retval COLINK_JSON_CREATE_ERR       创建JSON对象错误。
 * @retval COLINK_DATA_SEND_ERROR       发送数据出错。
 * @retval COLINK_DEV_TYPE_ERROR        设置设备类型不正确。
 */
ColinkErrorCode colinkSubDevSendRes(ColinkSubDevice *sub_dev, ColinkSubDevResultCode error_code, char req_sequence[]);

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

/**
 * @brief 添加多个子设备到子设备列表。
 *
 * @par 描述:
 * 添加多个子设备到本地子设备列表，用于重启时添加保存在flash中的子设备列表，需
 * 在初始化（colinkInit）之后调用。
 *
 * @param list       [IN] 欲添加进本地子设备列表的子设备列表。
 * @param num        [IN] 子设备数量。
 *
 * @retval 0          全部添加成功。
 * @retval 大于0      已添加成功的子设备数量。
 */
uint16_t colinkGatewayAddSubDev(ColinkSubDeviceList *list, uint16_t num);

/**
 * @brief 获取当前子设备数量。
 *
 * @par 描述:
 * 获取当前网关设备上子设备的数量。
 *
 * @param 无
 *
 * @retval  当前子设备数量。
 */
uint16_t colinkGatewayGetSubDevNum(void);

/**
 * @brief 获取当前子设备信息列表。
 *
 * @par 描述:
 * 获取当前网关设备上的子设备信息列表。
 *
 * @param list       [IN/OUT] 子设备信息列表缓冲区，由开发者根据实际需求分配空间。
 * @param index      [IN]     子设备信息i列表读取起始位置
 * @param num        [IN]     获取的子设备信息数量
 *
 * @retval 成功获取的子设备信息数量。
 */
uint16_t colinkGatewayGetSubDevList(ColinkSubDeviceList *list, uint16_t index, uint16_t num);


#endif  /* __COLINK_GATEWAY_PROFILE_H__ */

