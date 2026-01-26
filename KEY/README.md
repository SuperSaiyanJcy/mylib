# Embedded Button Driver (Event-Driven)

这是一个通用的、轻量级的嵌入式按键驱动模块。采用 **C语言面向对象** 思想设计，基于 **状态机 (FSM)** 实现。

该驱动完全将 **核心逻辑** 与 **硬件实现** 分离，支持多按键注册，具备完善的软件消抖（按下与松开双边消抖）机制。

---

## ✨ 特性 (Features)

* **高度解耦**：核心逻辑 (`key.c`) 不包含任何硬件相关代码，移植仅需修改 `key_hal.c`。
* **面向对象**：每个按键通过结构体独立管理，通过链表自动遍历。
* **全事件支持**：支持 **单击**、**双击**、**长按** 以及 **长按松开** (`Long Press Release`) 事件。
* **非阻塞设计**：采用时间戳差值 (`Time Delta`) 算法，不依赖固定周期的定时器中断，可在主循环中随意调用。
* **双边消抖**：支持按下消抖和松开消抖，有效防止机械开关的抖动误触。
* **安全机制**：内部集成链表查重逻辑，防止重复注册导致的死循环。

---

## 📂 文件结构 (File Structure)

| 文件            | 类型 | 说明                                | 修改建议     |
| :-------------- | :--- | :---------------------------------- | :----------- |
| **`key.c`**     | Core | 核心状态机逻辑与链表管理            | **禁止修改** |
| **`key.h`**     | Core | 核心结构体、API 声明、枚举定义      | **禁止修改** |
| **`key_hal.c`** | User | 硬件接口实现、按键定义、回调编写    | **在此开发** |
| **`key_hal.h`** | Conf | 全局参数配置 (消抖时间、长按阈值等) | **在此配置** |

---

## 🚀 快速开始 (Quick Start)

### 1. 配置参数 (`key_hal.h`)

根据实际需求调整全局的时间阈值：

```c
/* 软件消抖开关 (1:开启, 0:关闭) */
#define KEY_DEBOUNCE_EN         1
/* 消抖时间 (ms) - 建议 20ms */
#define KEY_DEBOUNCE_TIME       20

/* 长按判定时间 (ms) */
#define KEY_LONG_PRESS_TIME     1500
/* 双击判定最大间隔 (ms) */
#define KEY_DOUBLE_CLICK_TIME   300
```
### 2. 对接底层接口 (`key_hal.c`)

你需要实现两个基础函数，以对接你的硬件平台（如 STM32, ESP32 等）：

```c
#include "key_hal.h"

/* 1. 获取系统毫秒级时间戳 (必须实现) */
uint32_t Key_GetTickMs(void) {
    return HAL_GetTick(); // 以 STM32 HAL 库为例
}

/* 2. 初始化按键 GPIO (必须实现) */
void Key_GPIO_Init(void) {
    // 在此处初始化 GPIO 引脚为输入模式 (上拉或下拉)
}
```
### 3. 定义按键与回调（`main.c`或`key_hal.c`）
```c
#include "key.h"

/* --- 1. 定义按键句柄 --- */
static Key_t key_menu;

/* --- 2. 编写 GPIO 读取函数 --- */
/* 返回 true 表示按键处于有效电平 (按下状态) */
static bool Read_Menu_Pin(void) {
    // 假设低电平有效
    return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET);
}

/* --- 3. 编写事件回调 --- */
static void OnMenuClick(Key_Handle_t key) {
    printf("Menu Key: Single Click\n");
}

static void OnMenuDouble(Key_Handle_t key) {
    printf("Menu Key: Double Click\n");
}

static void OnMenuLong(Key_Handle_t key) {
    printf("Menu Key: Long Press Start...\n");
}

static void OnMenuLongRelease(Key_Handle_t key) {
    printf("Menu Key: Long Press Released!\n");
}

/* --- 4. 注册按键 --- */
void App_Init(void) {
    // 初始化硬件
    Key_GPIO_Init();
    
    // 配置按键对象
    key_menu.read_pin  = Read_Menu_Pin;       // 绑定硬件读取
    key_menu.cb_single = OnMenuClick;         // 绑定单击
    key_menu.cb_double = OnMenuDouble;        // 绑定双击
    key_menu.cb_long   = OnMenuLong;          // 绑定长按
    key_menu.cb_long_release = OnMenuLongRelease; // 绑定长按松开
    
    // 注册到驱动链表
    Key_Register(&key_menu);
}
```

### 4. 核心循环（`main.c`）

在主循环 (` while(1)`) 或低优先级任务中周期性调用 `Key_Loop()`。

```c
int main(void) {
    HAL_Init();
    App_Init(); // 初始化按键

    while (1) {
        // 核心扫描任务
        // 内部基于时间戳判断，调用频率不影响判定精度
        Key_Loop();
        
        // 其他用户任务...
    }
}
```

---

## ⚙️ 逻辑说明 (Logic Details)

### 状态机流转 (State Machine)

驱动内部维护一个严密的状态机，确保事件互斥且精准：

1.  **IDLE**: 空闲状态。
2.  **DEBOUNCE_PRESS**: 按下消抖。
3.  **PRESS**: 确认按下，开始监测长按。
    * 若时间超过阈值 -> **触发长按 (`cb_long`)** 并置位长按标志。
4.  **WAIT_DOUBLE**: 松开后进入“等待双击”窗口。
    * 若在此窗口内再次按下 -> **触发双击 (`cb_double`)** (清除长按标志)。
    * 若窗口超时 -> **触发单击 (`cb_single`)**。
5.  **WAIT_RELEASE**: 等待按键彻底松开（长按触发后，或双击触发后）。
6.  **DEBOUNCE_RELEASE**: 松开消抖，防止弹片回弹导致的误判。
7.  **DEBOUNCE_FINISH**: 确认彻底松开后：
    * 如果长按标志位有效 -> **触发长按松开 (`cb_long_release`)**。

### 事件优先级

* **长按 (Long Press)**: 优先于松开检测。按下时间超过阈值立即触发。
* **双击 (Double Click)**: 抢占式触发。在双击窗口内检测到第二次按下，立即触发，不等待第二次松开。
* **单击 (Single Click)**: 只有在松开且“双击窗口”超时后才会触发。

---

## ⚠️ 注意事项 (Notes)

1.  **时间溢出**: `Key_GetTickMs` 返回值溢出 (`uint32_t` 翻转) 不会影响驱动逻辑，驱动内部采用了无符号减法计算时间差，逻辑依然正确。
2.  **重复注册**: `Key_Register` 函数内部已实现查重逻辑，重复注册同一个按键句柄会被自动忽略，防止链表成环。
3.  **回调上下文**: 回调函数是在 `Key_Loop` 的上下文中执行的。如果 `Key_Loop` 运行在中断中（不推荐），请不要在回调里执行耗时操作。