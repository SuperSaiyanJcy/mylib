#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IIC_SDA_H       /* Implement: Set SDA Pin High */
#define IIC_SDA_L       /* Implement: Set SDA Pin Low */
#define IIC_SCL_H       /* Implement: Set SCL Pin High */
#define IIC_SCL_L       /* Implement: Set SCL Pin Low */
#define IIC_READ_SDA    0/* Implement: Read SDA Pin Value (0 or 1) */

#define IIC_ENABLE_CLOCK_STRETCHING     0   // 1: Enable, 0: Disable

#if IIC_ENABLE_CLOCK_STRETCHING
#define IIC_SCL_STRETCH_TIMEOUT         1000U
#define IIC_READ_SCL    0/* Implement: Read SCL Pin Value (0 or 1) */
#endif

/**
 * @brief Initialize the low-level hardware (GPIOs) for IIC.
 * Users must implement specific GPIO configuration here (e.g., Enable Clocks, Set Modes).
 * Note: SDA and SCL should be configured as Open-Drain Output with Pull-Up resistors.
 */
void IIC_Hal_Init(void);

/**
 * @brief Short delay for IIC timing control.
 * This function determines the IIC bus speed (Baud Rate).
 * Adjust the loop count inside to meet Standard Mode (100kHz) or Fast Mode (400kHz) requirements.
 */
void IIC_Delay(void);

#ifdef __cplusplus
}
#endif
