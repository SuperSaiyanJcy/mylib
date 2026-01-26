#include "IIC.h"
/*
 * IIC assumptions:
 * 1. SDA is open-drain with external pull-up
 * 2. Single-master only
 * 3. Optional clock stretching support (compile-time)
 * 4. ACK is sampled after one delay (fail-fast when disabled)
 */

/* ===========================
 * Clock Stretching Configuration
 * =========================== */

#if IIC_ENABLE_CLOCK_STRETCHING

#define IIC_WAIT_SCL_HIGH()                    \
    do                                         \
    {                                          \
        uint32_t _t = IIC_SCL_STRETCH_TIMEOUT; \
        while (!IIC_READ_SCL)                  \
        {                                      \
            if (--_t == 0)                     \
                return false;                  \
        }                                      \
    } while (0)

#else

/* Zero-cost when disabled */
#define IIC_WAIT_SCL_HIGH() \
    do                      \
    {                       \
    } while (0)

#endif

// Initialize IIC
void IIC_Init(void)
{
    IIC_Hal_Init();
}

/* NOTE:
 * Start/Stop phases do NOT handle clock stretching.
 * Assumes single-master, non-faulted bus.
 */
// SCL High,SDA High to Low,then SCL Low to transmit data
static void IIC_Start(void)
{
    IIC_SDA_H;
    IIC_SCL_H;
    IIC_Delay();
    IIC_SDA_L;
    IIC_Delay();
    IIC_SCL_L;
    IIC_Delay();
}

/* Repeated START is implemented by reusing Start().
 * Valid because SCL is low before calling Restart().
 */
#define IIC_Restart IIC_Start

// SCL High,SDA Low to High,then SCL Low to stop data transmission
static void IIC_Stop(void)
{
    IIC_SCL_L;
    IIC_SDA_L;
    IIC_Delay();
    IIC_SCL_H;
    IIC_Delay();
    IIC_SDA_H;
}

// Send ACK
static void IIC_Ack(void)
{
    IIC_SDA_L;
    IIC_SCL_H;
    IIC_Delay();
    IIC_SCL_L;
    IIC_Delay();
}

// Send NACK
static void IIC_Nack(void)
{
    IIC_SDA_H;
    IIC_SCL_H;
    IIC_Delay();
    IIC_SCL_L;
    IIC_Delay();
}

/* ===========================
 * Byte Transfer
 * =========================== */

// Send one byte, return true if ACK received
static bool IIC_Send_Byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & (0x80 >> i))
            IIC_SDA_H;
        else
            IIC_SDA_L;
        IIC_SCL_H;
        IIC_WAIT_SCL_HIGH();
        IIC_Delay();
        IIC_SCL_L;
        IIC_Delay();
    }
    // Wait for ACK
    IIC_SDA_H; // Release SDA
    IIC_SCL_H;
    IIC_WAIT_SCL_HIGH();
    IIC_Delay();
    if (IIC_READ_SDA)
    {
        // IIC_Stop(); // "IIC_Stop" decided by IIC_Write or IIC_Read
        return false; // No ACK received
    }
    IIC_SCL_L;
    IIC_Delay();
    return true; // ACK received
}

// Receive a byte of data
static bool IIC_Receive_Byte(uint8_t *data)
{
    uint8_t val = 0;

    IIC_SDA_H; // Release SDA

    for (uint8_t i = 0; i < 8; i++)
    {
        IIC_SCL_H;
        IIC_WAIT_SCL_HIGH();
        IIC_Delay();
        val <<= 1;
        if (IIC_READ_SDA)
            val |= 0x01;

        IIC_SCL_L;
        IIC_Delay();
    }
    *data = val;
    return true;
}

/* ===========================
 * Public IIC API
 * =========================== */

// Write with 8-bit register address
bool IIC_Write(uint8_t device_addr, uint8_t reg_addr, const uint8_t* data, uint16_t length)
{
    if (length == 0)
        return false;
    IIC_Start();
    if (IIC_Send_Byte((device_addr & 0x7F) << 1)) // Write mode
    {
        if (IIC_Send_Byte(reg_addr))
        {
            for (uint16_t i = 0; i < length; i++)
            {
                if (!IIC_Send_Byte(data[i]))
                {
                    IIC_Stop();
                    return false;
                }
            }
            IIC_Stop();
            return true;
        }
    }
    IIC_Stop();
    return false;
}

