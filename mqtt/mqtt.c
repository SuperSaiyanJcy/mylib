#include "mqtt.h"

/* ============================================================
 * 静态函数前置声明 (防止编译器报警)
 * ============================================================ */

static void MQTT_EncodeLength(uint32_t length, uint8_t *encoded, uint8_t *encodedLen);
static int MQTT_DecodeLength(const uint8_t *encoded, size_t rxlen, uint8_t *encodedLen, uint32_t *length, uint32_t maxLength);
static void MQTT_HandleIncoming(MQTT_Client *client, size_t len);
static void MQTT_SendPingRaw(void);
static void MQTT_MaintainKeepAlive(MQTT_Client *client);

/* --------------------------------------------------------------------------
 * 内部辅助函数
 * -------------------------------------------------------------------------- */

/* * 编码 MQTT 剩余长度字段 (Variable Byte Integer)
 * 算法：每字节低7位存数据，最高位(bit7)为延续标志(1表示后续还有字节)
 */
static void MQTT_EncodeLength(uint32_t length, uint8_t *encoded, uint8_t *encodedLen)
{
    *encodedLen = 0;
    do
    {
        /* 取 length 的低 7 位 */
        encoded[*encodedLen] = length % 128;
        length /= 128;

        /* 如果 length 还有剩余，将当前字节最高位置 1 */
        if (length > 0)
            encoded[*encodedLen] |= 0x80;

        (*encodedLen)++;
    } while (length > 0 && *encodedLen < 4); /* 限制最大 4 字节 */
}

/* * 解码 MQTT 剩余长度字段
 * 返回值: 0 成功, -1 超出最大值, -2 数据不足, -3 格式错误
 */
static int MQTT_DecodeLength(const uint8_t *encoded, size_t rxlen,
                             uint8_t *encodedLen, uint32_t *length, uint32_t maxLength)
{
    uint32_t multiplier = 1;
    *length = 0;
    *encodedLen = 0;
    uint8_t byte = 0;

    do
    {
        /* 检查是否越过接收缓冲区边界 */
        if (*encodedLen >= rxlen)
            return -2;

        byte = encoded[*encodedLen];
        /* 取低7位累加权重 */
        (*length) += (byte & 0x7F) * multiplier;

        /* 准备下一字节的权重 (x128) */
        multiplier *= 128;

        (*encodedLen)++;

        /* MQTT 协议规定长度字段最多 4 个字节 */
        if (*encodedLen > 4)
            return -3;

    } while ((byte & 0x80) != 0); /* 最高位为 1 继续读取 */

    /* 检查解析出的长度是否超过了应用层允许的最大值(通常是剩余缓冲区大小) */
    if (*length > maxLength)
        return -1;

    return 0;
}

/* --------------------------------------------------------------------------
 * 报文构建函数
 * -------------------------------------------------------------------------- */

