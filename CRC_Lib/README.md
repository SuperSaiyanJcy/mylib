# CRC_Lib - Lightweight CRC Library for Embedded Systems

**CRC_Lib** 是一个轻量级、高效的 C 语言 CRC 校验库，专为嵌入式系统设计。
它集成了最常用的 **CRC-8 (SMBus)** 和 **CRC-16 (Modbus)** 算法，采用**查表法 (Table-Driven)** 实现，以换取最快的执行速度，非常适合在对实时性要求高的 RTOS 或裸机环境中使用。

---

## 📁 文件结构

* `CRC_Lib.h`: 对外接口头文件。
* `CRC_Lib.c`: 算法实现及静态查找表 (Flash 占用优化)。
* `README.md`: 说明文档。

---

## 🚀 特性 (Features)

* **高效查表法**: 使用预计算的查找表（Look-up Table），大幅减少 CPU 计算周期。
* **标准兼容**:
    * **CRC-8**: 兼容 SMBus, I2C 传感器 (如 SHT3x, Sensirion 全系列, MPU6050 等)。
    * **CRC-16**: 兼容 Modbus RTU 标准协议 (工业自动化, PLC 通讯)。
* **零依赖**: 纯 C99 标准代码 (`<stdint.h>`)，无任何第三方依赖。
* **易集成**: 简单的 `.c/.h` 结构，直接拖入工程即可使用。

---

## 📊 算法参数规范 (Technical Specifications)

在对接上位机或硬件时，请参考以下参数模型：

| 参数项               | CRC-8 (SMBus)                | CRC-16 (Modbus)                        |
| :------------------- | :--------------------------- | :------------------------------------- |
| **Width**            | 8-bit                        | 16-bit                                 |
| **Polynomial**       | `0x07` ($x^8 + x^2 + x + 1$) | `0x8005` ($x^{16} + x^{15} + x^2 + 1$) |
| **Initial Value**    | `0x00`                       | `0xFFFF`                               |
| **Input Reflected**  | No                           | Yes (按字节翻转)                       |
| **Result Reflected** | No                           | Yes (按字节翻转)                       |
| **XOR Output**       | `0x00`                       | `0x00`                                 |
| **Check (0x31..39)** | `0xF4`                       | `0x4B37`                               |

---

## 🛠️ 集成指南 (Integration)

1.  将 `CRC_Lib.c` 和 `CRC_Lib.h` 复制到你的工程目录（例如 `Drivers/BSP/CRC`）。
2.  在你的 IDE (Keil, IAR, GCC/Make) 中将 `CRC_Lib.c` 添加到编译路径。
3.  在需要使用的文件中包含头文件：

```c
#include "CRC_Lib.h"
```
---

## 💻 使用示例 (Usage)

**1. 计算 CRC-8 (I2C/SMBus)**

```c
#include <stdio.h>
#include "CRC_Lib.h"

int main(void)
{
    // 模拟 I2C 传感器数据: [CMD, DATA_MSB, DATA_LSB]
    uint8_t sensor_data[] = {0x2C, 0x06, 0xAB};
    
    // 计算校验码
    uint8_t checksum = CRC8_Cal(sensor_data, 3);
    
    printf("Calculated CRC8: 0x%02X\n", checksum);
    
    return 0;
}
```

**2. 计算 CRC-16 (Modbus RTU)**

```c
#include <stdio.h>
#include "CRC_Lib.h"

int main(void)
{
    // Modbus 报文示例: [地址, 功能码, 寄存器高, 寄存器低, 数量高, 数量低]
    uint8_t modbus_frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    
    // 计算校验码
    uint16_t crc16 = Modbus_CRC16_Cal(modbus_frame, 6);
    
    // Modbus 协议通常要求低字节在前 (Little Endian)
    uint8_t crc_lo = crc16 & 0xFF;
    uint8_t crc_hi = crc16 >> 8;
    
    printf("CRC16: 0x%04X (Lo: 0x%02X, Hi: 0x%02X)\n", crc16, crc_lo, crc_hi);
    
    return 0;
}
```
---
## ⚠️ 注意事项

1. **空间占用**: 查表法通过空间换时间。
   - `CRC8`表占用 256 字节 Flash。
   - `CRC16`表占用 512 字节 Flash (高字节表 + 低字节表)。
   - 如果您的单片机 Flash 极度紧张（如小于 4KB 的 8051），请慎用查表法。
2. **线程安全**: 本库函数为纯函数（Pure Function），无静态变量状态，天然**可重入且线程安全**，可在中断 (ISR) 或多线程 RTOS 任务中放心调用。

