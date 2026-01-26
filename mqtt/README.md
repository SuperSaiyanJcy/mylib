# Lightweight Embedded MQTT Client

这是一个专为嵌入式系统（STM32, ESP32, FreeRTOS, Bare-metal）设计的高性能、轻量级 MQTT v3.1.1 客户端库。

它采用了 **生产者-消费者（异步收发分离）** 架构，解决了传统阻塞式 MQTT 库在嵌入式环境中导致系统响应迟缓的问题，同时保证了 QoS 1 消息的可靠传输和自动化的心跳维护。

---

## ✨ 核心特性 (Features)

* **异步收发分离架构**：
    * **发送 (Producer)**：`MQTT_TryOperation` 负责构建报文并阻塞等待 ACK，支持超时重试，确保指令送达。
    * **接收 (Consumer)**：`MQTT_ProcessLoop` 负责非阻塞轮询，处理下行消息、自动回复 PUBACK、自动维护心跳。
* **零动态内存**：无 `malloc`/`free`，所有缓冲区由用户提供（静态分配），彻底杜绝内存碎片。
* **自动化协议管理**：
    * **自动心跳**：后台自动监测空闲时间并发送 PINGREQ，无需上层干预。
    * **自动 PUBACK**：收到 QoS 1 消息立即自动回复，防止服务器重发。
    * **智能去重**：自动过滤重复收到的 QoS 1 消息。
    * **PacketID 管理**：发送时自动自增，自动处理回绕。
* **线程安全设计**：明确的 HAL 层锁机制要求，支持多任务并发调用。

---

## 📂 文件结构

| 文件         | 说明                                       |
| :----------- | :----------------------------------------- |
| `mqtt.h`     | 核心头文件，定义结构体、枚举和 API。       |
| `mqtt.c`     | 协议栈核心实现（报文构建、解析、状态机）。 |
| `mqtt_hal.h` | 硬件抽象层接口声明。                       |
| `mqtt_hal.c` | 硬件抽象层弱定义实现（用户需重写此文件）。 |

---

## 🛠️ 移植指南 (Porting)

你需要根据你的硬件平台（如 STM32 HAL库, LwIP, UART 等）实现以下接口。

### ⚠️ 重要提示：线程安全
由于 **业务发送线程** 和 **后台接收线程** 可能会同时调用发送接口，**`HAL_MQTT_Send` 必须保证线程安全**。

```c
/* mqtt_hal_port.c */

// [必须实现] 发送数据 (必须使用互斥锁/临界区保护)
void HAL_MQTT_Send(const uint8_t *buf, size_t len) {
    // FreeRTOS 示例:
    xSemaphoreTake(xMutexTx, portMAX_DELAY);
    UART_Send_Bytes(buf, len); 
    xSemaphoreGive(xMutexTx);
}

// [必须实现] 接收数据 (建议带超时机制)
// 返回值：实际读取到的字节数，0 表示无数据/超时
int HAL_MQTT_Recv(uint8_t *buf, size_t bufSize, uint32_t timeoutMs) {
    return UART_Recv_Bytes(buf, bufSize, timeoutMs);
}

// [必须实现] 获取系统时间戳 (单位：毫秒)
uint32_t HAL_MQTT_GetTick(void) {
    return HAL_GetTick(); // STM32 示例
}

// [可选] 日志打印
void HAL_MQTT_Log(const char *fmt, ...) {
    printf(fmt, ...);
}

// [可选] 毫秒延时
void HAL_MQTT_Delay(uint32_t ms) {
    osDelay(ms); // FreeRTOS 示例
}

// [业务回调] 当收到云端推送的消息时触发
void HAL_MQTT_OnPublishReceived(const char *topic, const char *payload, size_t len) {
    printf(">> Recv Topic: %s\n", topic);
    printf(">> Payload: %.*s\n", (int)len, payload);
}
```
---

## :rocket:使用示例（Usage）

**1. 初始化客户端**

定义缓冲区并初始化结构体：

```c
#include "mqtt.h"

// 定义缓冲区 (大小根据实际业务调整)
uint8_t tx_buffer[1024];
uint8_t rx_buffer[1024];
uint8_t msg_topic_buf[128];  // 用于解析下行 Topic
char    msg_payload_buf[512]; // 用于解析下行 Payload

MQTT_Client mqttClient;

void MQTT_Init(void) {
    memset(&mqttClient, 0, sizeof(MQTT_Client));
    
    // 1. 绑定缓冲区
    mqttClient.txBuf = tx_buffer;
    mqttClient.txBufSize = sizeof(tx_buffer);
    mqttClient.rxBuf = rx_buffer;
    mqttClient.rxBufSize = sizeof(rx_buffer);
    mqttClient.msgTopicBuf = msg_topic_buf;
    mqttClient.msgTopicBufSize = sizeof(msg_topic_buf);
    mqttClient.msgPayloadBuf = msg_payload_buf;
    mqttClient.msgPayloadBufSize = sizeof(msg_payload_buf);

    // 2. 配置连接参数
    mqttClient.clientId = "Device_ID_001";
    mqttClient.userName = "admin";
    mqttClient.password = "password";
    mqttClient.keepAlive = 60;       // 60秒心跳
    mqttClient.cleanSession = 1;
    
    // 3. 配置重试策略
    mqttClient.retryIntervalMs = 2000; // 等待 ACK 超时 2秒
    mqttClient.maxRerys = 3;           // 失败重试 3次
}
```
**RTOS双任务模式（推荐）**