/* 构建 CONNECT 报文 */
uint32_t MQTT_BuildConnectPacket(uint8_t *txBuf, size_t txBufSize,
                                 const char *clientId, const char *userName,
                                 const char *password, uint16_t keepAlive,
                                 uint8_t cleanSession)
{
    if (!txBuf || !clientId)
        return 0;

    size_t clientIdLen = strlen(clientId);
    size_t userNameLen = userName ? strlen(userName) : 0;
    size_t passwordLen = password ? strlen(password) : 0;

    /* 计算剩余长度:
     * 可变报头(10字节: ProtoName(6)+Lvl(1)+Flags(1)+KA(2)) +
     * 载荷(ClientID(2+len) + User(2+len)? + Pass(2+len)?)
     */
    uint32_t remainingLength = 10 + (2 + clientIdLen);
    if (userNameLen)
        remainingLength += (2 + userNameLen);
    if (passwordLen)
        remainingLength += (2 + passwordLen);

    uint8_t encodedLen = 0;
    /* 预估总长度: 固定头(1) + 长度字段(最大4) + 剩余长度 */
    if (1 + 4 + remainingLength > txBufSize)
        return 0;

    /* 1. 固定报头: 0x10 = CONNECT, 此时不带 Flag */
    txBuf[0] = 0x10;

    /* 2. 编码剩余长度，写入位置从 txBuf[1] 开始 */
    MQTT_EncodeLength(remainingLength, &txBuf[1], &encodedLen);

    /* 指针 p 指向可变报头的起始位置 */
    uint8_t *p = &txBuf[1 + encodedLen];

    /* 3. 可变报头: Protocol Name "MQTT" */
    *p++ = 0x00;
    *p++ = 0x04; // 长度 4
    *p++ = 'M';
    *p++ = 'Q';
    *p++ = 'T';
    *p++ = 'T';

    /* Protocol Level: 4 代表 v3.1.1 */
    *p++ = 0x04;

    /* Connect Flags 标志位构建 */
    uint8_t connectFlags = 0;
    if (userNameLen)
        connectFlags |= 0x80; // User Name Flag (bit 7)
    if (passwordLen)
        connectFlags |= 0x40; // Password Flag (bit 6)
    /* Will Retain(0), Will QoS(00), Will Flag(0) */
    if (cleanSession)
        connectFlags |= 0x02; // Clean Session Flag (bit 1) set to 1
    *p++ = connectFlags;

    /* Keep Alive (大端序) */
    *p++ = (uint8_t)(keepAlive >> 8);
    *p++ = (uint8_t)keepAlive;

    /* 4. 有效载荷 (Payload) */

    /* Client Identifier */
    *p++ = (uint8_t)(clientIdLen >> 8);
    *p++ = (uint8_t)clientIdLen;
    memcpy(p, clientId, clientIdLen);
    p += clientIdLen;

    /* User Name */
    if (userNameLen)
    {
        *p++ = (uint8_t)(userNameLen >> 8);
        *p++ = (uint8_t)userNameLen;
        memcpy(p, userName, userNameLen);
        p += userNameLen;
    }

    /* Password */
    if (passwordLen)
    {
        *p++ = (uint8_t)(passwordLen >> 8);
        *p++ = (uint8_t)passwordLen;
        memcpy(p, password, passwordLen);
        p += passwordLen;
    }

    /* 返回构建的总字节数 */
    return (uint32_t)(p - txBuf);
}

/* 构建 SUBSCRIBE 报文 当前实现仅支持单 Topic 订阅 */
uint32_t MQTT_BuildSubscribePacket(uint8_t *txBuf, size_t txBufSize,
                                   const char *topic, uint16_t packetId, uint8_t qos)
{
    if (!txBuf || !topic)
        return 0;

    size_t topicLen = strlen(topic);
    /* 剩余长度 = Packet Identifier(2) + Topic Length(2) + Topic + Requested QoS(1) */
    uint32_t remainingLength = 2 + 2 + topicLen + 1;

    uint8_t encodedLen = 0;
    if (1 + 4 + remainingLength > txBufSize)
        return 0;

    /* 固定报头: 0x82 (SUBSCRIBE = 8, bits 3-0 必须为 0010) */
    txBuf[0] = 0x82;
    MQTT_EncodeLength(remainingLength, &txBuf[1], &encodedLen);

    uint8_t *p = &txBuf[1 + encodedLen];

    /* 可变报头: Packet Identifier */
    *p++ = (uint8_t)(packetId >> 8);
    *p++ = (uint8_t)packetId;

    /* Payload: Topic Filter */
    *p++ = (uint8_t)(topicLen >> 8);
    *p++ = (uint8_t)topicLen;
    memcpy(p, topic, topicLen);
    p += topicLen;

    /* Payload: Requested QoS */
    *p++ = qos & 0x03;

    return (uint32_t)(p - txBuf);
}

