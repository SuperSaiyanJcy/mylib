#include "key_hal.h"
#include "key.h" // Needed for Key_Handle_t definition

/* Include MCU HAL (e.g., #include "stm32f4xx_hal.h") */
// #include "main.h"

/* ================= Implementation ================= */

uint32_t Key_GetTickMs(void)
{
    /* Return system tick in ms (e.g. HAL_GetTick()) */
    // return HAL_GetTick();
    return 0;
}

void Key_GPIO_Init(void)
{
    /* Initialize GPIO pins here */
    // __HAL_RCC_GPIOA_CLK_ENABLE();
    // GPIO_InitTypeDef GPIO_InitStruct = {0};
    // GPIO_InitStruct.Pin = GPIO_PIN_0;
    // GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    // HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
