/**
 * @file ring_buffer_hal.c
 * @brief 环形缓冲区硬件抽象层实现
 * @note **用户需根据具体的 MCU 型号修改此文件**
 */

#include "ring_buffer_hal.h"
#include <stddef.h> /* 引入 NULL */

/* ============================================================
 * 第一步：包含你的 MCU 对应的 HAL 库头文件
 * ============================================================ */
// #include "stm32f4xx_hal.h"  // 例如：对于 STM32F4
// #include "gd32f10x.h"       // 例如：对于 GD32F1

/* ============================================================
 * 第二步：实现获取 DMA 指针的函数
 * ============================================================ */

uint32_t hal_rb_get_dma_head(void *hw_handle, uint32_t buffer_size)
{
    // 1. 安全检查
    if (hw_handle == NULL)
        return 0;

    /* * [适配区]
     * 请根据具体的硬件库实现获取 DMA 剩余传输量 (Counter)
     */

    // 示例：STM32 HAL 库
    // DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)hw_handle;
    // uint32_t remaining = __HAL_DMA_GET_COUNTER(hdma);

    // 示例：标准库
    // uint32_t remaining = DMA_GetCurrDataCounter(DMA1_Channel1);

    // 模拟数据：假设没有硬件，返回 0
    uint32_t remaining = 0;

    // 2. 计算并返回当前索引
    // 大多数 DMA 寄存器是倒计数的，所以用 Size - Remaining
    return buffer_size - remaining;
}