/* 构建 UNSUBSCRIBE 报文 */
uint32_t MQTT_BuildUnsubscribePacket(uint8_t *txBuf, size_t txBufSize,
                                     const char *topic, uint16_t packetId)
{
    if (!txBuf || !topic)
        return 0;

    size_t topicLen = strlen(topic);
    /* 剩余长度 = Packet Identifier(2) + Topic Length(2) + Topic */
    /* UNSUBSCRIBE Payload 只包含 Topic Filter，不包含 QoS */
    uint32_t remainingLength = 2 + 2 + topicLen;

    uint8_t encodedLen = 0;
    if (1 + 4 + remainingLength > txBufSize)
        return 0;

    /* 固定报头: 0xA2 (UNSUBSCRIBE = 10, bits 3-0 必须为 0010) */
    /* Type(1010) | Reserved(0010) = 10100010 = 0xA2 */
    txBuf[0] = 0xA2;

    /* 编码剩余长度 */
    MQTT_EncodeLength(remainingLength, &txBuf[1], &encodedLen);

    uint8_t *p = &txBuf[1 + encodedLen];

    /* 可变报头: Packet Identifier */
    *p++ = (uint8_t)(packetId >> 8);
    *p++ = (uint8_t)packetId;

    /* Payload: Topic Filter */
    /* 包含长度前缀的字符串 */
    *p++ = (uint8_t)(topicLen >> 8);
    *p++ = (uint8_t)topicLen;
    memcpy(p, topic, topicLen);
    p += topicLen;

    /* UNSUBSCRIBE 报文没有 QoS 字段 */

    return (uint32_t)(p - txBuf);
}

/* 构建 PUBLISH 报文 */
uint32_t MQTT_BuildPublishPacket(uint8_t *txBuf, size_t txBufSize,
                                 const char *topic, const char *msg,
                                 uint16_t packetId, uint8_t dup,
                                 uint8_t qos, uint8_t retain)
{
    if (!txBuf || !topic || !msg)
        return 0;

    /* QoS 1 和 2 需要 Packet ID */
    if (qos > 0 && packetId == 0)
        return 0;

    size_t topicLen = strlen(topic);
    size_t msgLen = strlen(msg);

    /* 计算剩余长度: Topic Length(2) + Topic + [PacketID(2) if QoS>0] + Payload */
    uint32_t remainingLength = 2 + topicLen + msgLen;
    if (qos > 0)
        remainingLength += 2;

    uint8_t encodedLen = 0;
    if (1 + 4 + remainingLength > txBufSize)
        return 0;

    /* 固定报头 byte 1 */
    /* Bit 7-4: Type(3=PUBLISH) */
    /* Bit 3: DUP flag */
    /* Bit 2-1: QoS level */
    /* Bit 0: RETAIN flag */
    txBuf[0] = (uint8_t)(0x30 | ((dup & 0x01) << 3) | ((qos & 0x03) << 1) | (retain & 0x01));

    MQTT_EncodeLength(remainingLength, &txBuf[1], &encodedLen);
    uint8_t *p = &txBuf[1 + encodedLen];

    /* 可变报头: Topic Name */
    *p++ = (uint8_t)(topicLen >> 8);
    *p++ = (uint8_t)topicLen;
    memcpy(p, topic, topicLen);
    p += topicLen;

    /* 可变报头: Packet Identifier (仅当 QoS > 0 时存在) */
    if (qos > 0)
    {
        *p++ = (uint8_t)(packetId >> 8);
        *p++ = (uint8_t)packetId;
    }

    /* Payload: Message */
    memcpy(p, msg, msgLen);
    p += msgLen;

    return (uint32_t)(p - txBuf);
}

/* 构建 PINGREQ 报文 */
uint32_t MQTT_BuildPingReqPacket(uint8_t *txBuf, size_t txBufSize)
{
    if (!txBuf || txBufSize < 2)
        return 0;
    /* 0xC0 = PINGREQ, 0x00 = Remaining Length 0 */
    txBuf[0] = 0xC0;
    txBuf[1] = 0x00;
    return 2;
}

/* 构建 DISCONNECT 报文 */
uint32_t MQTT_BuildDisconnectPacket(uint8_t *txBuf, size_t txBufSize)
{
    if (!txBuf || txBufSize < 2)
        return 0;
    /* 0xE0 = DISCONNECT, 0x00 = Remaining Length 0 */
    txBuf[0] = 0xE0;
    txBuf[1] = 0x00;
    return 2;
}

