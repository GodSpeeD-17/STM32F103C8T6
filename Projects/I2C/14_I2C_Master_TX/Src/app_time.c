/**
 * @file	app_time.c
 * @author	Shrey Shah
 * @brief	Implements application timebase and blocking-delay services
 * @version	v1.0
 * @date	26-08-2026
 *
 * @details
 * @section APP_TIME_C_HIERARCHY Hierarchy
 * - Position: Layer 2 - Application service implementation
 * - Called by: Layer 3 App_Init(), application behavior, and one selected IRQ
 * - Uses: Layer 1 RCC plus selected SysTick/Timer/NVIC and optional BSP Timer APIs
 *
 * @section APP_TIME_C_RESPONSIBILITY Responsibility
 * This module derives a fixed 1 ms hardware period from the stable clock tree.
 * The Timer path fixes its counter clock at 1 MHz and fixes ARR at 999. The
 * module initializes exactly one interrupt-driven timebase,
 * owns its monotonic tick and strong handler, converts
 * milliseconds into that configured tick domain, and delegates optional
 * microsecond polling delays to BSP-owned TIM4.
 *
 * @section APP_TIME_C_BOUNDARY Dependency Boundary
 * All family-specific source, clock-gate, and IRQ mappings remain private to
 * this project-local service. Timer derives its own kernel frequency through
 * RCC, BSP owns TIM4 delay configuration, and no shared layer calls upward.
 */

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "app_time.h"
#include "stm32f1xx.h"

#if (APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE)
#include "rcc.h"
#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_SYSTICK)
#include "systick.h"
#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM2) || (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM3) || (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM4)
#include "nvic.h"
#include "timer.h"
#endif /* APP_TIMEBASE_SOURCE */
#endif /* APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE */

#if (APP_ENABLE_TIMER_US_DELAY == 1U)
#include "bsp_timer.h"
#endif /* APP_ENABLE_TIMER_US_DELAY */

// ==================================================================================================== //
// Private Configuration
// ==================================================================================================== //

/** @brief Largest interval safe for one unsigned 32-bit elapsed-tick comparison @def APP_TIME_ELAPSED_TICK_MAX */
#define APP_TIME_ELAPSED_TICK_MAX			((uint32_t) 0x7FFFFFFFUL)
/** @brief Fixed 1 kHz interrupt rate providing one application tick per millisecond @def APP_TIME_TICK_RATE_HZ */
#define APP_TIME_TICK_RATE_HZ				((frequency_t) 1000UL)
/** @brief Fixed SysTick clock source using the undivided 72 MHz processor clock @def APP_TIME_SYSTICK_CLOCK_SOURCE */
#define APP_TIME_SYSTICK_CLOCK_SOURCE		(SYSTICK_CLOCK_SOURCE_PROCESSOR)
/** @brief Fixed SysTick reload encoding for a 1 ms period at 72 MHz HCLK @def APP_TIME_SYSTICK_RELOAD_VALUE */
#define APP_TIME_SYSTICK_RELOAD_VALUE		((systick_reload_value_t) ((RCC_HCLK_MAX_FREQ / APP_TIME_TICK_RATE_HZ) - 1UL))
/** @brief Fixed Timer counter frequency providing one count per microsecond @def APP_TIME_TIMER_COUNTER_HZ */
#define APP_TIME_TIMER_COUNTER_HZ			((frequency_t) 1000000UL)
/** @brief Fixed ARR encoding producing a 1 ms period from the 1 MHz Timer counter @def APP_TIME_TIMER_AUTO_RELOAD */
#define APP_TIME_TIMER_AUTO_RELOAD			((tim_auto_reload_t) ((APP_TIME_TIMER_COUNTER_HZ / APP_TIME_TICK_RATE_HZ) - 1UL))
/** @brief Timer IRQ path contains no latched service failure @def APP_TIME_TIMER_IRQ_FAULT_NONE */
#define APP_TIME_TIMER_IRQ_FAULT_NONE		((uint8_t) 0x00U)
/** @brief Timer IRQ path has latched a contained service failure @def APP_TIME_TIMER_IRQ_FAULT_LATCHED */
#define APP_TIME_TIMER_IRQ_FAULT_LATCHED	((uint8_t) 0x01U)

