#include "SPI.h"

void SPI_Init(void)
{
    SPI_Hal_Init();
}

void SPI_CS_Enable(void)
{
    SPI_CS_L;
    SPI_Delay(); // Ensure setup time before clocking
}

void SPI_CS_Disable(void)
{
    SPI_Delay(); // Ensure hold time after last clock
    SPI_CS_H;
    SPI_Delay(); // Ensure minimum idle time between transactions
}

/*
 * Internal Macro to handle MOSI Output
 */
#define SPI_WRITE_BIT(byte, bit)      \
    do                                \
    {                                 \
        if ((byte) & (0x80 >> (bit))) \
            SPI_MOSI_H;               \
        else                          \
            SPI_MOSI_L;               \
    } while (0)

uint8_t SPI_SwapByte(uint8_t tx_data)
{
    uint8_t rx_data = 0;
    uint8_t i;

    // =================================================================
    // CPHA = 0 Logic (Mode 0, Mode 2)
    // Data sampled on 1st edge, Changed on 2nd edge
    // =================================================================
#if (SPI_MODE == 0) || (SPI_MODE == 2)

    for (i = 0; i < 8; i++)
    {
        // 1. Setup Data (Output) BEFORE the first clock edge
        SPI_WRITE_BIT(tx_data, i);
        SPI_Delay();

// 2. Latch Data (Input) on Leading Edge
#if (SPI_MODE == 0)
        SPI_SCK_H; // Rising Edge (Idle Low -> High)
#else
        SPI_SCK_L; // Falling Edge (Idle High -> Low)
#endif

        // Sample MISO
        if (SPI_READ_MISO)
            rx_data |= (0x80 >> i);

        SPI_Delay();

// 3. Restore Clock (Trailing Edge)
#if (SPI_MODE == 0)
        SPI_SCK_L;
#else
        SPI_SCK_H;
#endif

        // (Next data bit setup happens at top of loop or after loop)
    }

    // =================================================================
    // CPHA = 1 Logic (Mode 1, Mode 3)
    // Data Changed on 1st edge, Sampled on 2nd edge
    // =================================================================
#elif (SPI_MODE == 1) || (SPI_MODE == 3)

    for (i = 0; i < 8; i++)
    {
// 1. Leading Edge (Clock Shift)
#if (SPI_MODE == 1)
        SPI_SCK_H; // Rising Edge
#else
        SPI_SCK_L; // Falling Edge
#endif

        // 2. Change Data (Output)
        SPI_WRITE_BIT(tx_data, i);
        SPI_Delay();

// 3. Trailing Edge (Clock Latch)
#if (SPI_MODE == 1)
        SPI_SCK_L; // Falling Edge
#else
        SPI_SCK_H; // Rising Edge
#endif

        // Sample MISO
        if (SPI_READ_MISO)
            rx_data |= (0x80 >> i);

        SPI_Delay();
    }
#endif

    return rx_data;
}

void SPI_Write(const uint8_t *data, uint16_t length)
{
    if (length == 0)
        return;
    for (uint16_t i = 0; i < length; i++)
    {
        SPI_SwapByte(data[i]);
    }
}

void SPI_Read(uint8_t *data, uint16_t length)
{
    if (length == 0)
        return;
    for (uint16_t i = 0; i < length; i++)
    {
        data[i] = SPI_SwapByte(SPI_DUMMY_BYTE);
    }
}

void SPI_Transfer(const uint8_t *tx_data, uint8_t *rx_data, uint16_t length)
{
    if (length == 0)
        return;
    for (uint16_t i = 0; i < length; i++)
    {
        rx_data[i] = SPI_SwapByte(tx_data[i]);
    }
}
