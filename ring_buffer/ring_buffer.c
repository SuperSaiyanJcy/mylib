/**
 * @file ring_buffer.c
 * @brief 环形缓冲区逻辑实现
 */

#include "ring_buffer.h"
#include "ring_buffer_hal.h" /* 引入硬件抽象层 */
#include <string.h>          /* 用于 memcpy */

/* 宏：检查 x 是否为 2 的幂。原理：(x & (x-1)) == 0 表示只有一位是1 */
#define IS_POWER_OF_TWO(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))

/**
 * @brief 内部内联函数：获取当前写指针位置
 * @note 使用 static inline 减少函数调用开销，编译器会直接展开代码
 */
static inline uint32_t _rb_get_head_pos(ring_buffer_t *rb)
{
    // 判断当前模式
    if (rb->mode == RB_MODE_DMA_CIRCULAR)
    {
        // DMA模式：写指针由硬件维护，需调用 HAL 层查询寄存器
        return hal_rb_get_dma_head(rb->hw_handle, rb->size);
    }
    else
    {
        // 软件模式：写指针存储在结构体中
        return rb->head;
    }
}

/* ==========================================
 * 接口实现
 * ========================================== */

int rb_init(ring_buffer_t *rb, uint8_t *buffer, uint32_t size, rb_mode_t mode, void *hw_handle)
{
    // 1. 基础参数空指针检查
    if (!rb || !buffer)
        return -1;

    // 2. 核心约束检查：Size 必须是 2 的幂
    // 如果不是 2 的幂，后续的位运算逻辑将失效，必须报错
    if (!IS_POWER_OF_TWO(size))
    {
        return -1;
    }

    // 3. 结构体初始化
    rb->buffer = buffer; // 绑定内存
    rb->size = size;     // 记录总大小
    rb->mask = size - 1; // 计算位掩码 (例: 128 -> 0x7F)

    rb->head = 0;              // 写指针归零
    rb->tail = 0;              // 读指针归零
    rb->mode = mode;           // 设置模式
    rb->hw_handle = hw_handle; // 绑定硬件句柄

    return 0; // 初始化成功
}

uint32_t rb_get_count(ring_buffer_t *rb)
{
    // 1. 获取当前写指针 (内联展开)
    uint32_t head = _rb_get_head_pos(rb);
    // 2. 获取当前读指针
    uint32_t tail = rb->tail;

    // 3. 利用无符号整数溢出特性和位运算计算距离
    // 即使 head < tail (发生了回绕)，(head - tail) 也会得到很大的正数
    // 再与 mask 进行 AND 运算，即可得到真实的环形距离
    return (head - tail) & rb->mask;
}

uint32_t rb_write(ring_buffer_t *rb, const uint8_t *data, uint32_t len)
{
    // 1. 安全检查：DMA 模式下不允许软件写入，直接返回 0
    if (rb->mode == RB_MODE_DMA_CIRCULAR)
        return 0;

    // 2. 参数检查：源指针为空或长度为0，直接返回
    if (data == NULL || len == 0)
        return 0;

    // 3. 计算当前缓冲区内的数据量
    uint32_t count = rb_get_count(rb);

    // 4. 计算剩余可用空间
    // 保留 1 个字节不使用。
    // 如果 size=128, count=0, 则 space = 127。
    // 如果填满 128 字节，Head 将追上 Tail，导致满空无法区分。
    uint32_t space = (rb->size - 1) - count;

    // 5. 如果空间已满，直接返回
    if (space == 0)
        return 0;

    // 6. 限制写入长度，不能超过剩余空间
    if (len > space)
        len = space;

    // 7. 执行写入操作
    uint32_t head = rb->head;
    uint32_t to_end = rb->size - head; // 当前 Head 到缓冲区末尾的距离

    if (len <= to_end)
    {
        // 情况 A: 写入不跨越边界
        memcpy(&rb->buffer[head], data, len);
    }
    else
    {
        // 情况 B: 写入跨越边界 (分两段)
        // 第一段：填满末尾
        memcpy(&rb->buffer[head], data, to_end);
        // 第二段：剩余数据写到开头
        memcpy(&rb->buffer[0], data + to_end, len - to_end);
    }

    // 8. 更新写指针 (位运算回绕)
    rb->head = (rb->head + len) & rb->mask;

    // 9. 返回实际写入的字节数
    return len;
}

