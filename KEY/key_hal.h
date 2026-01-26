#ifndef __KEY_HAL_H__
#define __KEY_HAL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================= Configuration ================= */

/* Debounce Logic (Applies to both Press and Release) */
#define KEY_DEBOUNCE_EN 1    // 1: Enable, 0: Disable
#define KEY_DEBOUNCE_TIME 20 // ms

/* Time Thresholds */
#define KEY_LONG_PRESS_TIME 1500  // ms
#define KEY_DOUBLE_CLICK_TIME 300 // ms

/* ================= Hardware API ================= */

/* To be implemented in key_hal.c */
uint32_t Key_GetTickMs(void); // System Tick
void Key_GPIO_Init(void);     // GPIO Init

#ifdef __cplusplus
}
#endif

#endif /* __KEY_HAL_H__ */
