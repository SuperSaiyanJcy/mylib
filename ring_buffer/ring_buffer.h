/**
 * @file ring_buffer.h
 * @brief 环形缓冲区对外接口头文件 (C99标准)
 * @details 核心特性：
 * 1. 要求缓冲区大小为 2 的幂，使用位运算优化索引计算。
 * 2. 预留 1 字节空间用于区分“满”和“空”状态。
 * 3. 支持软件写入模式和 DMA 循环写入模式。
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>  // 包含 uint32_t, uint8_t 等类型
#include <stdbool.h> // 包含 bool 类型
#include <stddef.h>  // 包含 NULL 定义

#ifdef __cplusplus
extern "C"
{
#endif

/* ==========================================
 * 类型定义
 * ========================================== */

/**
 * @brief 缓冲区运行模式枚举
 */
typedef enum
{
    RB_MODE_SOFTWARE = 0, /* 软件模式：CPU 显式调用 rb_write 写入 */
    RB_MODE_DMA_CIRCULAR  /* DMA模式：硬件自动写入，软件仅负责读取 */
} rb_mode_t;

/**
 * @brief 环形缓冲区控制句柄
 */
typedef struct
{
    uint8_t *buffer; /* 指向实际内存数组的指针 */
    uint32_t size;   /* 缓冲区总容量 (必须是 2 的幂) */
    uint32_t mask;   /* 掩码 (size - 1)，用于位运算代替取余 */

    volatile uint32_t head; /* 写索引 (Head)，指向下一个写入位置 */
    volatile uint32_t tail; /* 读索引 (Tail)，指向下一个读取位置 */

    rb_mode_t mode;  /* 当前模式 */
    void *hw_handle; /* 硬件层句柄 (DMA模式下使用) */
} ring_buffer_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/**
 * @brief 初始化环形缓冲区
 * @param rb [出参] 控制块指针
 * @param buffer [入参] 实际存储数据的数组地址
 * @param size [入参] 数组大小 (警告：必须是 2 的幂，如 64, 128)
 * @param mode [入参] 运行模式
 * @param hw_handle [入参] 硬件句柄 (DMA模式必填，否则传 NULL)
 * @return 0: 成功, -1: 参数错误 (如 size 不是 2 的幂)
 */
int rb_init(ring_buffer_t *rb, uint8_t *buffer, uint32_t size, rb_mode_t mode, void *hw_handle);

/**
 * @brief 获取缓冲区内当前有效的数据长度
 * @param rb 句柄
 * @return 数据字节数
 */
uint32_t rb_get_count(ring_buffer_t *rb);

/**
 * @brief [软件模式] 向缓冲区写入数据
 * @note 会保留 1 字节空间不使用，防止满/空状态混淆
 * @param rb 句柄
 * @param data 数据源
 * @param len 期望写入的长度
 * @return 实际写入的长度 (如果空间不足，可能小于 len)
 */
uint32_t rb_write(ring_buffer_t *rb, const uint8_t *data, uint32_t len);

/**
 * @brief 读取数据到目标数组 (发生内存拷贝)
 * @param rb 句柄
 * @param dest 目标数组
 * @param max_len 最大读取长度
 * @return 实际读取到的字节数
 */
uint32_t rb_read(ring_buffer_t *rb, uint8_t *dest, uint32_t max_len);

/**
 * @brief [零拷贝] 获取一段连续的可读内存指针
 * @note 如果数据跨越了缓冲区末尾，此函数只返回 Tail 到末尾的那一段
 * @param rb 句柄
 * @param len [出参] 输出这段连续内存的长度 (如果为 NULL 则不输出)
 * @return 指向数据段的指针 (无数据返回 NULL)
 */
uint8_t *rb_peek_continuous(ring_buffer_t *rb, uint32_t *len);

/**
 * @brief 丢弃/跳过数据 (通常配合 peek 使用)
 * @note 包含边界检查，防止跳过超过存在的数据量
 * @param rb 句柄
 * @param len 要跳过的字节数
 */
void rb_skip(ring_buffer_t *rb, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // RING_BUFFER_H
