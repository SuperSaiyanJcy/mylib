#pragma once

#include "IIC_hal.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Initialize the IIC interface.
 * Configures the GPIO pins (SCL and SDA) to their default state (High/Open-Drain).
 * This must be called once before performing any transaction.
 */
void IIC_Init(void);

/**
 * @brief IIC Write Byte(s) to 8-bit register address
 * * @param device_addr 7-bit device address (e.g., 0x50, NOT 0xA0).
 * **参数必须是7位地址，不包含读写位。**
 * @param reg_addr    8-bit register address
 * @param data        Pointer to data buffer
 * @param length      Number of bytes to write
 * @return true if success, false if NACK/Error
 */
bool IIC_Write(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, uint16_t length);

/**
 * @brief IIC Write Byte(s) to 16-bit register address
 * * @param device_addr 7-bit device address (e.g., 0x50). **Must be 7-bit.**
 * @param reg_addr    16-bit register address
 * @param data        Pointer to data buffer
 * @param length      Number of bytes to write
 * @return true if success
 */
bool IIC_Write_Reg16(uint8_t device_addr, uint16_t reg_addr, const uint8_t *data, uint16_t length);

/**
 * @brief IIC Read Byte(s) from 8-bit register address
 * * @param device_addr 7-bit device address. **参数必须是7位地址。**
 * @param reg_addr    8-bit register address
 * @param data        Pointer to buffer to store read data
 * @param length      Number of bytes to read
 * @return true if success
 */
bool IIC_Read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t length);

/**
 * @brief IIC Read Byte(s) from 16-bit register address
 * * @param device_addr 7-bit device address. **Must be 7-bit.**
 * @param reg_addr    16-bit register address
 * @param data        Pointer to buffer to store read data
 * @param length      Number of bytes to read
 * @return true if success
 */
bool IIC_Read_Reg16(uint8_t device_addr, uint16_t reg_addr, uint8_t *data, uint16_t length);

/**
 * @brief Attempt to recover the IIC bus from a stuck state.
 * * If a slave device holds SDA Low (e.g., due to a reset during a transaction),
 * this function toggles SCL 9 times to clock out the remaining bits from the slave
 * and then sends a STOP condition to reset the bus logic.
 * * Usage: Call this if a timeout occurs or on system startup.
 */
void IIC_Bus_Recovery(void);

/**
 * @brief Scan the IIC bus to find the first connected device.
 * Useful when the device address is unknown.
 * @return 7-bit device address (e.g., 0x50). Returns 0x00 if no device is found.
 */
uint8_t IIC_Scan_Device_Addr(void);

#ifdef __cplusplus
}
#endif
