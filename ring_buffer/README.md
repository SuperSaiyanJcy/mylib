# High-Performance DMA Ring Buffer (C99)

这是一个专为嵌入式系统设计的高效环形缓冲区（Ring Buffer）库。它采用纯 C99 实现，逻辑层与硬件层完全解耦，特别针对 DMA 循环传输模式进行了优化，支持零拷贝（Zero-Copy）数据处理。

## ✨ 核心特性

* **极致性能**：强制要求缓冲区大小为 **2 的幂（Power of 2）**，使用位运算（`&`）代替昂贵的取余运算（`%`）和比较跳转。
* **DMA 深度集成**：支持硬件自动更新写指针（Head），软件维护读指针（Tail），完美适配 STM32/GD32 等 MCU 的 DMA Circular Mode。
* **零拷贝接口**：提供 `rb_peek_continuous` 接口，允许直接访问缓冲区内部内存，避免数据在不同数组间即使是 `memcpy` 的开销。
* **高健壮性**：
    * 完整的空指针（NULL）检查。
    * 读写边界保护（防止指针跑飞）。
    * 保留 1 字节策略，精确区分“满”与“空”状态。
* **架构清晰**：HAL 层（硬件抽象层）隔离，移植仅需修改一个文件。

---

## 📂 文件结构

只需将以下 4 个文件加入你的工程：

| 文件名                  | 说明                                            |
| :---------------------- | :---------------------------------------------- |
| **`ring_buffer.h`**     | 对外接口头文件（用户主要引用此文件）。          |
| **`ring_buffer.c`**     | 核心逻辑实现（**无需修改**）。                  |
| **`ring_buffer_hal.h`** | 硬件抽象层接口声明。                            |
| **`ring_buffer_hal.c`** | **硬件适配层**（**需根据你的 MCU 型号修改**）。 |

---

## 🚀 快速开始

### 1. 移植适配 (HAL)

在 `ring_buffer_hal.c` 中，你需要根据具体的芯片（STM32, GD32, ESP32 等）实现 DMA 剩余数据量的获取。

**以 STM32 HAL 库为例：**

```c
// ring_buffer_hal.c
#include "ring_buffer_hal.h"
#include "stm32f4xx_hal.h" // 根据实际型号引入头文件

uint32_t hal_rb_get_dma_head(void *hw_handle, uint32_t buffer_size) {
    if (hw_handle == NULL) return 0;
    
    DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)hw_handle;
    
    // STM32 DMA 计数器是倒计数的 (从 Size -> 0)
    // 当前写入位置 = 总大小 - 剩余量
    return buffer_size - __HAL_DMA_GET_COUNTER(hdma);
}
```

### 2. 初始化

⚠️ **注意**：`size`必须是 **2 的幂** (例如 64, 128, 1024, 4096)，否则初始化函数`rb_init`会返回`-1`.

```c
#include "ring_buffer.h"

// 定义缓冲区 (通常放在特定的 RAM 区域，或声明为 volatile)
#define RX_BUF_SIZE 128 // 必须是 2^N
uint8_t rx_buffer[RX_BUF_SIZE];
ring_buffer_t rb;

// 初始化
void app_init() {
    // 软件模式示例
    if (rb_init(&rb, rx_buffer, RX_BUF_SIZE, RB_MODE_SOFTWARE, NULL) != 0) {
        // 错误处理：Size 不是 2 的幂
        while(1);
    }
}
```

---

## 📖 使用场景示例

### 场景 A：配合 DMA 使用 (高效零拷贝)

适用于 UART/SPI/I2S 等高速数据接收。

1. **配置 DMA**：在 CubeMX 或代码中将 DMA 配置为 **Circular Mode**，数据长度设为 `RX_BUF_SIZE`。

2. **主循环处理**：
```c
// 假设这是你的 DMA 句柄
extern DMA_HandleTypeDef hdma_usart1_rx;

void setup_dma() {
    // 1. 初始化 RingBuffer，传入 DMA 句柄
    rb_init(&rb, rx_buffer, RX_BUF_SIZE, RB_MODE_DMA_CIRCULAR, &hdma_usart1_rx);
    
    // 2. 启动硬件 DMA 接收 (HAL 库示例)
    HAL_UART_Receive_DMA(&huart1, rx_buffer, RX_BUF_SIZE);
}

void main_loop() {
    uint32_t len;
    uint8_t *ptr;

    while (1) {
        // --- 零拷贝获取数据 ---
        // rb_peek_continuous 会返回当前可读的第一段连续内存
        ptr = rb_peek_continuous(&rb, &len);

        if (len > 0) {
            // 直接处理 ptr 指向的数据 (无需 memcpy)
            Process_Data_Protocol(ptr, len);

            // 处理完毕后，手动推进读指针
            rb_skip(&rb, len);
        }
        
        // 可选：低功耗休眠或处理其他任务
    }
}
```

