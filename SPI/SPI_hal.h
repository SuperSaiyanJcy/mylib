#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
/* ============================================================
 * [User Configuration] SPI Mode & Hardware Settings
 * ============================================================ */

/**
 * @brief Select SPI Mode (0, 1, 2, or 3)
 * Mode | CPOL | CPHA | Clock Idle | Sampling Edge
 * 0   |  0   |  0   |    Low     | Rising (1st)
 * 1   |  0   |  1   |    Low     | Falling (2nd)
 * 2   |  1   |  0   |    High    | Falling (1st)
 * 3   |  1   |  1   |    High    | Rising (2nd)
 */
#define SPI_MODE 0

/**
 * @brief Dummy byte to send when reading data
 * Standard is usually 0xFF or 0x00
 */
#define SPI_DUMMY_BYTE 0xFF

/* ============================================================
 * [Hardware Macros] Map these to your specific MCU HAL
 * Note: GPIOs should be configured as:
 * - CS, SCK, MOSI : Output Push-Pull
 * - MISO          : Input (Floating or Pull-up based on slave)
 * ============================================================ */

// Example for STM32 HAL:
// #define SPI_CS_H        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
// #define SPI_CS_L        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)

#define SPI_CS_H    /* Implement: Set CS Pin High */
#define SPI_CS_L    /* Implement: Set CS Pin Low */
#define SPI_SCK_H   /* Implement: Set SCK Pin High */
#define SPI_SCK_L   /* Implement: Set SCK Pin Low */
#define SPI_MOSI_H  /* Implement: Set MOSI Pin High */
#define SPI_MOSI_L  /* Implement: Set MOSI Pin Low */

/* Must return 0 or 1 */
#define SPI_READ_MISO   0 /* Implement: Read MISO Pin State */

/* ============================================================
 * HAL Function Prototypes
 * ============================================================ */

void SPI_Hal_Init(void);
void SPI_Delay(void);

#ifdef __cplusplus
}
#endif
