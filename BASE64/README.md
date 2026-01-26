# Tiny C99 Base64 Library

这是一个专为 **嵌入式系统** 和 **高性能场景** 设计的轻量级 C99 Base64 编解码库。

---

## ✨ 特性 (Features)

* **零动态内存分配 (Zero Malloc)**: 所有缓冲区由调用者提供，无需 `malloc`/`free`，杜绝内存泄漏和碎片化，非常适合 STM32, ESP32 等单片机环境。
* **二进制安全 (Binary Safe)**: 严格通过长度参数处理数据，完美支持图片、音频、加密密文等包含 `0x00` 的二进制数据。
* **高性能 (High Performance)**: 解码过程使用 O(1) 查表法，比传统的 `strchr` 查找方式快得多。
* **安全防溢出 (Buffer Safe)**: API 强制要求传入缓冲区大小，并提供辅助宏计算所需空间，防止缓冲区溢出。
* **标准兼容**: 符合标准 Base64 (RFC 4648) 规范，使用标准 C99 编写，无第三方依赖。

---

## 📂 文件结构 (File Structure)

```text
.
├── BASE64/
│   ├── base64.c      # 实现核心逻辑
│   └── base64.h      # 头文件与 API 声明
├── main.c            # 示例程序
├── Makefile          # 构建脚本
└── README.md         # 说明文档
```
---

## 🚀 快速开始 (Quick Start)

**1. API 概览**

在使用前，请包含头文件：

```c
#include "BASE64/base64.h"
```
库提供了两个核心宏来帮助你分配栈内存：
- `BASE64_ENCODE_OUT_SIZE(n)`:计算编码`n`字节数据所需的缓冲区大小（含`\0`）。
- `BASE64_DECODE_OUT_SIZE(n)`:计算解码`n`字节字符串所需的最大缓冲区大小。

**2. 编码示例**
```c
#include <stdio.h>
#include "BASE64/base64.h"

int main() {
    // 原始二进制数据
    uint8_t data[] = {0x48, 0x65, 0x00, 0x6C, 0x6C, 0x6F}; // "He\0llo"
    size_t len = sizeof(data);

    // 1. 准备缓冲区 (使用宏计算大小)
    char encoded_buf[BASE64_ENCODE_OUT_SIZE(sizeof(data))];

    // 2. 执行编码
    int ret = base64_encode(data, len, encoded_buf, sizeof(encoded_buf));

    if (ret >= 0) {
        printf("Base64 Result: %s\n", encoded_buf);
    } else {
        printf("Buffer too small!\n");
    }
    return 0;
}
```
**3. 解码示例**
```c
#include <stdio.h>
#include "BASE64/base64.h"

int main() {
    const char *str = "SGVsbG8=";
    size_t len = strlen(str);

    // 1. 准备缓冲区
    uint8_t decoded_buf[BASE64_DECODE_OUT_SIZE(strlen(str))];

    // 2. 执行解码
    int ret = base64_decode(str, len, decoded_buf, sizeof(decoded_buf));

    if (ret >= 0) {
        printf("Decoded size: %d bytes\n", ret);
        // 处理解码后的二进制数据...
    } else {
        printf("Decode failed (Invalid char or buffer too small)\n");
    }
    return 0;
}
```