#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM2)

/** @brief Timer instance selected as the application timebase @def APP_TIMEBASE_TIMER_INSTANCE */
#define APP_TIMEBASE_TIMER_INSTANCE			(TIM2)
/** @brief RCC clock bus paired with the selected application Timer @def APP_TIMEBASE_TIMER_CLOCK_BUS */
#define APP_TIMEBASE_TIMER_CLOCK_BUS		(RCC_APB1_BUS)
/** @brief RCC clock gate paired with the selected application Timer @def APP_TIMEBASE_TIMER_CLOCK_MASK */
#define APP_TIMEBASE_TIMER_CLOCK_MASK		(RCC_APB1ENR_TIM2EN)
/** @brief NVIC line paired with the selected application Timer @def APP_TIMEBASE_TIMER_IRQn */
#define APP_TIMEBASE_TIMER_IRQn				(TIM2_IRQn)

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM3)

/** @brief Timer instance selected as the application timebase @def APP_TIMEBASE_TIMER_INSTANCE */
#define APP_TIMEBASE_TIMER_INSTANCE			(TIM3)
/** @brief RCC clock bus paired with the selected application Timer @def APP_TIMEBASE_TIMER_CLOCK_BUS */
#define APP_TIMEBASE_TIMER_CLOCK_BUS		(RCC_APB1_BUS)
/** @brief RCC clock gate paired with the selected application Timer @def APP_TIMEBASE_TIMER_CLOCK_MASK */
#define APP_TIMEBASE_TIMER_CLOCK_MASK		(RCC_APB1ENR_TIM3EN)
/** @brief NVIC line paired with the selected application Timer @def APP_TIMEBASE_TIMER_IRQn */
#define APP_TIMEBASE_TIMER_IRQn				(TIM3_IRQn)

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM4)

/** @brief Timer instance selected as the application timebase @def APP_TIMEBASE_TIMER_INSTANCE */
#define APP_TIMEBASE_TIMER_INSTANCE			(TIM4)
/** @brief RCC clock bus paired with the selected application Timer @def APP_TIMEBASE_TIMER_CLOCK_BUS */
#define APP_TIMEBASE_TIMER_CLOCK_BUS		(RCC_APB1_BUS)
/** @brief RCC clock gate paired with the selected application Timer @def APP_TIMEBASE_TIMER_CLOCK_MASK */
#define APP_TIMEBASE_TIMER_CLOCK_MASK		(RCC_APB1ENR_TIM4EN)
/** @brief NVIC line paired with the selected application Timer @def APP_TIMEBASE_TIMER_IRQn */
#define APP_TIMEBASE_TIMER_IRQn				(TIM4_IRQn)

#endif /* APP_TIMEBASE_SOURCE */

// ==================================================================================================== //
// Private Data
// ==================================================================================================== //

/** @brief Application-owned tick counter retained independently of the selected hardware source */
__STATIC volatile uint32_t appTick = 0UL;

// ==================================================================================================== //
// Local Tick Access Helpers
// ==================================================================================================== //

/**
 * @brief Returns the current application tick value
 * @returns Current 32-bit application tick
 */
__STATIC_FORCEINLINE uint32_t App_TimeGetTickValue(void)
{
	return appTick;
}

/**
 * @brief Updates the application tick value
 * @param[in] tickValue New 32-bit application tick
 * @returns Nothing
 */
__STATIC_FORCEINLINE void App_TimeSetTickValue(const uint32_t tickValue)
{
	appTick = tickValue;
}

/**
 * @brief Increments the application tick by one millisecond
 * @returns Nothing
 */
__STATIC_FORCEINLINE void App_TimeIncrementTickValue(void)
{
	appTick++;
}

#if (APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE)

#if (APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_SYSTICK)
/** @brief Latched Timer IRQ-service failure preventing an incomplete delivery-path restart */
__STATIC volatile uint8_t appTimerIRQFault = APP_TIME_TIMER_IRQ_FAULT_NONE;
#endif /* APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_SYSTICK */

// ==================================================================================================== //
// Local Timebase Helpers
// ==================================================================================================== //