### 场景 B：传统软件模式 (中断/轮询写入)

适用于没有 DMA 或数据量较小的场景。

```c
// 1. 中断服务函数 (写入)
void UART_IRQHandler(void) {
    uint8_t data = UART_Read_Byte();
    // 写入一个字节
    rb_write(&rb, &data, 1);
}

// 2. 主程序 (读取)
void main_task() {
    uint8_t tmp_buf[32];
    // 传统读取方式 (发生一次内存拷贝)
    uint32_t count = rb_read(&rb, tmp_buf, 32);
    
    if (count > 0) {
        // 处理 tmp_buf...
    }
}
```
---

## ⚙️ 原理说明

**为什么必须是 2 的幂？**

本库使用了位运算来优化回绕计算：

- **传统做法：**`head = (head + len) % size;` (涉及除法指令，耗时较多)
- **本库做法：**`head = (head + len) & (size - 1);` (位与指令，单周期完成)

当 `size` 为 $2^n$ 时，`x % size` 数学上等价于 `x & (size - 1)`。

**内存布局与回绕**

当数据写入跨越缓冲区末尾时，`rb_peek_continuous` 会自动截断返回长度。你需要调用两次循环来处理跨边界的数据，但通常这比为了保持连续性而进行内存搬运（memmove）要快得多。

```Plaintext
[ ... Data 2 ... ][ T (Tail) ... Data 1 ... ][ End ]
                  ^ 返回 Data 1 的指针和长度
处理完 Data 1 并 Skip 后 ->
[ T ... Data 2 ... ][ ... Empty ... ][ End ]
^ 再次 Peek 返回 Data 2 的指针和长度
```

---
## :pushpin:测试案例，复制可用

