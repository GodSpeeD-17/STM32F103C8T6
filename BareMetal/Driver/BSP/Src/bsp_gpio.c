/**
 * @file	bsp_gpio.c
 * @author	Shrey Shah
 * @brief	Implements the Blue Pill board GPIO capability
 * @version	v1.0
 * @date	26-08-2026
 *
 * @details
 * This source owns the cohesive RCC and GPIO initialization transaction for
 * the active-low PC13 on-board LED. The independently selectable board USART
 * capability remains isolated in `bsp_usart.c`.
 */

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "bsp_gpio.h"
#include "rcc.h"

#ifdef STM32F103C8T6__

// ==================================================================================================== //
// Private Configuration
// ==================================================================================================== //

/** @brief GPIOC clock gate required by the on-board LED route @def BSP_GPIO_OB_LED_CLOCK_ENABLE_MASK */
#define BSP_GPIO_OB_LED_CLOCK_ENABLE_MASK		(RCC_APB2ENR_IOPCEN)

// ==================================================================================================== //
// Local Helpers
// ==================================================================================================== //

/**
 * @brief Enables the GPIOC clock required by the on-board LED
 * @returns @ref driver_status_t "On-board LED clock-enable status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: GPIOC clock was enabled
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: The fixed APB2 clock mapping was rejected
 */
static inline driver_status_t BSP_GPIO_EnableOBLEDClock(void)
{
	//! Keep RCC ownership inside the board capability so callers request one complete initialization transaction.
	return RCC_SetPeripheralClockState(RCC_APB2_BUS, BSP_GPIO_OB_LED_CLOCK_ENABLE_MASK, DRIVER_STATUS_ON);
}

/**
 * @brief Configures PC13 and establishes the deterministic inactive LED state
 * @returns @ref driver_status_t "On-board LED GPIO-configuration status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: PC13 was configured and its latch was set high
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: The fixed board GPIO mapping was rejected
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: GPIOC clock validation or GPIO staging failed
 */
static inline driver_status_t BSP_GPIO_ConfigureOBLED(void)
{
	//! Configure the board route before publishing its inactive latch state so consumers never observe an uninitialized output mode.
	ASSERT_DRIVER_STATUS(GPIO_LED_Init(BSP_GPIO_OB_LED_PORT, BSP_GPIO_OB_LED_PIN));
	//! PC13 is active-low, so setting its latch completes initialization with the LED visibly off.
	return GPIO_PinSet(BSP_GPIO_OB_LED_PORT, BSP_GPIO_OB_LED_PIN);
}

// ==================================================================================================== //
// Public API
// ==================================================================================================== //

driver_status_t BSP_InitOBLED(void)
{
	//! Acquire the board GPIO resource first, then configure the only pin owned by this capability.
	ASSERT_DRIVER_STATUS(BSP_GPIO_EnableOBLEDClock());
	return BSP_GPIO_ConfigureOBLED();
}

void BSP_OB_LED_Set(void)
{
	//! The Blue Pill LED is active-low, so reset PC13's latch to make the LED electrically active.
	(void) GPIO_PinReset(BSP_GPIO_OB_LED_PORT, BSP_GPIO_OB_LED_PIN);
}

void BSP_OB_LED_Reset(void)
{
	//! The Blue Pill LED is active-low, so set PC13's latch to return the LED to its inactive state.
	(void) GPIO_PinSet(BSP_GPIO_OB_LED_PORT, BSP_GPIO_OB_LED_PIN);
}

void BSP_OB_LED_Toggle(void)
{
	//! Delegating the latch transition to GPIO preserves the pin-level atomicity expected by board LED consumers.
	(void) GPIO_PinToggle(BSP_GPIO_OB_LED_PORT, BSP_GPIO_OB_LED_PIN);
}

#endif /* STM32F103C8T6__ */