/**
 * @brief Validates the explicit source request
 * @param[in] timeSource Requested application timebase source
 * @returns @ref driver_status_t "Application timebase-request validation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Source matches the compiled timebase contract
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: Source differed from @ref `APP_TIMEBASE_SOURCE`
 */
__STATIC_FORCEINLINE driver_status_t App_TimeValidateRequest(const app_time_source_t timeSource)
{
	//! The build links one hardware path only, so reject requests that would require an absent Driver.
	if (timeSource != ((app_time_source_t) APP_TIMEBASE_SOURCE))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	return DRIVER_STATUS_SUCCESS;
}

#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_SYSTICK)

/**
 * @brief Initializes the SysTick hardware path for the fixed 1 ms application tick
 * @returns @ref driver_status_t "SysTick-backed application-timebase status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: SysTick and software state were initialized
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: The fixed SysTick configuration was rejected
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: SysTick remained active during configuration
 */
__STATIC driver_status_t App_TimeInitSysTick(void)
{
	//! RCC_Config72MHz() fixes HCLK, so apply the precomputed 71999 reload directly.
	ASSERT_DRIVER_STATUS(SysTick_DeConfig());
	ASSERT_DRIVER_STATUS(SysTick_SetConfig(APP_TIME_SYSTICK_CLOCK_SOURCE, APP_TIME_SYSTICK_RELOAD_VALUE));

	//! Publish the software unit and both origins before the first exception can be delivered.
	App_TimeSetTickValue(0UL);
	SysTick_ResetCurrentValue();
	ASSERT_DRIVER_STATUS(SysTick_SetIRQSourceState(DRIVER_STATUS_ON));
	return SysTick_SetOperationState(DRIVER_STATUS_ON);
}

#else

/**
 * @brief Initializes the selected general-purpose Timer for the fixed 1 ms application tick
 * @returns @ref driver_status_t "Timer-backed application-timebase status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Timer, NVIC, and software state were initialized
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: A fixed Timer mapping was rejected as null
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: The requested period was not exactly representable by the fixed 1 MHz counter and ARR
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Stable clock frequency or Timer gate state was unavailable
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The Timer was unexpectedly running during configuration
 */
__STATIC driver_status_t App_TimeInitTimer(void)
{
	// Local Variables
	frequency_t inputClockHz = 0UL;
	uint32_t prescalerDivider = 0UL;
	tim_config_t config = {0};

	//! Determine every divider before enabling the Timer peripheral gate.
	ASSERT_DRIVER_STATUS(TIM_GetInputClockFrequency(APP_TIMEBASE_TIMER_INSTANCE, &inputClockHz));
	prescalerDivider = inputClockHz / APP_TIME_TIMER_COUNTER_HZ;

	//! PSC fixes one Timer count at 1 us; the fixed ARR then produces one update every millisecond.
	config.timebase.prescaler = (tim_prescaler_t) (prescalerDivider - 1UL);
	config.timebase.auto_reload = APP_TIME_TIMER_AUTO_RELOAD;
	config.timebase.initial_count = TIMx_DEFAULT_CNT;
	config.counter.digital_filter_clock_division = TIMx_DIGITAL_FILTER_CLOCK_DIV_1;
	config.counter.alignment = TIMx_MODE_NORMAL;
	config.counter.direction = TIMx_DIR_COUNT_UP;
	config.counter.one_pulse = TIMx_OPM_DISABLE;
	config.counter.auto_reload_preload = TIMx_ARPE_ENABLE;
	config.counter.update_source = TIMx_UPDATE_SOURCE_OVERFLOW_UNDERFLOW_ONLY;

	//! Make the selected register bank accessible only after every clock-independent operation succeeds.
	ASSERT_DRIVER_STATUS(RCC_SetPeripheralClockState(APP_TIMEBASE_TIMER_CLOCK_BUS, APP_TIMEBASE_TIMER_CLOCK_MASK, DRIVER_STATUS_ON));
	ASSERT_DRIVER_STATUS(TIM_DeConfig(APP_TIMEBASE_TIMER_INSTANCE));
	ASSERT_DRIVER_STATUS(TIM_Config(APP_TIMEBASE_TIMER_INSTANCE, &config));

	//! Establish the software origin and clear both peripheral and NVIC stale state before opening delivery.
	App_TimeSetTickValue(0UL);
	appTimerIRQFault = APP_TIME_TIMER_IRQ_FAULT_NONE;

	//! Clear pending IRQ
	ASSERT_DRIVER_STATUS(TIM_AckIRQEvents(APP_TIMEBASE_TIMER_INSTANCE, TIMx_IRQ_EVENT_UPDATE));
	ASSERT_DRIVER_STATUS(NVIC_ClearPendingIRQ(APP_TIMEBASE_TIMER_IRQn));
	//! Enable NVIC IRQ
	ASSERT_DRIVER_STATUS(NVIC_EnableIRQ(APP_TIMEBASE_TIMER_IRQn));
	//! Set IRQ Source
	ASSERT_DRIVER_STATUS(TIM_SetIRQSources(APP_TIMEBASE_TIMER_INSTANCE, TIMx_IRQ_SOURCE_UPDATE, DRIVER_STATUS_ON));
	return TIM_SetOperationState(APP_TIMEBASE_TIMER_INSTANCE, DRIVER_STATUS_ON);
}

