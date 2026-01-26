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