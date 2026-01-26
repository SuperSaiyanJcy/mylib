#ifndef __KEY_H__
#define __KEY_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "key_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Types Definitions --- */

/* Forward declaration */
struct KeyStruct;

/* Handle type definition */
typedef struct KeyStruct *Key_Handle_t;

/* Function pointers */
typedef bool (*KeyReadPinFn_t)(void);
typedef void (*KeyCallbackFn_t)(Key_Handle_t key);

/* State Machine States */
typedef enum
{
    KEY_STATE_IDLE = 0,
    KEY_STATE_DEBOUNCE_PRESS,   // Debouncing Press
    KEY_STATE_PRESS,            // Stable Press (Holding)
    KEY_STATE_DEBOUNCE_RELEASE, // Debouncing Release
    KEY_STATE_WAIT_DOUBLE,      // Waiting for second click
    KEY_STATE_WAIT_RELEASE,     // Waiting for full release
    KEY_STATE_DEBOUNCE_FINISH   // Debouncing final release
} KeyState_e;

/* Key Object Structure */
typedef struct KeyStruct
{
    /* --- Configuration (Set by User) --- */
    KeyReadPinFn_t read_pin; // Hardware read function

    KeyCallbackFn_t cb_single;       // Single click callback
    KeyCallbackFn_t cb_double;       // Double click callback
    KeyCallbackFn_t cb_long;         // Long press callback
    KeyCallbackFn_t cb_long_release; // Long press release callback

    /* --- Internal State (Managed by Driver) --- */
    KeyState_e state;       // Current FSM State
    uint32_t timestamp;     // Time marker for events
    bool long_press_active; // Flag: Is long press currently active?
    Key_Handle_t next;      // Linked list pointer
} Key_t;

/* --- API Functions --- */

/**
 * @brief Register a key to the scanning list
 * @param key Handle to the key object (Pointer to struct)
 */
void Key_Register(Key_Handle_t key);

/**
 * @brief Main logic loop. Call this frequently (e.g. in main loop)
 */
void Key_Loop(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H__ */
