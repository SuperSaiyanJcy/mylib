#include "key.h"

/* Linked list head */
static Key_Handle_t head_handle = NULL;

void Key_Register(Key_Handle_t key)
{
    if (key == NULL || key->read_pin == NULL)
        return;

    /* 1. Prevent duplicate registration (Check for loops) */
    Key_Handle_t cur = head_handle;
    while (cur != NULL)
    {
        if (cur == key)
        {
            return; // Already registered, abort to prevent circular link
        }
        cur = cur->next;
    }

    /* 2. Reset internal state */
    key->state = KEY_STATE_IDLE;
    key->timestamp = 0;
    key->long_press_active = false;

    /* 3. Head insertion */
    key->next = head_handle;
    head_handle = key;
}

static void Key_Engine(Key_Handle_t key, uint32_t current_time)
{
    bool is_pressed = key->read_pin();

    switch (key->state)
    {
    /* ---------------- IDLE ---------------- */
    case KEY_STATE_IDLE:
        if (is_pressed)
        {
            key->timestamp = current_time;
            key->long_press_active = false; // Reset flag
#if KEY_DEBOUNCE_EN
            key->state = KEY_STATE_DEBOUNCE_PRESS;
#else
            key->state = KEY_STATE_PRESS;
#endif
        }
        break;

        /* ---------------- DEBOUNCE PRESS ---------------- */
#if KEY_DEBOUNCE_EN
    case KEY_STATE_DEBOUNCE_PRESS:
        if (!is_pressed)
        {
            key->state = KEY_STATE_IDLE; // False trigger (Glitch)
        }
        else if ((current_time - key->timestamp) >= KEY_DEBOUNCE_TIME)
        {
            key->state = KEY_STATE_PRESS;  // Confirmed Press
            key->timestamp = current_time; // Reset time for Long Press check
        }
        break;
#endif

    /* ---------------- STABLE PRESS ---------------- */
    case KEY_STATE_PRESS:
        if (is_pressed)
        {
            // Check for Long Press
            if ((current_time - key->timestamp) >= KEY_LONG_PRESS_TIME)
            {
                if (key->cb_long)
                    key->cb_long(key);

                key->long_press_active = true; // Mark long press as active
                key->state = KEY_STATE_WAIT_RELEASE;
            }
        }
        else
        {
            // Released -> Go to debounce release or wait double
            key->timestamp = current_time;
#if KEY_DEBOUNCE_EN
            key->state = KEY_STATE_DEBOUNCE_RELEASE;
#else
            key->state = KEY_STATE_WAIT_DOUBLE;
#endif
        }
        break;

        /* ---------------- DEBOUNCE RELEASE (Press -> WaitDouble) ---------------- */
#if KEY_DEBOUNCE_EN
    case KEY_STATE_DEBOUNCE_RELEASE:
        if (is_pressed)
        {
            // Signal bounced back to high -> still pressed
            key->state = KEY_STATE_PRESS;
            key->timestamp = current_time;
        }
        else if ((current_time - key->timestamp) >= KEY_DEBOUNCE_TIME)
        {
            // Confirmed Release -> Start Double Click Window
            key->state = KEY_STATE_WAIT_DOUBLE;
            key->timestamp = current_time;
        }
        break;
#endif

    /* ---------------- WAIT DOUBLE CLICK ---------------- */
    case KEY_STATE_WAIT_DOUBLE:
        if (is_pressed)
        {
            // Second press detected -> Double Click
            if (key->cb_double)
                key->cb_double(key);

            key->long_press_active = false; // Double click invalidates long press release
            key->state = KEY_STATE_WAIT_RELEASE;
        }
        else if ((current_time - key->timestamp) >= KEY_DOUBLE_CLICK_TIME)
        {
            // Timeout -> Single Click
            if (key->cb_single)
                key->cb_single(key);
            key->state = KEY_STATE_IDLE; // Single click sequence done
        }
        break;

    /* ---------------- WAIT FULL RELEASE ---------------- */
    case KEY_STATE_WAIT_RELEASE:
        if (!is_pressed)
        {
            key->timestamp = current_time;
#if KEY_DEBOUNCE_EN
            key->state = KEY_STATE_DEBOUNCE_FINISH;
#else
            // If debounce disabled, check release immediately
            if (key->long_press_active)
            {
                if (key->cb_long_release)
                    key->cb_long_release(key);
                key->long_press_active = false;
            }
            key->state = KEY_STATE_IDLE;
#endif
        }
        break;

        /* ---------------- DEBOUNCE FINISH (WaitRelease -> Idle) ---------------- */
#if KEY_DEBOUNCE_EN
    case KEY_STATE_DEBOUNCE_FINISH:
        if (is_pressed)
        {
            key->state = KEY_STATE_WAIT_RELEASE; // Bounced back
        }
        else if ((current_time - key->timestamp) >= KEY_DEBOUNCE_TIME)
        {
            // Fully Released: Check for Long Press Release Event
            if (key->long_press_active)
            {
                if (key->cb_long_release)
                    key->cb_long_release(key);
                key->long_press_active = false;
            }

            key->state = KEY_STATE_IDLE; // Reset FSM
        }
        break;
#endif

    default:
        key->state = KEY_STATE_IDLE;
        break;
    }
}

void Key_Loop(void)
{
    uint32_t now = Key_GetTickMs();
    Key_Handle_t cur = head_handle;

    while (cur != NULL)
    {
        Key_Engine(cur, now);
        cur = cur->next;
    }
}
