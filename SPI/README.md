# Generic Software SPI Driver (Bit-Banging)

这是一个通用的、平台无关的软件模拟 SPI (Bit-Banging) 驱动库。设计目标是**高效率**和**易移植性**。

通过宏定义在编译时处理 SPI 模式（Mode 0-3），该驱动在运行时没有额外的判断开销，生成的代码极其精简且高效。

---

## ✨ 主要特性 (Features)

* **硬件无关性**：核心时序逻辑与 GPIO 操作完全分离。
* **全模式支持**：支持所有 4 种 SPI 模式 (Mode 0, 1, 2, 3)，通过宏配置。
* **零运行时开销**：使用预编译指令 (`#if`) 处理模式差异，运行时无 `if-else` 判断，速度极快。
* **标准接口**：提供标准的 `SwapByte` 全双工交换接口，以及批量读写封装。
* **代码规范**：遵循 MSB First (高位先行) 标准，符合绝大多数 SPI 设备要求。

---

## 📂 文件结构 (File Structure)

| 文件名          | 描述                                           | 是否需要修改 |
| :-------------- | :--------------------------------------------- | :----------- |
| **`SPI.c`**     | SPI 核心时序逻辑 (SwapByte, Transfer...)       | ❌ 否         |
| **`SPI.h`**     | 公共 API 接口声明                              | ❌ 否         |
| **`SPI_hal.c`** | **硬件抽象层实现** (GPIO 初始化、延时实现)     | ✅ **是**     |
| **`SPI_hal.h`** | **硬件抽象层配置** (SPI 模式选择、GPIO 宏定义) | ✅ **是**     |

---

## 🚀 移植指南 (Porting Guide)

只需修改 `SPI_hal.h` 和 `SPI_hal.c` 两个文件。

### 1. 配置模式与引脚 (`SPI_hal.h`)

打开 `SPI_hal.h`，根据从机设备的数据手册配置模式，并绑定 GPIO：

```c
/* ============================================================
 * 用户配置区域 (User Configuration)
 * ============================================================ */

// 选择 SPI 模式 (0, 1, 2, 3)
// Mode 0 是最常用的 (CPOL=0, CPHA=0)
#define SPI_MODE                0 

// 硬件引脚映射 (以 STM32 HAL 为例)
// 注意：不要在宏后面加分号，也不要加括号
#define SPI_CS_H        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define SPI_CS_L        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)

#define SPI_SCK_H       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)
#define SPI_SCK_L       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET)

#define SPI_MOSI_H      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET)
#define SPI_MOSI_L      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET)

// 读取 MISO 引脚 (必须返回 0 或 1)
#define SPI_READ_MISO   HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6)
```
### 2. 实现初始化与延时（`SPI_hal.c`）

在`SPI_hal.c`中填充：

```c
void SPI_Hal_Init(void)
{
    // 1. 开启 GPIO 时钟
    // 2. 配置引脚模式：
    //    - CS, SCK, MOSI: 推挽输出 (Output Push-Pull)
    //    - MISO: 浮空输入 (Input Floating) 或 上拉输入
    
    // 3. 设置初始状态 (CS 不选中)
    SPI_CS_H;
}

void SPI_Delay(void)
{
    // 微秒级延时，控制 SPI 速率
    // 对于高速 MCU，需要适当增加延时以满足从机建立时间
    volatile uint32_t i = 5;
    while (i--);
}
```

---

## 📖 API 说明 (API Reference)

### 核心控制
* `void SPI_Init(void)`: 初始化 SPI 接口。
* `void SPI_CS_Enable(void)`: 拉低 CS 片选信号（开始传输）。
* `void SPI_CS_Disable(void)`: 拉高 CS 片选信号（结束传输）。

### 数据传输
SPI 是全双工协议，发送的同时必然接收。

* `uint8_t SPI_SwapByte(uint8_t tx_data)`
    * 发送一个字节，并返回接收到的一个字节。这是最基础的原子操作。
* `void SPI_Write(const uint8_t *data, uint16_t length)`
    * 批量写入数据（接收到的数据被丢弃）。
* `void SPI_Read(uint8_t *data, uint16_t length)`
    * 批量读取数据（发送 Dummy Byte，默认 0xFF）。
* `void SPI_Transfer(const uint8_t *tx_data, uint8_t *rx_data, uint16_t length)`
    * 全双工批量交换。

---

## 💻 使用示例 (Usage Examples)

### 1. 读写寄存器 (典型 SPI 传感器)

```c
#include "SPI.h"

void Read_Sensor_Reg(void)
{
    SPI_Init();
    
    // 1. 拉低片选
    SPI_CS_Enable();
    
    // 2. 发送寄存器地址 (例如 0x80 | 0x0F)
    SPI_SwapByte(0x8F); 
    
    // 3. 发送 Dummy Byte 以读取数据
    uint8_t value = SPI_SwapByte(0xFF);
    
    // 4. 拉高片选
    SPI_CS_Disable();
    
    printf("Register Value: 0x%02X\r\n", value);
}
```

### 2. 批量写入（如OLED/LCD刷屏）

```c
void LCD_Write_Data(uint8_t *buffer, uint16_t size)
{
    SPI_CS_Enable();
    
    // 使用批量写入函数，效率更高
    SPI_Write(buffer, size);
    
    SPI_CS_Disable();
}
```
---

## ⚠️ 注意事项

1.  **SPI 模式选择**：这是最容易出错的地方。请务必查阅从机数据手册，确认其 **CPOL** (时钟极性) 和 **CPHA** (时钟相位)。
    * Mode 0 (CPOL=0, CPHA=0): 大部分设备使用此模式。
    * Mode 3 (CPOL=1, CPHA=1): 也是常见模式。
    * *修改 `SPI_hal.h` 中的 `SPI_MODE` 宏即可切换。*
2.  **GPIO 模式**：与 IIC 不同，SPI 的 SCK, MOSI, CS 必须配置为 **推挽输出 (Push-Pull)**，而不是开漏。
3.  **MSB First**：本驱动默认采用 **高位先行 (MSB First)**，这是绝大多数 SPI 设备的标准。如果遇到极少数 LSB First 的设备，需要手动翻转数据位。
4.  **CS 控制**：`SPI_Write` 和 `SPI_Read` 函数内部**不包含** CS 控制。这是为了允许用户在一个 CS 有效期间进行多次连续的读写操作（例如：写命令 -> 读数据）。请在调用 API 前后手动调用 `SPI_CS_Enable/Disable`。