/**
 * @brief Contains a Timer IRQ-service failure without blocking in Handler mode
 * @returns Nothing
 * @note Cleanup calls are best effort because this path is entered only after
 * a Timer Driver operation has already failed
 */
__STATIC_FORCEINLINE void App_TimeContainTimerIRQFault(void)
{
	//! Latch the incomplete delivery path before masking its hardware sources.
	appTimerIRQFault = APP_TIME_TIMER_IRQ_FAULT_LATCHED;
	(void) NVIC_DisableIRQ(APP_TIMEBASE_TIMER_IRQn);
	(void) TIM_SetIRQSources(APP_TIMEBASE_TIMER_INSTANCE, TIMx_IRQ_SOURCE_UPDATE, DRIVER_STATUS_OFF);
	(void) TIM_SetOperationState(APP_TIMEBASE_TIMER_INSTANCE, DRIVER_STATUS_OFF);
	(void) NVIC_ClearPendingIRQ(APP_TIMEBASE_TIMER_IRQn);
}

/**
 * @brief Services at most one observed update event for the selected Timer
 * @returns Nothing
 * @note The bounded handler performs one status read and at most one
 * acknowledgement before returning from Handler mode
 */
__STATIC_FORCEINLINE void App_TimeHandleTimerIRQ(void)
{
	// Local Variable
	tim_event_flag_t irqEvents = TIMx_IRQ_EVENT_NONE;

	//! A failed status read leaves event ownership unknown, so contain the source and return.
	if (TIM_GetIRQEvents(APP_TIMEBASE_TIMER_INSTANCE, &irqEvents) != DRIVER_STATUS_SUCCESS)
	{
		App_TimeContainTimerIRQFault();
		return;
	}
	if ((irqEvents & TIMx_IRQ_EVENT_UPDATE) == TIMx_IRQ_EVENT_NONE)
	{
		return;
	}

	//! Acknowledge first so a later update can request a fresh exception, then publish exactly one tick.
	if (TIM_AckIRQEvents(APP_TIMEBASE_TIMER_INSTANCE, TIMx_IRQ_EVENT_UPDATE) != DRIVER_STATUS_SUCCESS)
	{
		App_TimeContainTimerIRQFault();
		return;
	}
	App_TimeIncrementTickValue();
}

#endif /* APP_TIMEBASE_SOURCE */

// ==================================================================================================== //
// Application Timebase Public APIs
// ==================================================================================================== //

driver_status_t App_TimeInit(const app_time_source_t timeSource)
{
	//! Reject a request for a hardware path that this firmware deliberately did not link.
	ASSERT_DRIVER_STATUS(App_TimeValidateRequest(timeSource));

#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_SYSTICK)
	//! SysTick uses the stable undivided processor clock and its dedicated Core exception.
	return App_TimeInitSysTick();
#else
	//! General-purpose Timer sources share the fixed 1 MHz counter and external-NVIC initialization path.
	return App_TimeInitTimer();
#endif /* APP_TIMEBASE_SOURCE */
}

