#ifndef _CRC_LIB_H_
#define _CRC_LIB_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* * ============================================================
    * CRC8 计算 (SMBus 标准)
    * ============================================================
    * 多项式: x^8 + x^2 + x + 1 (0x07)
    * 初始值: 0x00
    * 适用: I2C 传感器 (如 SHT3x, MPU6050), SMBus 设备
    */
uint8_t CRC8_Cal(const uint8_t *pchMsg, uint16_t wDataLen);

/* * ============================================================
    * Modbus CRC16 计算
    * ============================================================
    * 多项式: x^16 + x^15 + x^2 + 1 (0x8005)
    * 初始值: 0xFFFF
    * 适用: Modbus RTU 通讯, 工业自动化设备
    * 返回: CRC16 校验值 
    */
uint16_t Modbus_CRC16_Cal(const uint8_t *pchMsg, uint16_t wDataLen);

#ifdef __cplusplus
}
#endif

#endif // _CRC_LIB_H_