uint8_t *rb_peek_continuous(ring_buffer_t *rb, uint32_t *len)
{
    // 1. 获取数据量
    uint32_t count = rb_get_count(rb);

    // 2. 如果无数据
    if (count == 0)
    {
        // 安全性检查：如果用户传入了有效的 len 指针才赋值
        if (len)
            *len = 0;
        return NULL;
    }

    // 3. 计算 Tail 到缓冲区末尾的距离
    uint32_t tail = rb->tail;
    uint32_t to_end = rb->size - tail;

    // 4. 计算当前这段连续数据的长度
    uint32_t continuous_len;
    if (count <= to_end)
    {
        // 数据未回绕，或者是回绕了但还没读到回绕点
        continuous_len = count;
    }
    else
    {
        // 数据已回绕，这里只返回 Tail 到 End 的那一段
        continuous_len = to_end;
    }

    // 5. 安全性检查：赋值输出长度
    if (len)
        *len = continuous_len;

    // 6. 返回当前读指针的数据地址
    return &rb->buffer[tail];
}

void rb_skip(ring_buffer_t *rb, uint32_t len)
{
    // 1. 获取当前实际数据量
    uint32_t count = rb_get_count(rb);

    // 2. 【修改点】逻辑边界保护
    // 如果请求跳过的长度大于实际拥有的长度，强制限制为当前长度。
    // 这防止了 Tail 指针逻辑上跑到了 Head 指针的前面（超前）。
    if (len > count)
    {
        len = count;
    }

    // 3. 更新读指针 (位运算回绕)
    // 即使 len 很大，位运算本身也是安全的不会越界，
    // 但上面的 len > count 检查保证了业务逻辑的正确性。
    rb->tail = (rb->tail + len) & rb->mask;
}

uint32_t rb_read(ring_buffer_t *rb, uint8_t *dest, uint32_t max_len)
{
    // 1. 参数检查：目标指针为空或长度为0，直接返回
    if (dest == NULL || max_len == 0)
        return 0;
    
    // 2. 利用 peek 获取第一段数据
    uint32_t chunk_len;
    uint8_t *chunk_ptr = rb_peek_continuous(rb, &chunk_len);

    // 3. 如果没数据，直接返回
    if (chunk_len == 0)
        return 0;

    // 4. 决定本次拷贝多少
    uint32_t to_read = (max_len < chunk_len) ? max_len : chunk_len;

    // 5. 拷贝内存
    memcpy(dest, chunk_ptr, to_read);

    // 6. 推进指针 (rb_skip 内部已有保护，这里调用是安全的)
    rb_skip(rb, to_read);

    // 7. 检查是否需要读取第二段 (跨越边界回绕的情况)
    uint32_t total_read = to_read;

    // 如果用户想要的还没给够
    if (max_len > total_read)
    {
        // 再次 peek。由于刚才 skip 了，如果之前是回绕状态，现在 tail 应该变成了 0
        chunk_ptr = rb_peek_continuous(rb, &chunk_len);

        // 如果还有数据 (说明确实跨越了边界)
        if (chunk_len > 0)
        {
            // 计算剩余需要多少
            uint32_t remain = max_len - total_read;
            // 取最小值
            uint32_t second_read = (remain < chunk_len) ? remain : chunk_len;

            // 拷贝第二段
            memcpy(dest + total_read, chunk_ptr, second_read);

            // 再次推进
            rb_skip(rb, second_read);

            // 累加总长度
            total_read += second_read;
        }
    }

    return total_read;
}