/* --------------------------------------------------------------------------
 * 报文校验与解析函数
 * -------------------------------------------------------------------------- */

/* 检查 CONNACK 报文 */
bool MQTT_CheckConnAck(const uint8_t *rxBuf, size_t rxLen)
{
    /* 最小长度 4 字节 */
    if (rxLen < 4)
        return false;
    /* 0x20 = Type(CONNACK), 0x02 = Remaining Length */
    if (rxBuf[0] != 0x20 || rxBuf[1] != 0x02)
        return false;

    /* rxBuf[2]: Session Present Flag (忽略) */

    /* rxBuf[3]: Connect Return Code (0x00 表示 Accepted) */
    return (rxBuf[3] == 0x00);
}

/* 检查 SUBACK 报文 */
bool MQTT_CheckSubAck(const uint8_t *rxBuf, size_t rxLen, uint16_t packetId, uint8_t qos)
{
    /* 最小长度: Fixed(2) + PacketID(2) + ReturnCode(1) = 5 */
    if (rxLen < 5)
        return false;

    /* 0x90 = Type(SUBACK) */
    if (rxBuf[0] != 0x90)
        return false;

    /* 剩余长度应为 3 */
    if (rxBuf[1] != 0x03)
        return false;

    /* 校验 Packet Identifier */
    if (rxBuf[2] != (uint8_t)(packetId >> 8) || rxBuf[3] != (uint8_t)packetId)
        return false;

    /* 校验 Return Code */
    /* 0x80 表示 Failure */
    if (rxBuf[4] == 0x80)
        return false;

    /* 服务器授予的 QoS 可以低于请求的 QoS，但不应高于 */
    /* 0x00(Max QoS0), 0x01(Max QoS1), 0x02(Max QoS2) */
    return (rxBuf[4] <= qos);
}

/* 检查 UNSUBACK 报文 */
bool MQTT_CheckUnsubAck(const uint8_t *rxBuf, size_t rxLen, uint16_t packetId)
{
    /* 最小长度: Fixed(2) + PacketID(2) = 4 */
    if (rxLen < 4)
        return false;

    /* 0xB0 = Type(UNSUBACK) = 11, Reserved = 0000 */
    if (rxBuf[0] != 0xB0)
        return false;

    /* 剩余长度应为 2 */
    if (rxBuf[1] != 0x02)
        return false;

    /* 校验 Packet Identifier */
    if (rxBuf[2] != (uint8_t)(packetId >> 8) || rxBuf[3] != (uint8_t)packetId)
        return false;

    /* UNSUBACK 没有 payload，只要 ID 对得上即为成功 */
    return true;
}

/* 检查 PUBACK 报文 */
bool MQTT_CheckPubAck(const uint8_t *rxBuf, size_t rxLen, uint16_t packetId)
{
    /* 长度 4 字节 */
    if (rxLen < 4)
        return false;
    /* 0x40 = Type(PUBACK), 0x02 = Remaining Length */
    if (rxBuf[0] != 0x40 || rxBuf[1] != 0x02)
        return false;

    /* 校验 Packet Identifier */
    if (rxBuf[2] != (uint8_t)(packetId >> 8) || rxBuf[3] != (uint8_t)packetId)
        return false;

    return true;
}

/* 检查 PINGRESP 报文 */
bool MQTT_CheckPingResp(const uint8_t *rxBuf, size_t rxLen)
{
    if (rxLen < 2)
        return false;
    /* 0xD0 = Type(PINGRESP), 0x00 = Remaining Length */
    return (rxBuf[0] == 0xD0 && rxBuf[1] == 0x00);
}

/* * 解析收到的 PUBLISH 报文
 * 返回值: >=0 Payload长度; -1 报文头错误; -2 数据未收全; -3 格式逻辑错误; -4 缓冲区不足
 */
