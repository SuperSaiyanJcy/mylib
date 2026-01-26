#include "mqtt_hal.h"

__WEAK void HAL_MQTT_Init(void)
{
    /* 用户在应用层实现：UART 初始化 / DMA / 网卡等 */
}

/*
 * [重要提示 / IMPORTANT]
 * HAL_MQTT_Send 必须保证线程安全 (Thread-Safe)。
 *
 * 原因：
 * 本 MQTT 库采用了异步收发分离架构：
 * 1. 主发送线程调用 MQTT_TryOperation 发送业务报文。
 * 2. 接收线程调用 MQTT_ProcessLoop -> HandleIncoming 自动回复 PUBACK。
 *
 * 这两个线程可能会同时调用 HAL_MQTT_Send。
 * 如果底层硬件（如 UART/DMA）不支持并发写入，您必须在此函数内部
 * 使用互斥锁 (Mutex) 或 临界区 (Critical Section) 进行保护。
 *
 * 示例 (FreeRTOS):
 * void HAL_MQTT_Send(const uint8_t *buf, size_t len) {
 * xSemaphoreTake(xMutexTx, portMAX_DELAY);
 * UART_Send_DMA(buf, len);
 * xSemaphoreGive(xMutexTx);
 * }
 */
__WEAK void HAL_MQTT_Send(const uint8_t *data, size_t len)
{
    /* 用户在应用层实现：通过 UART / 网卡 等发送数据 */
}

__WEAK int HAL_MQTT_Recv(uint8_t *buf, size_t bufSize, uint32_t timeoutMs)
{
    /* 用户在应用层实现：通过 UART / 网卡 等接收数据 */
    return 0;
}

__WEAK void HAL_MQTT_Log(const char *fmt, ...)
{
    /* 用户在应用层实现：日志输出函数 */
}

__WEAK void HAL_MQTT_OnPublishReceived(const char *topic, const char *payload, size_t payloadLen)
{
    /* 用户在应用层实现：处理接收到的 PUBLISH 报文 */
}

__WEAK void HAL_MQTT_Delay(uint32_t ms)
{
    /* 用户在应用层实现：延时函数 */
}

__WEAK uint32_t HAL_MQTT_GetTick(void)
{
    /* 用户在应用层实现：获取系统时间戳 */
    return 0;
}
