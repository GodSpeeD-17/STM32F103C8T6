/**
 * @file	app_time.h
 * @author	Shrey Shah
 * @brief	Declares application timebase and blocking-delay services
 * @version	v1.0
 * @date	26-08-2026
 *
 * @details
 * @section APP_TIME_H_HIERARCHY Hierarchy
 * - Position: Layer 2 - Application service
 * - Used by: Layer 3 `app_init` and optional application behavior
 * - Uses: One selected Layer 1 SysTick or Timer path and optional BSP Timer
 *
 * @section APP_TIME_H_RESPONSIBILITY Responsibility
 * This service configures the selected source as one fixed 1 ms
 * interrupt-driven timebase, owns the monotonic software tick and selected strong IRQ
 * handler, and publishes tick-based observation plus blocking millisecond and
 * optional microsecond delays.
 *
 * @section APP_TIME_H_BOUNDARY Dependency Boundary
 * This header exposes application timing vocabulary only. SysTick, Timer,
 * NVIC, RCC, and BSP headers remain private to `app_time.c`; their Drivers and
 * handlers never include or call this application service.
 */

// Header Guard
#ifndef APP_TIME_H_
#define APP_TIME_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "app_config.h"
#include "stm32f1xx_data_types.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Application timing service
 * @defgroup App_Time Application Time
 * @ingroup App_Template
 * @{
 */

// ==================================================================================================== //
// Application Timebase Types
// ==================================================================================================== //

/** @brief Application timebase source selector type @typedef app_time_source_t */
typedef uint8_t app_time_source_t;

#if (APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE)

// ==================================================================================================== //
// Application Timebase Initialization API
// ==================================================================================================== //

/**
 * @brief Initializes the selected interrupt-driven application timebase
 * @details
 * Validates that @p `timeSource` matches the source compiled by CMake and
 * derives the fixed 1 ms hardware period. A selected Timer is prescaled to a
 * 1 MHz counter clock and uses ARR=999. The operation configures that source
 * only after the RCC clock tree is stable, establishes a zero software/
 * hardware origin, enables interrupt delivery, and starts counter operation.
 *
 * The Timer path enables NVIC delivery before enabling the Timer update IRQ
 * source. The SysTick path uses its dedicated Cortex-M3 exception and therefore
 * performs no external NVIC transaction.
 * @param[in] timeSource Requested application timebase source
 * Accepted values:
 * - @ref `APP_TIME_SOURCE_SYSTICK`
 * - @ref `APP_TIME_SOURCE_TIM2`
 * - @ref `APP_TIME_SOURCE_TIM3`
 * - @ref `APP_TIME_SOURCE_TIM4`
 * - Must equal the build-selected @ref `APP_TIMEBASE_SOURCE`
 * @returns @ref driver_status_t "Application timebase initialization status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Hardware, interrupt delivery, and software tick state were initialized
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `timeSource` or the fixed derived configuration was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The stable source clock or required peripheral gate was unavailable
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The selected source was unexpectedly running during configuration
 * @pre RCC_Config72MHz() completed successfully so every selected Timer input
 * clock is an integer multiple of the fixed 1 MHz counter frequency
 * @pre This boot-time initializer is called once while the selected source and
 * its external NVIC line, when applicable, remain in their reset-quiescent state
 * @warning A selected general-purpose Timer is reserved exclusively by this
 * application time service for its complete initialized lifetime
 */
driver_status_t App_TimeInit(const app_time_source_t timeSource);

// ==================================================================================================== //
// Application Timebase Operation State APIs
// ==================================================================================================== //

// ---------------------------------------------------------------------------------------------------- //
// Application Timebase Operation State Pair
// ---------------------------------------------------------------------------------------------------- //

/**
 * @brief Returns the selected application timebase operation state
 * @returns @ref driver_status_t "Application timebase operation state"
 * @retval - @ref `DRIVER_STATUS_OFF`: Selected counter operation is disabled
 * @retval - @ref `DRIVER_STATUS_ON`: Selected counter operation is enabled
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The selected Timer clock gate is disabled or its IRQ service latched a failure
 */
driver_status_t App_TimeGetOperationState(void);

/**
 * @brief Sets the selected application timebase operation state
 * @param[in] operationState Requested counter operation state
 * Accepted values:
 * - @ref `DRIVER_STATUS_OFF`: Stop the configured timebase counter
 * - @ref `DRIVER_STATUS_ON`: Start the configured timebase counter
 * @returns @ref driver_status_t "Application timebase operation-state update status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Selected counter operation state was updated
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `operationState` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The selected Timer clock gate is disabled or its IRQ service latched a failure
 * @pre App_TimeInit() completed successfully before requesting @ref `DRIVER_STATUS_ON`
 */
driver_status_t App_TimeSetOperationState(const driver_status_t operationState);

#endif /* APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE */

// ==================================================================================================== //
// Application Time Observation APIs
// ==================================================================================================== //

/**
 * @brief Returns ticks accumulated since the application timebase origin
 * @returns Current 32-bit application tick count
 * @note One tick always represents one millisecond; a no-timebase build retains
 * the counter at zero
 * @note An arbitrary setter is absent because App_TimeInit() alone establishes
 * the monotonic counter's zero origin
 */
uint32_t App_TimeGetTick(void);

/**
 * @brief Calculates elapsed ticks from an earlier application tick
 * @param[in] startTick Earlier tick captured through App_TimeGetTick()
 * Accepted values:
 * - `0UL..0xFFFFFFFFUL`: Any previously captured 32-bit tick value
 * @returns Elapsed ticks modulo the 32-bit application tick range
 * @note Unsigned subtraction remains valid across one counter wrap
 */
uint32_t App_TimeGetElapsedTicks(const uint32_t startTick);

// ==================================================================================================== //
// Application Blocking Delay APIs
// ==================================================================================================== //

/**
 * @brief Provides a minimum blocking millisecond delay using the application timebase
 * @details
 * Uses @p `delayMs` directly as the required tick interval, captures one
 * origin, and sleeps with `WFI` between interrupts until that wrap-safe
 * interval elapses. No hardware is reconfigured by this operation.
 * @param[in] delayMs Requested delay in milliseconds
 * Accepted values:
 * - `1UL..0x7FFFFFFFUL`: Any wrap-safe millisecond interval
 * @returns @ref driver_status_t "Application millisecond-delay status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The requested minimum duration elapsed
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `delayMs` was zero or exceeded the wrap-safe tick range
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: No running timebase or usable thread-mode interrupt delivery was available
 * @pre App_TimeInit() completed successfully when a timebase source is selected
 * @warning This function blocks the calling thread while interrupt handlers continue to execute
 */
driver_status_t App_DelayMs(const uint32_t delayMs);

#if (APP_ENABLE_TIMER_US_DELAY == 1U)

/**
 * @brief Provides a minimum blocking microsecond delay through BSP-owned TIM4
 * @details Delegates the complete delay transaction to BSP_TimerDelayUs()
 * without exposing or reproducing TIM4 configuration in the application.
 * @param[in] delayUs Requested delay in microseconds
 * Accepted values:
 * - `1UL..0xFFFFFFFFUL`: BSP Timer delay duration
 * @returns @ref driver_status_t "Application microsecond-delay status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Every BSP Timer delay chunk completed
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `delayUs` was zero or BSP TIM4 state was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The BSP-owned TIM4 clock gate was disabled
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: A bounded TIM4 polling transaction timed out
 * @pre BSP_InitTimerUSDelay() completed successfully
 * @warning This operation polls and blocks the processor; it uses no Timer IRQ
 */
driver_status_t App_DelayUs(const uint32_t delayUs);

#endif /* APP_ENABLE_TIMER_US_DELAY */

#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_SYSTICK)

// ==================================================================================================== //
// Application Timebase Interrupt Handler
// ==================================================================================================== //

/** @brief Advances the application tick from the selected SysTick exception @returns Nothing */
void SysTick_Handler(void);

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM2)

/** @brief Services the application timebase through TIM2 update events @returns Nothing */
void TIM2_IRQHandler(void);

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM3)

/** @brief Services the application timebase through TIM3 update events @returns Nothing */
void TIM3_IRQHandler(void);

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM4)

/** @brief Services the application timebase through TIM4 update events @returns Nothing */
void TIM4_IRQHandler(void);

#endif /* APP_TIMEBASE_SOURCE */

/** @} */ // App_Time

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_TIME_H_ */
