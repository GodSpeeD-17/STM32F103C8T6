/**
 * @file	app_init.h
 * @author	Shrey Shah
 * @brief	Declares application-owned hardware-service initialization
 * @version	v1.0
 * @date	26-08-2026
 *
 * @details
 * @section APP_INIT_H_HIERARCHY Hierarchy
 * - Position: Layer 3 - Application orchestration
 * - Used by: Layer 4 `app_startup`
 * - Uses: Layer 2 application services and Layer 1 RCC and selected BSP capabilities
 *
 * @section APP_INIT_H_RESPONSIBILITY Responsibility
 * This interface exposes the single ordered initialization transaction used
 * before `main`. The implementation configures the clock first and then every
 * application service enabled by `app_config.h`. It configures the optional
 * on-board LED capability when selected. Debug policy additionally directs BSP
 * to configure its fixed full-duplex transport; otherwise application behavior
 * owns any required USART acquisition.
 *
 * @section APP_INIT_H_BOUNDARY Dependency Boundary
 * This header includes shared status types only. RCC, selected timebase
 * Drivers, BSP capabilities, and application-service headers remain private
 * to `app_init.c` and do not leak upward into startup.
 */

// Header Guard
#ifndef APP_INIT_H_
#define APP_INIT_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx_data_types.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Application hardware-service initialization
 * @defgroup App_Init Application Initialization
 * @ingroup App_Template
 * @{
 */

// ==================================================================================================== //
// Public API
// ==================================================================================================== //

/**
 * @brief Initializes the hardware services selected by application policy
 * @details
 * Applies the 72 MHz RCC preset first, initializes the selected SysTick or
 * general-purpose Timer as the fixed 1 ms application timebase, requests the
 * optional BSP-owned TIM4 microsecond-delay transaction, then requests the
 * optional BSP on-board LED transaction. When debug is enabled,
 * BSP_InitUSART() enables the GPIOA/USART1 clocks and configures the fixed
 * PA9/PA10 115200-baud 8N1 full-duplex board transport. Processing stops at
 * the first failed transaction.
 * @returns @ref driver_status_t "Application initialization status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Every enabled application service was initialized
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: An enabled service configuration was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: An enabled service hardware
 * precondition was not satisfied, including a selected board USART clock gate
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: A selected timebase or BSP Timer was running
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: RCC initialization timed out
 * @retval - @ref `DRIVER_STATUS_ERROR`: RCC hardware state could not be decoded
 * @note Timebase hardware is untouched when @ref `APP_TIMEBASE_SOURCE` is
 * @ref `APP_TIME_SOURCE_NONE`
 * @note BSP TIM4 hardware is untouched when @ref `APP_ENABLE_TIMER_US_DELAY` is `0U`
 * @note The on-board LED GPIO is untouched when @ref `APP_ENABLE_ONBOARD_LED` is `0U`
 * @note BSP owns the complete board USART RCC, pin, and peripheral
 * configuration transaction requested here only when debug is enabled
 */
driver_status_t App_Init(void);

/** @} */

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_INIT_H_ */