// Read with 8-bit register address
bool IIC_Read(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint16_t length)
{
    if (length == 0)
        return false;
    IIC_Start();
    if (IIC_Send_Byte((device_addr & 0x7F) << 1)) // Write mode
    {
        if (IIC_Send_Byte(reg_addr))
        {
            IIC_Restart();
            if (IIC_Send_Byte(((device_addr & 0x7F) << 1) | 0x01)) // Read mode
            {
                for (uint16_t i = 0; i < length - 1; i++)
                {
                    if (!IIC_Receive_Byte(&data[i]))
                    {
                        IIC_Stop();
                        return false;
                    }
                    IIC_Ack();
                }
                if (!IIC_Receive_Byte(&data[length - 1]))
                {
                    IIC_Stop();
                    return false;
                }
                IIC_Nack();
                IIC_Stop();
                return true;
            }
        }
    }
    IIC_Stop();
    return false;
}

// Write with 16-bit register address
bool IIC_Write_Reg16(uint8_t device_addr, uint16_t reg_addr, const uint8_t *data, uint16_t length)
{
    if (length == 0)
        return false;
    IIC_Start();
    if (IIC_Send_Byte((device_addr & 0x7F) << 1))
    {
        if (IIC_Send_Byte((uint8_t)(reg_addr >> 8)) &&
            IIC_Send_Byte((uint8_t)(reg_addr & 0xFF)))
        {
            for (uint16_t i = 0; i < length; i++)
            {
                if (!IIC_Send_Byte(data[i]))
                {
                    IIC_Stop();
                    return false;
                }
            }
            IIC_Stop();
            return true;
        }
    }
    IIC_Stop();
    return false;
}

// Read with 16-bit register address
bool IIC_Read_Reg16(uint8_t device_addr, uint16_t reg_addr, uint8_t *data, uint16_t length)
{
    if (length == 0)
        return false;
    IIC_Start();
    if (IIC_Send_Byte((device_addr & 0x7F) << 1))
    {
        if (IIC_Send_Byte((uint8_t)(reg_addr >> 8)) &&
            IIC_Send_Byte((uint8_t)(reg_addr & 0xFF)))
        {
            IIC_Restart();
            if (IIC_Send_Byte(((device_addr & 0x7F) << 1) | 0x01)) // Read mode
            {
                for (uint16_t i = 0; i < length - 1; i++)
                {
                    if (!IIC_Receive_Byte(&data[i]))
                    {
                        IIC_Stop();
                        return false;
                    }
                    IIC_Ack();
                }
                if (!IIC_Receive_Byte(&data[length - 1]))
                {
                    IIC_Stop();
                    return false;
                }
                IIC_Nack();
                IIC_Stop();
                return true;
            }
        }
    }
    IIC_Stop();
    return false;
}

// Recover IIC bus in case of stuck condition
void IIC_Bus_Recovery(void)
{
    IIC_Init();
    IIC_SDA_H;
    for (uint8_t i = 0; i < 9; i++)
    {
        IIC_SCL_H;
        IIC_Delay();
        IIC_SCL_L;
        IIC_Delay();
    }
    IIC_Stop();
}

// Scan for the first available device on the bus
uint8_t IIC_Scan_Device_Addr(void)
{
    // Iterate through valid 7-bit addresses (0x01 to 0x7F)
    // 0x00 is usually reserved for General Call
    for (uint8_t addr = 1; addr < 128; addr++)
    {
        IIC_Start();

        // Send address with Write bit (0) -> (addr << 1)
        // IIC_Send_Byte returns true if ACK is received (Device present)
        if (IIC_Send_Byte(addr << 1))
        {
            // ACK received: Device found at this address
            // Must send Stop to release the bus cleanly
            IIC_Stop();
            return addr;
        }

        // NACK received: No device at this address
        // Must send Stop to reset bus state for the next probe
        IIC_Stop();

        // Optional: Small delay between probes to prevent flooding
        // IIC_Delay();
    }

    return 0x00; // No device responded
}