driver_status_t App_TimeGetOperationState(void)
{
#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_SYSTICK)
	//! Preserve the SysTick Driver's exact binary state contract.
	return SysTick_GetOperationState();
#else
	//! Report a contained ISR failure before querying the stopped Timer.
	if (appTimerIRQFault == APP_TIME_TIMER_IRQ_FAULT_LATCHED)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}
	//! Preserve the selected Timer Driver's exact binary state or clock-state error.
	return TIM_GetOperationState(APP_TIMEBASE_TIMER_INSTANCE);
#endif /* APP_TIMEBASE_SOURCE */
}

driver_status_t App_TimeSetOperationState(const driver_status_t operationState)
{
#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_SYSTICK)
	//! Delegate validation and the Core counter transition to the SysTick Driver.
	return SysTick_SetOperationState(operationState);
#else
	//! A contained IRQ-service failure requires reinitialization rather than a blind counter restart.
	if (appTimerIRQFault == APP_TIME_TIMER_IRQ_FAULT_LATCHED)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}
	//! Delegate validation and the peripheral counter transition to the Timer Driver.
	return TIM_SetOperationState(APP_TIMEBASE_TIMER_INSTANCE, operationState);
#endif /* APP_TIMEBASE_SOURCE */
}

#endif /* APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE */

// ==================================================================================================== //
// Application Time Observation APIs
// ==================================================================================================== //

uint32_t App_TimeGetTick(void)
{
	//! An aligned 32-bit read is atomic on Cortex-M3, so no interrupt masking is required.
	return App_TimeGetTickValue();
}

uint32_t App_TimeGetElapsedTicks(const uint32_t startTick)
{
	//! Unsigned subtraction preserves elapsed duration across one complete 32-bit counter wrap.
	return (App_TimeGetTickValue() - startTick);
}

// ==================================================================================================== //
// Application Blocking Delay APIs
// ==================================================================================================== //

driver_status_t App_DelayMs(const uint32_t delayMs)
{
#if (APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE)
	// Local Variable
	uint32_t startTick = 0UL;

	//! One tick is exactly 1 ms, so validate the requested duration directly in the wrap-safe domain.
	if ((delayMs == 0UL) || (delayMs > APP_TIME_ELAPSED_TICK_MAX))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! WFI-based waiting requires a running source and deliverable interrupts from thread mode.
	if ((App_TimeGetOperationState() != DRIVER_STATUS_ON) || (__get_IPSR() != 0UL) || (__get_PRIMASK() != 0UL))
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	startTick = App_TimeGetTick();
	while (App_TimeGetElapsedTicks(startTick) < delayMs)
	{
		//! Sleep between interrupts instead of spinning for the complete blocking interval.
		__WFI();
	}

	return DRIVER_STATUS_SUCCESS;
#else
	//! A timebase-free build retains the API but cannot satisfy a tick-backed delay request.
	(void) delayMs;
	return DRIVER_STATUS_ERROR_STATE;
#endif /* APP_TIMEBASE_SOURCE */
}

#if (APP_ENABLE_TIMER_US_DELAY == 1U)

driver_status_t App_DelayUs(const uint32_t delayUs)
{
	//! BSP owns TIM4 configuration and chunking, so the application forwards only the requested duration.
	return BSP_TimerDelayUs(delayUs);
}

#endif /* APP_ENABLE_TIMER_US_DELAY */

// ==================================================================================================== //
// Application Timebase Interrupt Handler
// ==================================================================================================== //

#if (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_SYSTICK)

void SysTick_Handler(void)
{
	//! Keep the Core exception bounded to one monotonic publication.
	App_TimeIncrementTickValue();
}

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM2)

void TIM2_IRQHandler(void)
{
	//! Keep the ABI handler thin while the shared private service owns event ordering and failure policy.
	App_TimeHandleTimerIRQ();
}

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM3)

void TIM3_IRQHandler(void)
{
	//! Keep the ABI handler thin while the shared private service owns event ordering and failure policy.
	App_TimeHandleTimerIRQ();
}

#elif (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM4)

void TIM4_IRQHandler(void)
{
	//! Keep the ABI handler thin while the shared private service owns event ordering and failure policy.
	App_TimeHandleTimerIRQ();
}

#endif /* APP_TIMEBASE_SOURCE */
