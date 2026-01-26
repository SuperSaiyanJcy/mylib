#ifndef __MQTT_HAL_H__
#define __MQTT_HAL_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 编译器兼容性宏定义 (用于弱定义 Weak 属性)
 * ============================================================ */
#if defined(__GNUC__) || defined(__clang__)
#define __WEAK __attribute__((weak))
#elif defined(__ICCARM__) || defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define __WEAK __attribute__((weak))
#else
#define __WEAK
#endif

/* 发送数据接口 */
void HAL_MQTT_Send(const uint8_t *buf, size_t len);

/* 接收数据接口 (阻塞或带超时读取)
 * 返回值: >0 实际读取字节数; =0 超时; <0 错误
 */
int HAL_MQTT_Recv(uint8_t *buf, size_t bufSize, uint32_t timeoutMs);

/* 日志打印接口 */
void HAL_MQTT_Log(const char *fmt, ...);

/* 时间戳获取与延时接口 */
uint32_t HAL_MQTT_GetTick(void);

/* 延时接口 */
void HAL_MQTT_Delay(uint32_t ms);

/* 初始化接口 */
void HAL_MQTT_Init(void);

/* 接收到 PUBLISH 报文时的回调函数 */
void HAL_MQTT_OnPublishReceived(const char *topic, const char *payload, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_HAL_H__ */