int MQTT_ParsePublishMessage(const uint8_t *rxBuf, size_t rxLen,
                             uint8_t *recvTopic, size_t recvTopicSize,
                             char *payload, size_t payloadSize,
                             uint16_t *packetId)
{
    uint32_t remainingLength = 0;
    uint8_t decodedLen = 0;

    if (rxLen < 2)
        return -1;

    /* 检查 Fixed Header: 高4位是否为 3 (PUBLISH = 0x30 ~ 0x3F) */
    if ((rxBuf[0] & 0xF0) != 0x30)
        return -1;

    /* 解码剩余长度字段，同时检查是否越界 */
    if (MQTT_DecodeLength(&rxBuf[1], rxLen - 1, &decodedLen, &remainingLength, rxLen - 2) != 0)
        return -1;

    /* 校验：当前缓冲区数据总长度必须包含完整的 MQTT 报文 */
    /* 1(Fixed Type) + decodedLen(Length Bytes) + remainingLength(Body) */
    if (rxLen < 1 + decodedLen + remainingLength)
        return -2;

    /* 指针移动到可变报头开始处 */
    const uint8_t *p = &rxBuf[1 + decodedLen];

    /* 获取 QoS 标志位 */
    uint8_t qos = (rxBuf[0] & 0x06) >> 1;

    /* --- 开始解析可变报头 --- */

    /* 1. 读取 Topic Length */
    /* 确保剩余长度至少包含 2 字节的 Topic Length */
    if (remainingLength < 2)
        return -3;

    uint32_t topicLen = (p[0] << 8) | p[1];

    /* 计算可变报头的元数据长度: TopicLen字段(2) + [PacketID(2)] */
    uint32_t headerMetaLen = 2;
    if (qos > 0)
        headerMetaLen += 2;

    /* 安全检查: Topic 内容长度 + 元数据长度 不能超过 剩余长度 */
    /* 这是一个关键检查，防止 topicLen 伪造导致指针越界 */
    if (topicLen + headerMetaLen > remainingLength)
        return -3;

    /* 2. 提取 Topic */
    /* 限制拷贝长度，防止 recvTopic 溢出 */
    if (recvTopicSize == 0)
        return -4;
    size_t copyLen = (topicLen < recvTopicSize - 1) ? topicLen : (recvTopicSize - 1);
    memcpy(recvTopic, &p[2], copyLen);
    recvTopic[copyLen] = 0; /* 补零 */

    /* 移动指针越过 Topic 字段 */
    p += (2 + topicLen);

    /* 3. 提取 Packet Identifier (若 QoS > 0) */
    if (qos > 0)
    {
        *packetId = (p[0] << 8) | p[1];
        p += 2;
    }
    else
    {
        *packetId = 0;
    }

    /* --- 开始解析 Payload --- */

    /* 计算 Payload 长度 */
    /* Payload = 剩余长度 - (Topic长度字段 + Topic内容 + PacketID字段) */
    uint32_t payloadLen = remainingLength - (2 + topicLen + (qos > 0 ? 2 : 0));

    /* 检查用户缓冲区是否足够 */
    if (payloadLen >= payloadSize)
        return -4;

    memcpy(payload, p, payloadLen);
    payload[payloadLen] = 0; /* 补零 */

    return (int)payloadLen;
}

/* ============================================================
 * 核心：接收与处理循环 (Receiver Task)
 * ============================================================ */

/* * 内部函数：处理接收到的数据
 * 该函数在 MQTT_ProcessLoop 中被调用
 */
