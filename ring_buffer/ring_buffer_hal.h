/**
 * @file ring_buffer_hal.h
 * @brief 硬件层接口
 */

#ifndef RING_BUFFER_HAL_H
#define RING_BUFFER_HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 获取 DMA 当前的写入位置索引
 * @param hw_handle 硬件句柄
 * @param buffer_size 缓冲区总大小
 * @return 当前 Write Index (0 ~ buffer_size-1)
 */
uint32_t hal_rb_get_dma_head(void *hw_handle, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // RING_BUFFER_HAL_H