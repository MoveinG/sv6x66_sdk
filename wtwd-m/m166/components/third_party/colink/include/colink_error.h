/** 
 * @file     colink_error.h
 */
#ifndef __COLINK_ERROR_H__
#define __COLINK_ERROR_H__

typedef enum
{
    COLINK_INIT_NO_ERROR                = 0,        /**< 无错误 */
    COLINK_INIT_ARG_INVALID             = -2,       /**< 无效的参数 */
}ColinkInitErrorCode;

typedef enum
{
    COLINK_NO_ERROR                     = 0,        /**< 无错误 */
    COLINK_ARG_INVALID                  = -2,       /**< 无效的参数 */
    COLINK_JSON_INVALID                 = -3,       /**< 无效的JSON格式 */
    COLINK_JSON_CREATE_ERR              = -4,       /**< 创建JSON对象错误 */
    COLINK_DATA_SEND_ERROR              = -5,       /**< 发送数据出错 */
    COLINK_DEV_TYPE_ERROR               = -6,       /**< 设备类型错误 */
}ColinkErrorCode;

typedef enum
{
     COLINK_PROCESS_NO_ERROR            = 0,        /**< 无错误 */
     COLINK_PROCESS_INIT_INVALID        = -12,      /**< colinkInit未初始化成功 */
     COLINK_PROCESS_TIMEOUT             = -13,      /**< 长时间未被调用colinkProcess */
     COLINK_PROCESS_MEMORY_ERROR        = -14,      /**< 内存分配错误 */
}ColinkProcessErrorCode;

typedef enum
{
    COLINK_TCP_NO_ERROR                 = 0,        /**< 无错误 */
    COLINK_TCP_ARG_INVALID              = -2,       /**< 无效的参数 */
    COLINK_TCP_CREATE_CONNECT_ERR       = -21,      /**< 创建TCP连接错误 */
    COLINK_TCP_SEND_ERR                 = -23,      /**< TCP发送失败 */
    COLINK_TCP_READ_ERR                 = -25,      /**< TCP读取失败 */
    COLINK_TCP_CONNECT_TIMEOUT          = -26,      /**< TCP连接超时 */
    COLINK_TCP_CONNECT_ERR              = -27,      /**< TCP连接失败 */
    COLINK_TCP_CONNECTING               = -28,      /**< TCP连接中    */
    COLINK_TCP_READ_INCOMPLETED         = -29,      /**< TCP读包不完整 */
    COLINK_GET_HOSTBYNAME_ERR           = -30,      /**< 获取域名失败 */
}ColinkTcpErrorCode;

typedef enum
{
    COLINK_OTA_NO_ERROR            = 0,         /**< OTA升级成功 */
    COLINK_OTA_DOWNLOAD_ERROR      = 404,       /**< OTA文件下载失败 */
    COLINK_OTA_MODEL_ERROR         = 406,       /**< 设备型号不正确 */
    COLINK_OTA_DIGEST_ERROR        = 409,       /**< 固件校验失败 */
}ColinkOtaResCode;

typedef enum
{
    COLINK_LINK_NO_ERROR            = 0,         /**< 无错误 */
    COLINK_LINK_ARG_INVALID         = -2,        /**< 无效的参数 */
    COLINK_LINK_OPERATION_ERROR     = -50,       /**< 流程操作不对 */
}ColinkLinkErrorCode;

typedef enum
{
    COLINK_GATEWAT_NO_ERROR         = 0,         /**< 无错误 */
    COLINK_GATEWAT_ARG_INVALID      = -2,        /**< 无效的参数 */
    COLINK_GATEWAT_NET_ERROR        = -100,      /**< 网络异常 */
    COLINK_GATEWAT_DEV_TYPE_ERROR   = -101,      /**< 设备不属于网关类型 */
}ColinkGatewayErrorCode;

typedef enum
{
    COLINK_SUB_DEV_NO_ERROR         = 0,         /**< 操作成功 */
    COLINK_SUB_DEV_BAD_REQUEST      = 400,       /**< 参数错误 */
    COLINK_SUB_DEV_AUTH_ERROR       = 401,       /**< 认证不通过 */
    COLINK_SUB_DEV_NO_PERMISSION    = 403,       /**< 无权限 */
    COLINK_SUB_DEV_NUMBER_ERROR     = 414,       /**< 子设备数量超过限制 */
    COLINK_SUB_DEV_SERVER_ERROR     = 500,       /**< 服务器内部错误 */
}ColinkSubDevResultCode;

#endif /* #ifndef __COLINK_ERROR_H__ */