static void MQTT_HandleIncoming(MQTT_Client *client, size_t len)
{
    /* 只要收到了合法数据，说明链路是通的，刷新活动时间 */
    client->lastActiveTick = HAL_MQTT_GetTick();

    /* 获取报文类型 (高4位) */
    uint8_t packetType = client->rxBuf[0] & 0xF0;

    /* ----------------------------------------------------
     * 场景 1: 服务器推送的消息 (PUBLISH)
     * ---------------------------------------------------- */
    if (packetType == 0x30)
    {
        uint16_t pid = 0;
        uint8_t qos = (client->rxBuf[0] & 0x06) >> 1;

        /* 使用 client 结构体中的 msgTopicBuf 和 msgPayloadBuf 进行解析
           避免了在栈上分配大数组，防止栈溢出 */
        int payloadLen = MQTT_ParsePublishMessage(client->rxBuf, len,
                                                  client->msgTopicBuf, client->msgTopicBufSize,
                                                  client->msgPayloadBuf, client->msgPayloadBufSize,
                                                  &pid);

        if (payloadLen >= 0)
        {
            /* --- 步骤 A: 响应 PUBACK (最优先) --- */
            /* 只要是 QoS 1，无论是否重复，都必须回复 ACK，告诉服务器“我活着且收到了” */
            if (qos == 1)
            {
                /* 手动构建 PUBACK 报文 (4字节)，不依赖外部 Build 函数 */
                uint8_t ackBuf[4];
                ackBuf[0] = 0x40; /* Type: PUBACK */
                ackBuf[1] = 0x02; /* Length: 2 */
                ackBuf[2] = (uint8_t)(pid >> 8);
                ackBuf[3] = (uint8_t)pid;

                HAL_MQTT_Send(ackBuf, 4);
                HAL_MQTT_Log("MQTT: Auto-replied PUBACK id=%d\r\n", pid);
            }

            /* --- 步骤 B: 去重判断 --- */
            /* 如果 QoS > 0，需要检查是否重复处理 */
            if (qos > 0)
            {
                if (pid == client->lastRxPacketId)
                {
                    /* ID 相同，说明是重复报文，直接丢弃，不回调用户 */
                    HAL_MQTT_Log("MQTT: Duplicate Msg ID %d, dropped.\r\n", pid);
                    return;
                }
                /* 更新最后收到的 ID */
                client->lastRxPacketId = pid;
            }

            /* --- 步骤 C: 回调用户 --- */
            HAL_MQTT_OnPublishReceived((const char *)client->msgTopicBuf,
                                       (const char *)client->msgPayloadBuf,
                                       (size_t)payloadLen);
        }
        return; /* 处理完毕 */
    }

    /* ----------------------------------------------------
     * 场景 2: 各种应答报文 (ACK)
     * ---------------------------------------------------- */

    /* 如果发送线程当前并没有在等待 ACK，则忽略这些报文 */
    if (client->waitState != MQTT_WAIT_BUSY)
        return;

    bool isExpected = false;

    switch (packetType)
    {
    case 0x20: /* CONNACK */
        if (client->awaitType == MQTT_OP_CONNECT &&
            MQTT_CheckConnAck(client->rxBuf, len))
            isExpected = true;
        break;

    case 0x90: /* SUBACK */
        if (client->awaitType == MQTT_OP_SUBSCRIBE &&
            MQTT_CheckSubAck(client->rxBuf, len, client->awaitPacketId, client->qos))
            isExpected = true;
        break;

    case 0xB0: /* UNSUBACK */
        if (client->awaitType == MQTT_OP_UNSUBSCRIBE &&
            MQTT_CheckUnsubAck(client->rxBuf, len, client->awaitPacketId))
            isExpected = true;
        break;

    case 0x40: /* PUBACK */
        if (client->awaitType == MQTT_OP_PUBLISH &&
            MQTT_CheckPubAck(client->rxBuf, len, client->awaitPacketId))
            isExpected = true;
        break;

    case 0xD0: /* PINGRESP */
        if (client->awaitType == MQTT_OP_PING &&
            MQTT_CheckPingResp(client->rxBuf, len))
            isExpected = true;
        break;

    default:
        break;
    }

    /* 如果确认是发送线程期待的 ACK */
    if (isExpected)
    {
        /* 修改标志位，唤醒发送线程 */
        client->waitState = MQTT_WAIT_SUCCESS;
        HAL_MQTT_Log("MQTT: Received expected ACK for Op %d\r\n", client->awaitType);
    }
}

/* ============================================================
 * 内部函数：发送 PINGREQ (非阻塞，不占用 txBuf)
 * ============================================================ */
static void MQTT_SendPingRaw(void)
{
    /* PINGREQ 固定报文: 0xC0 0x00 */
    uint8_t pingBuf[2] = {0xC0, 0x00};
    /* 直接发送，不等待响应，响应由 ProcessLoop 的 HandleIncoming 异步处理 */
    HAL_MQTT_Send(pingBuf, 2);
    HAL_MQTT_Log("MQTT: KeepAlive PING sent\r\n");
}

