/*
 * base64.h
 * 无动态内存分配的 Base64 编解码库 (C99)
 */

#ifndef BASE64_H
#define BASE64_H

#include <stddef.h> // for size_t
#include <stdint.h> // for uint8_t

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * === 辅助宏：计算缓冲区大小 ===
 * 用于在声明数组时确定所需的最小长度。
 */

// 计算编码 N 字节数据所需的缓冲区大小 (包含结尾的 NULL)
// 公式: 4 * ceil(N / 3) + 1
#define BASE64_ENCODE_OUT_SIZE(n) ((((n) + 2) / 3) * 4 + 1)

// 计算解码 N 字节 Base64 字符串所需的最大缓冲区大小
// 公式: 3 * (N / 4)
#define BASE64_DECODE_OUT_SIZE(n) (((n) / 4) * 3)

/*
    * === API 函数声明 ===
    */

/**
 * @brief Base64 编码
 * * @param src       [in] 输入的二进制数据
 * @param src_len   [in] 输入数据的长度 (字节数)
 * @param dst       [out] 输出缓冲区 (必须足够大)
 * @param dst_size  [in] 输出缓冲区的大小 (使用 BASE64_ENCODE_OUT_SIZE 计算)
 * @return int      成功返回编码后的字符串长度(不含NULL)，失败返回 -1 (缓冲区不足)
 */
int base64_encode(const uint8_t *src, size_t src_len, char *dst, size_t dst_size);

/**
 * @brief Base64 解码
 * * @param src       [in] 输入的 Base64 字符串
 * @param src_len   [in] 输入字符串的长度 (必须是 4 的倍数)
 * @param dst       [out] 输出缓冲区 (用于存放二进制数据)
 * @param dst_size  [in] 输出缓冲区的大小 (使用 BASE64_DECODE_OUT_SIZE 计算)
 * @return int      成功返回解码后的字节数，失败返回 -1 (格式错误或缓冲区不足)
 */
int base64_decode(const char *src, size_t src_len, uint8_t *dst, size_t dst_size);

#ifdef __cplusplus
}
#endif

#endif // BASE64_H