这是最高效的运行方式：一个任务负责“守护”（心跳+接收），另一个任务负责“业务”（发送）。

**任务 A：接收守护进程 (Receiver Task)**
```c
void Task_MQTT_Process(void *arg) {
    while (1) {
        // 核心函数：负责接收数据、自动回复 PUBACK、维持心跳
        // 建议 HAL_MQTT_Recv 内部有一定的阻塞超时 (如 50ms) 以释放 CPU
        MQTT_ProcessLoop(&client);
        
        // 如果 ProcessLoop 内部是非阻塞的，这里需要加一点延时
        // osDelay(10); 
    }
}
```
**任务 B：业务逻辑进程 (Sender Task)**
```c
void Task_MQTT_Logic(void *arg) {
    App_MQTT_Init();

    // 1. 发起连接 (阻塞直到成功或重试失败)
    while (!MQTT_TryOperation(&client, MQTT_OP_CONNECT)) {
        HAL_MQTT_Log("Connect failed, retrying...\n");
        HAL_MQTT_Delay(1000);
    }
    HAL_MQTT_Log("MQTT Connected!\n");

    // 2. 订阅主题 (阻塞等待 SUBACK)
    client.packetId = 1; // 初始化 ID
    client.subTopic = "cmd/control";
    client.qos = 1;
    MQTT_TryOperation(&client, MQTT_OP_SUBSCRIBE);

    while (1) {
        // 3. 周期性上报数据
        client.pubTopic = "data/sensor";
        client.pubMsg = "{\"temp\": 25.6}";
        client.qos = 1; // QoS 1 需要等待 PUBACK
        
        // 注意：packetId 会在 TryOperation 内部自动自增
        if (MQTT_TryOperation(&client, MQTT_OP_PUBLISH)) {
            HAL_MQTT_Log("Publish Success\n");
        } else {
            HAL_MQTT_Log("Publish Failed (Timeout)\n");
            // 可选：检测到断开后执行重连逻辑
        }

        HAL_MQTT_Delay(5000); // 5秒发一次
    }
}
```

**3. 裸机模式 (Bare Metal)**

在主循环中交替执行处理和发送。
```c
int main(void) {
    HAL_Init();
    App_MQTT_Init();
    
    // 建立连接
    if (MQTT_TryOperation(&client, MQTT_OP_CONNECT)) {
        // 订阅
        client.subTopic = "cmd";
        client.qos = 1;
        MQTT_TryOperation(&client, MQTT_OP_SUBSCRIBE);
    }

    uint32_t last_pub_tick = 0;

    while (1) {
        // 1. 必须高频调用，负责接收、心跳和 ACK 回复
        MQTT_ProcessLoop(&client);

        // 2. 业务发送 (非阻塞检查时间)
        if (HAL_GetTick() - last_pub_tick > 10000) {
            client.pubTopic = "status";
            client.pubMsg = "alive";
            client.qos = 0; // QoS 0 发送极快，不等待 ACK
            
            MQTT_TryOperation(&client, MQTT_OP_PUBLISH); 
            
            last_pub_tick = HAL_GetTick();
        }
    }
}
```
---
## ⚙️ 核心 API 说明

`MQTT_TryOperation(client, op)`

**- 角色：**生产者 (Producer)。
**- 行为：**构建报文 -> 发送 -> 阻塞等待 接收线程通知结果 (ACK)。
**- QoS 0：**发送即返回成功，不等待。
**- PacketID：**对于 PUBLISH 操作，函数内部会自动处理 ID 自增。

`MQTT_ProcessLoop(client)`

**- 角色：**消费者 (Consumer)。
**- 行为：**读取数据 -> 解析报文。
  **- 收到 PUBLISH：**立即回复 PUBACK (QoS 1)，回调用户函数。
  **- 收到 ACK：**更新状态标志，唤醒正在阻塞的 TryOperation。
  **- 空闲时：**检查是否需要发送心跳 (PINGREQ)。

`HAL_MQTT_OnPublishReceived(topic, payload, len)`

**- 触发时机：**当收到服务器推送的 PUBLISH 消息，且校验通过（非重复）时。
**- 注意：**此函数在 MQTT_ProcessLoop 线程上下文中执行，请勿在此函数中执行耗时过长的操作。