/* 心跳检查函数 */
static void MQTT_MaintainKeepAlive(MQTT_Client *client)
{
    /* 1. 如果未连接，禁止发送心跳 */
    if (client->isConnected == false)
        return;

    /* 2. 如果 KeepAlive=0，表示禁用心跳 */
    if (client->keepAlive == 0)
        return;

    uint32_t currentTick = HAL_MQTT_GetTick();
    uint32_t keepAliveMs = client->keepAlive * 1000;

    /* 检查时间差是否超过 KeepAlive 的一半 */
    /* 注意：必须正确处理时间回绕 */
    if (currentTick - client->lastActiveTick > (keepAliveMs / 2))
    {
        /* 发送 PINGREQ */
        MQTT_SendPingRaw();

        /* [Bug修正]：防止 PING 洪水攻击。
         * 我们不能简单的 +=5000，因为如果断网很久，old + 5000 依然远小于 current。
         * 正确的做法是：假装我们在 "超时期限前5秒" 发送了心跳。
         * 这样如果 5秒内没收到 PINGRESP (刷新 lastActiveTick)，
         * ProcessLoop 会再次进入这里重发 PING。
         */
        client->lastActiveTick = currentTick - (keepAliveMs / 2) + 5000;

        /* 或者更简单的策略：暂时更新为当前时间，视为一次“尝试活动”。
           如果服务器不回包，应用层应该通过其他机制（如检测 PINGRESP 超时）来断开连接。
           但在轻量级实现中，上面的策略能保证 5秒重发一次 PING。
        */
    }
}

/* * 接收主循环 (消费者)
 * 作用：一直运行，负责从硬件读取数据，并调用 HandleIncoming 进行处理
 */
void MQTT_ProcessLoop(MQTT_Client *client)
{
    if (!client)
        return;

    /* 1. 尝试接收数据
       建议 timeout 设置为短时间 (如 50ms)，以便让线程有机会处理其他事务或退出。
       如果是在裸机 while(1) 中调用，timeout 可以设为 0 或 1。 */
    int rlen = HAL_MQTT_Recv(client->rxBuf, client->rxBufSize, 50);

    /* 如果读到了数据 */
    if (rlen > 0)
    {
        MQTT_HandleIncoming(client, (size_t)rlen);
    }

    /* 2. 检查并维持心跳 (在断开连接情况下会自动跳过) */
    MQTT_MaintainKeepAlive(client);
}

/* ============================================================
 * 核心：发送操作 (Sender Function)
 * ============================================================ */

/* * 执行 MQTT 操作
 * 作用：构建报文 -> 发送 -> 设置等待标志 -> 轮询等待标志变更
 */
