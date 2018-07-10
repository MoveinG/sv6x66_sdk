#ifndef __COLINK_PROFILE_H__
#define __COLINK_PROFILE_H__

#include <stdint.h>
#include "colink_typedef.h"
#include "colink_error.h"

/**
 * 设备信息的结构体，由酷宅提供。
 */
typedef struct
{
    char deviceid[11];         /**< 设备ID */
    char apikey[37];           /**< 设备密钥 */
    char model[11];            /**< 设备型号 */
    char distor_domain[32];    /**< 分配服务器域名，由APP下发给设备 */
    char version[12];          /**< 固件版本 */
    bool ssl_enable;           /**< 是否使能SSL */
    int __pad[2];
}ColinkDev;

/**
 * 设备上下线枚举类型。
 */
typedef enum
{
    DEVICE_OFFLINE   = 0,    /**< 设备离线 */
    DEVICE_ONLINE    = 1,    /**< 设备在线 */
} ColinkDevStatus;

/**
 * OTA固件信息的结构体。
 */
typedef struct
{
    char name[20];             /**< 固件名称 */
    char download_url[150];    /**< 下载链接 */
    char digest[65];           /**< 固件的SHA256值 */
}ColinkOtaInfo;

/**
 * 设备事件回调的结构体。
 */
typedef struct
{
    /**
     * @brief 接收update字段的回调函数。
     *
     * @par 描述:
     * 当APP需要改变设备状态时，通过此回调函数获取数据。
     *
     * @param data     [IN] 收到的字符串数据，以Json格式表示。
     *
     */
    void (*colinkRecvUpdate)(char* data);
    
    /**
     * @brief 设备状态发生改变时的回调函数。
     *
     * @par 描述:
     * 当设备状态发生改变时会产生回调函数。
     *
     * @param status   [IN] 设备状态 在线：DEVICE_ONLINE 离线：DEVICE_OFFLINE。
     *
     */
    void (*colinkNotifyDevStatus)(ColinkDevStatus status);

    /**
     * @brief 升级通知的回调函数。
     *
     * @par 描述:
     * 当APP发出升级通知后，通过此回调函数获取升级信息。
     * 当升级完成后需要调用colinkUpgradeResponse通知服务器升级完成。
     *
     * @param new_ver       [IN] 新固件的版本号。
     * @param file_list     [IN] OTA固件信息列表。
     * @param file_num      [IN] OTA固件的数量。
     *
     */
    void (*colinkUpgradeRequestCallback)(char *new_ver, ColinkOtaInfo file_list[], uint8_t file_num);
}ColinkEvent;

/**
 * @brief 初始化colink Device。
 *
 * @par 描述:
 * 初始化colink设备信息和注册回调事件。
 *
 * @param dev_data       [IN] 设备信息结构体指针。
 * @param reg_event      [IN] 注册回调事件结构体指针。
 *
 * @retval COLINK_INIT_NO_ERROR      初始化成功。
 * @retval COLINK_INIT_ARG_INVALID   传入的参数无效。
 */
ColinkInitErrorCode colinkInit(ColinkDev *dev_data, ColinkEvent *reg_event);

/**
 * @brief colink事件处理。
 *
 * @par 描述:
 * 启动colink事件处理，必须在colinkInit初始化成功后调用！
 * 建议调用间隔最大25~50毫秒。若该函数长时间未被调用，则可能返回异常信息。
 *
 * @param 无。
 *
 * @retval COLINK_PROCESS_NO_ERROR       运行正常。
 * @retval COLINK_PROCESS_INIT_INVALID   colinkInit未初始化成功。
 * @retval COLINK_PROCESS_TIMEOUT        长时间未被调用colinkProcess。
 * @retval COLINK_PROCESS_MEMORY_ERROR   内存分配错误。
 
 */
ColinkProcessErrorCode colinkProcess(void);

/**
 * @brief 发送update字段的数据。
 *
 * @par 描述:
 * 当设备状态发生改变时主动发到云平台，云平台再转发到APP同步设备状态。
 *
 * @param data   [IN] 以Json格式表示的字符串数据。
 *
 * @retval COLINK_NO_ERROR          响应成功。
 * @retval COLINK_ARG_INVALID       无效的参数。
 * @retval COLINK_JSON_INVALID      无效的JSON格式。
 * @retval COLINK_JSON_CREATE_ERR   创建JSON对象错误。
 * @retval COLINK_DATA_SEND_ERROR   发送数据出错。
 */
ColinkErrorCode colinkSendUpdate(char* data);

/**
 * @brief 获取设备的状态。
 *
 * @par 描述:
 * 获取设备的离线上线状态，开发者通过返回值作相应处理。
 *
 * @param 无。
 *
 * @retval DEVICE_OFFLINE     离线。
 * @retval DEVICE_ONLINE      在线。
 */
ColinkDevStatus colinkGetDevStatus(void); 

/**
 * @brief 响应升级的结果。
 *
 * @par 描述:
 * 当收到升级通知后（即colinkUpgradeRequestCallback），
 * 升级完成需要调用此接口来响应服务器是否升级成功的结果。
 *
 * @param error_code   [IN] 响应错误码值。参考ColinkOtaResCode。
 *
 * @retval COLINK_NO_ERROR          响应成功。
 * @retval COLINK_ARG_INVALID       无效的参数。
 * @retval COLINK_JSON_CREATE_ERR   创建JSON对象错误。
 * @retval COLINK_DATA_SEND_ERROR   发送数据出错。
 */
ColinkErrorCode colinkUpgradeResponse(ColinkOtaResCode error_code);

/**
 * @brief 获取colink版本号。
 *
 * @par 描述:
 * 获取colink版本号。
 *
 * @param 无。
 *
 * @retval 非NULL     成功获取colink版本。
 * @retval NULL       获取版本失败。
 */
const char *colinkGetVersion(void);

#endif