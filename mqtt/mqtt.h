#ifndef __MQTT_H__
#define __MQTT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "mqtt_hal.h"

/* ============================================================
 * 宏定义与枚举
 * ============================================================ */

/* MQTT 服务质量等级 (QoS) 定义 */
#define MQTT_QOS0 0 /* 最多一次 (Fire and Forget) */
#define MQTT_QOS1 1 /* 至少一次 (需要 PUBACK) */
#define MQTT_QOS2 2 /* 只有一次 (不支持) */

/* 协议规定的最大变长长度字节数 (4字节可表示 256MB) */
#define MQTT_MAX_VAR_LEN 4

/* MQTT 操作类型枚举 */
typedef enum
{
    MQTT_OP_CONNECT = 0, /* 连接请求 */
    MQTT_OP_SUBSCRIBE,   /* 订阅主题 */
    MQTT_OP_UNSUBSCRIBE, /* 取消订阅 */
    MQTT_OP_PUBLISH,     /* 发布消息 */
    /* MQTT_OP_PUBACK 已移除，完全由接收线程自动处理 */
    MQTT_OP_PING,      /* 心跳请求 */
    MQTT_OP_DISCONNECT /* 断开连接 */
} MQTT_Operation;

/* 异步等待状态枚举 (用于线程间同步) */
typedef enum
{
    MQTT_WAIT_IDLE = 0, /* 空闲：当前无等待任务 */
    MQTT_WAIT_BUSY,     /* 忙碌：已发送请求，正在等待 ACK */
    MQTT_WAIT_SUCCESS,  /* 成功：接收任务已收到预期的 ACK */
    MQTT_WAIT_TIMEOUT   /* 超时：等待时间耗尽 (通常由发送方判断) */
} MQTT_WaitState;

/* ============================================================
 * 核心数据结构
 * ============================================================ */

/* MQTT 客户端句柄结构体 */
typedef struct
{
    /* --- 通信缓冲区 --- */
    uint8_t *txBuf;   /* 发送缓冲区指针 (用于构建报文) */
    size_t txBufSize; /* 发送缓冲区大小 */
    uint8_t *rxBuf;   /* 接收缓冲区指针 (用于存放 HAL_Recv 的原始数据) */
    size_t rxBufSize; /* 接收缓冲区大小 */

    /* --- 消息解析缓冲区 (新增：防止栈溢出) --- */
    uint8_t *msgTopicBuf;     /* 用于存放解析出的下行 Topic */
    size_t msgTopicBufSize;   /* Topic 缓冲区大小 */
    char *msgPayloadBuf;      /* 用于存放解析出的下行 Payload */
    size_t msgPayloadBufSize; /* Payload 缓冲区大小 */

    /* --- 连接参数 --- */
    const char *clientId; /* 客户端 ID */
    const char *userName; /* 用户名 (可选) */
    const char *password; /* 密码 (可选) */
    uint16_t keepAlive;   /* 心跳间隔 (秒) */
    uint8_t cleanSession; /* 1=不保留会话, 0=保留会话 */

    /* --- 传输控制参数 --- */
    uint32_t retryIntervalMs; /* 等待 ACK 的超时时间/重试间隔 (毫秒) */
    uint8_t maxRetrys;        /* 最大重试次数 */

    /* --- 状态管理 --- */
    bool isConnected;        /* 连接状态标志 */
    uint32_t lastActiveTick; /* 最后一次成功收发的时间戳 */
    uint16_t lastRxPacketId; /* 接收去重记录 */

    /* --- 当前操作上下文 (由发送方设置) --- */
    const char *pubTopic; /* 要发布的主题 */
    const char *pubMsg;   /* 要发布的消息内容 */
    const char *subTopic; /* 要订阅/退订的主题 */
    uint16_t packetId;    /* 当前报文 ID */
    uint8_t qos;          /* QoS 等级 */
    uint8_t retain;       /* 保留标志 */

    /* --- 线程同步标志 (核心机制) --- */
    /* 发送方设置这些值，接收方检查这些值 */
    volatile MQTT_WaitState waitState; /* 当前等待状态 */
    volatile MQTT_Operation awaitType; /* 期待收到的 ACK 类型 (如 OP_PUBLISH -> 等待 PUBACK) */
    volatile uint16_t awaitPacketId;   /* 期待的 Packet ID */

} MQTT_Client;

/* --------------------------------------------------------------------------
    * MQTT 协议栈 API
    * -------------------------------------------------------------------------- */

/* --- 报文构建函数 --- */
uint32_t MQTT_BuildConnectPacket(uint8_t *txBuf, size_t txBufSize, const char *clientId, const char *userName, const char *password, uint16_t keepAlive, uint8_t cleanSession);
uint32_t MQTT_BuildSubscribePacket(uint8_t *txBuf, size_t txBufSize, const char *topic, uint16_t packetId, uint8_t qos);
uint32_t MQTT_BuildUnsubscribePacket(uint8_t *txBuf, size_t txBufSize, const char *topic, uint16_t packetId);
uint32_t MQTT_BuildPublishPacket(uint8_t *txBuf, size_t txBufSize, const char *topic, const char *msg, uint16_t packetId, uint8_t dup, uint8_t qos, uint8_t retain);
uint32_t MQTT_BuildPingReqPacket(uint8_t *txBuf, size_t txBufSize);
uint32_t MQTT_BuildDisconnectPacket(uint8_t *txBuf, size_t txBufSize);

/* --- 报文校验函数 --- */
bool MQTT_CheckConnAck(const uint8_t *rxBuf, size_t rxLen);
bool MQTT_CheckSubAck(const uint8_t *rxBuf, size_t rxLen, uint16_t packetId, uint8_t qos);
bool MQTT_CheckUnsubAck(const uint8_t *rxBuf, size_t rxLen, uint16_t packetId);
bool MQTT_CheckPubAck(const uint8_t *rxBuf, size_t rxLen, uint16_t packetId);
bool MQTT_CheckPingResp(const uint8_t *rxBuf, size_t rxLen);

/* --- 报文解析函数 --- */
int MQTT_ParsePublishMessage(const uint8_t *rxBuf, size_t rxLen, uint8_t *recvTopic, size_t recvTopicSize, char *payload, size_t payloadSize, uint16_t *packetId);

/* --- 核心业务函数 --- */

/* 发送操作 (生产者)：只负责构建报文、发送、并阻塞等待 waitState 变更为成功 */
bool MQTT_TryOperation(MQTT_Client *client, MQTT_Operation op);

/* 接收循环 (消费者)：负责接收数据、自动回复 ACK、并更新 waitState 标志 */
void MQTT_ProcessLoop(MQTT_Client *client);

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_H__ */