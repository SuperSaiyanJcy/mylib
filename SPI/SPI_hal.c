#include "SPI_hal.h"

/**
 * @brief Initialize SPI GPIOs
 * User must implement the specific GPIO configuration here.
 */
void SPI_Hal_Init(void)
{
    // 1. Enable GPIO Clocks
    // 2. Configure CS, SCK, MOSI as Output Push-Pull
    // 3. Configure MISO as Input

    // Set Default States
    SPI_CS_H; // CS Idle High (Inactive)

    // Set SCK Idle Level based on Mode
#if (SPI_MODE == 0) || (SPI_MODE == 1)
    SPI_SCK_L; // CPOL=0: Idle Low
#else
    SPI_SCK_H; // CPOL=1: Idle High
#endif
}

/**
 * @brief Microsecond delay for SPI timing
 * Adjust this loop to change SPI baud rate.
 */
void SPI_Delay(void)
{
    // Implementation depends on MCU speed.
    // Example for ~1MHz SPI on 72MHz MCU:
    volatile uint32_t i = 5;
    while (i--);
}
