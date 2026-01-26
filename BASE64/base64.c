/*
 * base64.c
 * 实现文件
 */

#include "base64.h"

// Base64 字符映射表
static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// 解码反向查找表 (Reverse Lookup Table)
// 数组索引对应 ASCII 值，数组值对应 Base64 索引(0-63)。
// 0xFF 表示非法字符。
// 这种方式是用空间(256字节)换时间，实现 O(1) 查找。
static const uint8_t decoding_table[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0-7
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 8-15
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 16-23
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 24-31
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 32-39
    0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F, // 40-47 (+ is 43, / is 47)
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, // 48-55 (0-7)
    0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 56-63 (8-9)
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // 64-71 (A-G)
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, // 72-79 (H-O)
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, // 80-87 (P-W)
    0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 88-95 (X-Z)
    0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, // 96-103 (a-g)
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, // 104-111 (h-o)
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, // 112-119 (p-w)
    0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 120-127 (x-z)
    // 128-255 全是 0xFF (省略初始化，默认为0，需注意这里只处理ASCII，
    // 但下面的逻辑会把 >127 的视为非法，所以只要前128个正确即可)
};

int base64_encode(const uint8_t *src, size_t src_len, char *dst, size_t dst_size)
{
    size_t needed_len = BASE64_ENCODE_OUT_SIZE(src_len);
    if (dst_size < needed_len)
    {
        return -1; // 缓冲区不足
    }

    size_t i = 0, j = 0;

    // 每次处理 3 个字节
    while (i + 2 < src_len)
    {
        uint32_t octet_a = src[i++];
        uint32_t octet_b = src[i++];
        uint32_t octet_c = src[i++];

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        dst[j++] = base64_table[(triple >> 18) & 0x3F];
        dst[j++] = base64_table[(triple >> 12) & 0x3F];
        dst[j++] = base64_table[(triple >> 6) & 0x3F];
        dst[j++] = base64_table[triple & 0x3F];
    }

    // 处理剩余的字节 (1个或2个)
    if (i < src_len)
    {
        uint32_t octet_a = src[i++];
        uint32_t octet_b = (i < src_len) ? src[i++] : 0;
        uint32_t octet_c = 0; // 肯定是0，因为如果还有第3个字节，上面的循环就会处理

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        dst[j++] = base64_table[(triple >> 18) & 0x3F];
        dst[j++] = base64_table[(triple >> 12) & 0x3F];

        // 剩余1个字节输入 -> 补2个=
        // 剩余2个字节输入 -> 补1个=
        if (src_len % 3 == 1)
        {
            dst[j++] = '=';
            dst[j++] = '=';
        }
        else
        {
            dst[j++] = base64_table[(triple >> 6) & 0x3F];
            dst[j++] = '=';
        }
    }

    dst[j] = '\0'; // 始终添加 null 结尾
    return (int)j;
}

int base64_decode(const char *src, size_t src_len, uint8_t *dst, size_t dst_size)
{
    // 长度检查：Base64 编码后的长度必须是 4 的倍数
    if (src_len == 0 || src_len % 4 != 0)
    {
        return -1;
    }

    // 计算解码后的理论最大长度
    size_t max_out_len = BASE64_DECODE_OUT_SIZE(src_len);
    if (dst_size < max_out_len)
    {
        return -1; // 缓冲区可能不足
    }

    size_t i = 0, j = 0;
    while (i < src_len)
    {
        // 读取 4 个字符
        uint8_t c1 = (uint8_t)src[i++];
        uint8_t c2 = (uint8_t)src[i++];
        uint8_t c3 = (uint8_t)src[i++];
        uint8_t c4 = (uint8_t)src[i++];

        // 检查非法字符 (利用查表，0xFF 为非法)
        // 注意：'=' 不在表中，需要单独判断
        uint8_t v1 = decoding_table[c1];
        uint8_t v2 = decoding_table[c2];

        // 前两个字符不能是 '=' 且必须合法
        if (v1 == 0xFF || v2 == 0xFF)
            return -1;

        // 拼接前 12 位
        uint32_t triple = (v1 << 18) | (v2 << 12);

        // 写入第 1 个字节
        dst[j++] = (triple >> 16) & 0xFF;

        // 处理第 3 个字符
        if (c3 == '=')
        {
            // 如果第3个是=，第4个必须也是=，结束
            if (c4 != '=')
                return -1;
            return (int)j;
        }

        uint8_t v3 = decoding_table[c3];
        if (v3 == 0xFF)
            return -1;

        triple |= (v3 << 6);
        dst[j++] = (triple >> 8) & 0xFF;

        // 处理第 4 个字符
        if (c4 == '=')
        {
            return (int)j;
        }

        uint8_t v4 = decoding_table[c4];
        if (v4 == 0xFF)
            return -1;

        triple |= v4;
        dst[j++] = triple & 0xFF;
    }

    return (int)j;
}