```c
#include <stdio.h>
#include <string.h>
#include "ring_buffer/ring_buffer.h"

/* * 演示配置：
 * 定义一个很小的缓冲区，大小必须是 2 的幂。
 * 这里的 16 (2^4) 方便我们在单行打印中观察所有数据。
 * * !!! 关键注意 !!!
 * 由于逻辑层保留了 1 个字节用于区分满/空，
 * 实际可用容量 = 16 - 1 = 15 字节。
 */
#define DEMO_BUF_SIZE 16

uint8_t demo_mem[DEMO_BUF_SIZE];
ring_buffer_t demo_rb;

/**
 * @brief 辅助函数：可视化打印缓冲区状态
 * 打印 Head, Tail, Count 以及内存的 Hex 视图
 */
void print_status(const char *step_desc)
{
    printf("\n------------------------------------------------------------\n");
    printf("[Step] %s\n", step_desc);

    // 获取当前逻辑状态
    uint32_t count = rb_get_count(&demo_rb);
    uint32_t head = demo_rb.head;
    uint32_t tail = demo_rb.tail;

    printf("State: Head=%-2u Tail=%-2u Count=%-2u (Max Capacity=%u)\n",
           head, tail, count, DEMO_BUF_SIZE - 1);

    printf("Dump : [ ");
    for (int i = 0; i < DEMO_BUF_SIZE; i++)
    {
        // 标记 Head 和 Tail 的位置
        if (i == demo_rb.head && i == demo_rb.tail)
            printf("HT");
        else if (i == demo_rb.head)
            printf("H ");
        else if (i == demo_rb.tail)
            printf("T ");
        else
            printf("  ");

        printf("%02X ", demo_mem[i]);
    }
    printf("]\n");
}

int main(void)
{
    printf("=== Ring Buffer Demo (Software Mode, Size=16) ===\n");

    // ============================================================
    // 1. 初始化
    // ============================================================
    // 将内存初始化为 0xEE，方便看出哪些地方还没被写过
    memset(demo_mem, 0xEE, DEMO_BUF_SIZE);

    // 初始化 RB，软件模式
    if (rb_init(&demo_rb, demo_mem, DEMO_BUF_SIZE, RB_MODE_SOFTWARE, NULL) != 0)
    {
        printf("Init Failed! Size is not power of 2.\n");
        return -1;
    }
    print_status("1. Initialization (Empty)");

    // ============================================================
    // 2. 写入数据 (未回绕)
    // ============================================================
    // 写入 5 个字节: 0x01 ~ 0x05
    uint8_t data_chunk1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    rb_write(&demo_rb, data_chunk1, 5);

    // 预期: Head=5, Tail=0, Count=5
    // 内存: 01 02 03 04 05 EE ...
    print_status("2. Write 5 bytes");

    // ============================================================
    // 3. 读取/跳过部分数据
    // ============================================================
    // 模拟处理了前 2 个字节，手动推进读指针
    rb_skip(&demo_rb, 2);

    // 预期: Head=5, Tail=2, Count=3
    // 有效数据是从索引 2 开始的 (03, 04, 05)
    print_status("3. Skip 2 bytes (Consume 01, 02)");

    // ============================================================
    // 4. 写入数据触发回绕 (Fill to Max Capacity)
    // ============================================================
    // 当前状态: Head=5, Tail=2. Count=3.
    // 剩余容量 (Space) = (Size - 1) - Count = 15 - 3 = 12 字节.
    // 我们正好写入 12 个字节，填满缓冲区。
    // 写入: 0xA1 ~ 0xAC
    uint8_t data_chunk2[] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC};
    rb_write(&demo_rb, data_chunk2, 12);

    // 预期计算:
    // 原 Head = 5. 写入 12.
    // 逻辑位置 = 5 + 12 = 17.
    // 位运算回绕 = 17 & 15 = 1.
    // 新 Head = 1.
    // 此时 Tail = 2. Head(1) + 1 == Tail(2)，缓冲区已满。
    // 索引 1 的位置是保留字节（Gap），虽然里面可能是旧数据或垃圾数据，但逻辑上它不属于有效数据。
    print_status("4. Write 12 bytes (Buffer FULL)");

    // ============================================================
    // 5. 使用 Peek 查看第一段连续数据
    // ============================================================
    // 此时数据在物理上分成了两段：
    // 第一段 (Tail到End): [2]...[15] (03..05 + A1..AB) -> 长度 14
    // 第二段 (Start到Head): [0] (AC) -> 长度 1

    uint32_t len_chunk1 = 0;
    uint8_t *ptr1 = rb_peek_continuous(&demo_rb, &len_chunk1);

    printf("\n>>> Operation: Peek Continuous (1st Call)\n");
    printf("    Returned Address: %p (Buffer + %ld)\n", ptr1, ptr1 - demo_mem);
    printf("    Returned Length : %d\n", len_chunk1);
    printf("    Data Content    : ");
    for (int i = 0; i < len_chunk1; i++)
        printf("%02X ", ptr1[i]);
    printf("\n");

    // 验证逻辑
    if (len_chunk1 == 14 && ptr1 == &demo_mem[2])
    {
        printf("    [Check] Result Correct: Got 14 bytes from index 2.\n");
    }
    else
    {
        printf("    [Check] Result ERROR!\n");
    }

    // ============================================================
    // 6. 处理完第一段后 Skip
    // ============================================================
    rb_skip(&demo_rb, len_chunk1);

    // 预期:
    // Tail 原为 2，加 14 = 16.
    // 16 & 15 = 0.
    // 新 Tail = 0.
    print_status("6. Skip 14 bytes");

    // ============================================================
    // 7. 使用 Peek 查看第二段剩余数据
    // ============================================================
    uint32_t len_chunk2 = 0;
    uint8_t *ptr2 = rb_peek_continuous(&demo_rb, &len_chunk2);

    printf("\n>>> Operation: Peek Continuous (2nd Call)\n");
    printf("    Returned Address: %p (Buffer + %ld)\n", ptr2, ptr2 - demo_mem);
    printf("    Returned Length : %d\n", len_chunk2);
    printf("    Data Content    : %02X\n", ptr2[0]);

    // 验证逻辑
    if (len_chunk2 == 1 && ptr2 == &demo_mem[0])
    {
        printf("    [Check] Result Correct: Got 1 byte from index 0.\n");
    }

    // ============================================================
    // 8. 演示 rb_skip 的边界保护功能
    // ============================================================
    // 当前只剩 1 个字节 (0xAC) 在 Index 0.
    // 我们尝试 Skip 100 个字节，测试会不会导致 Tail 跑飞。
    printf("\n>>> Operation: Try to Skip 100 bytes (Safety Test)\n");
    rb_skip(&demo_rb, 100);

    // 预期:
    // skip 内部发现 100 > count(1)，强制将 len 设为 1。
    // Tail 从 0 变成 1。
    // Head 也是 1。
    // 缓冲区变为空。
    print_status("8. After Safety Skip (Buffer Empty)");

    return 0;
}
```