bool MQTT_TryOperation(MQTT_Client *client, MQTT_Operation op)
{
    if (!client)
        return false;
    uint32_t len = 0;

    /* 1. 构建报文 (Tx 阶段) */
    /* 根据操作类型调用对应的构建函数 */
    switch (op)
    {
    case MQTT_OP_CONNECT:
        len = MQTT_BuildConnectPacket(client->txBuf, client->txBufSize, client->clientId, client->userName, client->password, client->keepAlive, client->cleanSession);
        break;
    case MQTT_OP_SUBSCRIBE:
        len = MQTT_BuildSubscribePacket(client->txBuf, client->txBufSize, client->subTopic, client->packetId, client->qos);
        break;
    case MQTT_OP_UNSUBSCRIBE:
        len = MQTT_BuildUnsubscribePacket(client->txBuf, client->txBufSize, client->subTopic, client->packetId);
        break;
    case MQTT_OP_PUBLISH:
        /* 自动管理 PacketID */
        /* 无论是 QoS 1 还是 QoS 0，都进行自增，确保 ID 唯一且流动 */
        client->packetId++;
        /* MQTT 协议规定 PacketID 不能为 0，如果溢出回绕到 0，必须跳过变为 1 */
        if (client->packetId == 0)
            client->packetId = 1;
        len = MQTT_BuildPublishPacket(client->txBuf, client->txBufSize, client->pubTopic, client->pubMsg, client->packetId, 0, client->qos, client->retain);
        break;
    case MQTT_OP_PING:
        len = MQTT_BuildPingReqPacket(client->txBuf, client->txBufSize);
        break;
    case MQTT_OP_DISCONNECT:
        len = MQTT_BuildDisconnectPacket(client->txBuf, client->txBufSize);
        break;
    default:
        return false;
    }

    /* 构建失败或缓冲区不足 */
    if (len == 0)
        return false;

    /* ----------------------------------------------------
     * 特殊情况：不需要等待 ACK 的操作
     * ---------------------------------------------------- */
    if ((op == MQTT_OP_PUBLISH && client->qos == 0) ||
        op == MQTT_OP_DISCONNECT)
    {
        /* 直接发送并返回成功 */
        HAL_MQTT_Send(client->txBuf, len);
        client->lastActiveTick = HAL_MQTT_GetTick();
        /* 如果是主动断开，标记为未连接 */
        if (op == MQTT_OP_DISCONNECT)
        {
            client->isConnected = false;
            HAL_MQTT_Log("MQTT: Disconnected by user\r\n");
        }
        return true;
    }

    /* ----------------------------------------------------
     * 正常情况：需要等待 ACK (带重试机制)
     * ---------------------------------------------------- */
    for (uint8_t attempt = 0; attempt < client->maxRetrys; attempt++)
    {
        HAL_MQTT_Log("MQTT: Sending Op %d (Attempt %d)\r\n", op, attempt + 1);

        /* [修复]如果是 PUBLISH 报文的重发，必须将 DUP 标志置 1 */
        /* MQTT 固定报头 Byte 1: Type(4) | DUP(1) | QoS(2) | Retain(1) */
        /* Bit 3 是 DUP 位，0x08 = 0000 1000 */
        if (attempt > 0 && op == MQTT_OP_PUBLISH)
        {
            client->txBuf[0] |= 0x08;
        }

        /* A. 在发送前，先设置好“我期待什么 ACK” */
        /* 这样能保证即使 ACK 回来得极快，Receiver 也能正确处理 */
        client->awaitType = op;
        client->awaitPacketId = client->packetId;
        client->waitState = MQTT_WAIT_BUSY; /* 状态：忙碌中 */

        /* B. 发送数据 */
        HAL_MQTT_Send(client->txBuf, len);

        /* C. 等待循环 (Wait Loop) */
        /* 这里不读 HAL_Recv，只检查 waitState 变量是否被 Receiver 线程修改 */
        uint32_t startTick = HAL_MQTT_GetTick();
        bool isTimeout = false;

        while (client->waitState == MQTT_WAIT_BUSY)
        {
            /* 检查是否超时 */
            if (HAL_MQTT_GetTick() - startTick > client->retryIntervalMs)
            {
                isTimeout = true;
                break;
            }

            /* 适当延时，让出 CPU 给 Receiver 线程运行 */
            HAL_MQTT_Delay(5);
        }

        /* D. 检查结果 */
        if (!isTimeout && client->waitState == MQTT_WAIT_SUCCESS)
        {
            client->waitState = MQTT_WAIT_IDLE; /* 清除状态 */
            /* 如果是连接操作成功，标记为已连接 */
            if (op == MQTT_OP_CONNECT)
            {
                client->isConnected = true;
                client->lastActiveTick = HAL_MQTT_GetTick(); /* 初始化心跳计时 */
                HAL_MQTT_Log("MQTT: lastActiveTick update\r\n");
            }
            return true;                        /* 操作成功 */
        }

        HAL_MQTT_Log("MQTT: Wait ACK Timeout\r\n");
        /* 如果超时，循环继续，进行下一次重发 */
    }

    /* 重试次数耗尽，操作失败 */
    client->waitState = MQTT_WAIT_IDLE;

    /* 如果连接尝试失败，确保标志位为 false */
    if (op == MQTT_OP_CONNECT)
    {
        client->isConnected = false;
    }
    return false;
}

