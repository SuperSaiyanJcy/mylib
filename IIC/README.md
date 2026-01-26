# Generic Software IIC Driver (Bit-Banging)

这是一个通用的、平台无关的软件模拟 IIC (Bit-Banging) 驱动库。设计目标是**高移植性**和**高健壮性**。

通过将底层 GPIO 操作（HAL 层）与协议逻辑（Driver 层）分离，用户只需修改极少量的代码即可将此驱动移植到任何 MCU（STM32, ESP32, AVR, 51 等）或 Linux 用户空间。

---

## ✨ 主要特性 (Features)

* **硬件无关性**：核心逻辑与硬件操作完全分离，轻松移植。
* **多模式支持**：支持 8 位寄存器地址和 16 位寄存器地址读写。
* **健壮性设计**：
    * **自动防呆**：内部强制掩码处理 (`& 0x7F`)，防止用户传入错误的 8 位读写地址。
    * **总线恢复**：提供 `IIC_Bus_Recovery` 机制，用于在从机死锁（拉低 SDA）时复位总线。
    * **时序保护**：严格的 Start/Stop/Ack 时序控制。
* **实用工具**：内置总线扫描功能 (`IIC_Scan_Device_Addr`)，快速查找未知设备地址。
* **可选功能**：支持 IIC 时钟延展 (Clock Stretching)（需在头文件开启）。

---

## 📂 文件结构 (File Structure)

| 文件名          | 描述                                                 | 是否需要修改 |
| :-------------- | :--------------------------------------------------- | :----------- |
| **`IIC.c`**     | IIC 协议核心逻辑（Start, Stop, Ack, Read, Write...） | ❌ 否         |
| **`IIC.h`**     | 公共 API 接口声明                                    | ❌ 否         |
| **`IIC_hal.c`** | **硬件抽象层实现**（GPIO 初始化、延时函数）          | ✅ **是**     |
| **`IIC_hal.h`** | **硬件抽象层配置**（GPIO 宏定义、参数配置）          | ✅ **是**     |

---

## 🚀 移植指南 (Porting Guide)

只需修改 `IIC_hal.h` 和 `IIC_hal.c` 两个文件即可适配您的硬件。

### 1. 配置 GPIO 宏 (`IIC_hal.h`)

打开 `IIC_hal.h`，根据您的 MCU SDK（如 STM32 HAL, Standard Lib 等）实现以下宏：

```c
// 示例：STM32 HAL 库实现
#define IIC_SDA_H       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET)
#define IIC_SDA_L       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET)

#define IIC_SCL_H       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)
#define IIC_SCL_L       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)

// 读取 SDA 引脚电平 (返回 0 或 1)
#define IIC_READ_SDA    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)

// 如果启用了时钟延展，需要实现读取 SCL
#define IIC_READ_SCL    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)
```

### 2. 实现初始化与延时（`IIC_hal.c`）

在`IIC_hal.c`中填充具体代码

```c
void IIC_Hal_Init(void)
{
    // 在这里初始化 GPIO
    // 1. 开启时钟
    // 2. 配置 SDA, SCL 为 **开漏输出 (Open-Drain)** 模式 + 上拉电阻
    // 3. 设置默认高电平
}

void IIC_Delay(void)
{
    // 微秒级延时，决定 IIC 通讯速率
    // 空循环或调用系统延时函数
    volatile uint32_t i = 10;
    while (i--);
}
```

---

## 📖 API 说明 (API Reference)

### 初始化与工具
* `void IIC_Init(void)`: 初始化 IIC 总线。
* `void IIC_Bus_Recovery(void)`: 发送 9 个时钟脉冲尝试解锁总线。
* `uint8_t IIC_Scan_First(void)`: 扫描总线，返回发现的第一个设备地址。

### 数据读写
所有读写函数的 `device_addr` 参数均指 **7位设备地址** (例如 `0x50`)，而非 8 位读写地址 (`0xA0`)。

* `bool IIC_Write(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, uint16_t length)`
    * 写入数据到 8 位寄存器地址。
* `bool IIC_Read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint16_t length)`
    * 从 8 位寄存器地址读取数据。
* `bool IIC_Write_Reg16(...)` / `bool IIC_Read_Reg16(...)`
    * 针对 16 位寄存器地址（如 EEPROM 大容量型号、某些传感器）的读写版本。

---

## 💻 使用示例 (Usage Examples)

### 1. 基础读写示例 (EEPROM / Sensor)

```c
#include "IIC.h"

#define DEV_ADDR  0x50  // 7-bit Address (相当于写地址 0xA0)

void Test_IIC(void)
{
    uint8_t tx_buf[2] = {0xAA, 0xBB};
    uint8_t rx_buf[2] = {0};

    IIC_Init();

    // 写入 2 个字节到寄存器 0x10
    if (IIC_Write(DEV_ADDR, 0x10, tx_buf, 2)) {
        printf("Write Success\r\n");
    } else {
        printf("Write Failed\r\n");
    }

    IIC_Delay(); // 等待写入完成（EEPROM通常需要）

    // 从寄存器 0x10 读取 2 个字节
    if (IIC_Read(DEV_ADDR, 0x10, rx_buf, 2)) {
        printf("Read Data: 0x%02X 0x%02X\r\n", rx_buf[0], rx_buf[1]);
    }
}
```
### 2. 扫描总线地址（`Bus Scan`）

不知道设备的地址？使用扫描功能：
```c
void Scan_Devices(void)
{
    printf("Scanning...\r\n");
    uint8_t addr = IIC_Scan_Device_Addr();
    
    if (addr != 0) {
        printf("Device Found at 7-bit Addr: 0x%02X\r\n", addr);
    } else {
        printf("No Device Found.\r\n");
        IIC_Bus_Recovery(); // 尝试复位总线
    }
}
```
---

## ⚠️ 注意事项

1.  **设备地址格式**：API 仅接受 **7位地址**。
    * 正确：`0x50`
    * 错误：`0xA0` (这是 8 位写地址)
    * *注：虽然代码内部做了 `& 0x7F` 掩码保护，但建议养成传递正确地址的习惯。*
2.  **GPIO 模式**：SDA 和 SCL 必须配置为 **Open-Drain (开漏输出)**，且外部必须接上拉电阻（通常 4.7kΩ）。如果不使用开漏模式，可能会导致短路。
3.  **延时调整**：根据 MCU 的主频调整 `IIC_Delay`。过快的速度可能导致从机无法响应（标准模式 100kHz，快速模式 400kHz）。
