#pragma once

#include "SPI_hal.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Initialize the Soft-SPI bus
 */
void SPI_Init(void);

/**
 * @brief Activate Chip Select (Pull CS Low)
 */
void SPI_CS_Enable(void);

/**
 * @brief Deactivate Chip Select (Pull CS High)
 */
void SPI_CS_Disable(void);

/**
 * @brief Transmit and Receive one byte simultaneously
 * @param tx_data Data to send
 * @return Data received from slave
 */
uint8_t SPI_SwapByte(uint8_t tx_data);

/**
 * @brief Write multiple bytes (RX data is discarded)
 * @param data Pointer to data buffer
 * @param length Number of bytes
 */
void SPI_Write(const uint8_t *data, uint16_t length);

/**
 * @brief Read multiple bytes (Transmits Dummy Byte)
 * @param data Pointer to buffer to store received data
 * @param length Number of bytes
 */
void SPI_Read(uint8_t *data, uint16_t length);

/**
 * @brief Full-duplex transfer
 * @param tx_data Pointer to TX buffer
 * @param rx_data Pointer to RX buffer
 * @param length Number of bytes
 */
void SPI_Transfer(const uint8_t *tx_data, uint8_t *rx_data, uint16_t length);

#ifdef __cplusplus
}
#endif
