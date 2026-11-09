/**
 * @file	app_init.c
 * @author	Shrey Shah
 * @brief	Orchestrates application-selected hardware services
 * @version	v1.0
 * @date	26-08-2026
 *
 * @details
 * @section APP_INIT_C_HIERARCHY Hierarchy
 * - Position: Layer 3 - Application orchestration implementation
 * - Called by: Layer 4 Reset_Handler()
 * - Uses: Layer 2 `app_time` and Layer 1 RCC/selected BSP capabilities
 *
 * @section APP_INIT_C_RESPONSIBILITY Responsibility
 * App_Init() establishes the system clock, starts the selected application
 * timebase, requests the optional BSP-owned TIM4 delay service, and configures
 * the optional on-board LED. Debug policy additionally requests the complete
 * board USART initialization transaction. The first initialization failure
 * propagates unchanged.
 *
 * @section APP_INIT_C_BOUNDARY Dependency Boundary
 * This module selects and orders services but does not access peripheral
 * registers directly. No service depends upward on `app_init`.
 */

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "app_init.h"
#include "app_config.h"
#include "app_time.h"
#include "rcc.h"

#if (APP_ENABLE_TIMER_US_DELAY == 1U)
#include "bsp_timer.h"
#endif /* APP_ENABLE_TIMER_US_DELAY */

#if (APP_ENABLE_ONBOARD_LED == 1U)
#include "bsp_gpio.h"
#endif /* APP_ENABLE_ONBOARD_LED */

#if (APP_ENABLE_DEBUG == 1U)
#include "bsp_usart.h"
#endif /* APP_ENABLE_DEBUG */

// ==================================================================================================== //
// Public API
// ==================================================================================================== //

driver_status_t App_Init(void)
{
	//! Establish the application clock tree before initializing clock-dependent services.
	ASSERT_DRIVER_STATUS(RCC_Config72MHz());

#if (APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE)
	//! Initialize the selected fixed-1-ms interrupt source only after every Core and bus clock is final.
	ASSERT_DRIVER_STATUS(App_TimeInit((app_time_source_t) APP_TIMEBASE_SOURCE));
#endif /* APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE */

#if (APP_ENABLE_TIMER_US_DELAY == 1U)
	//! Request the complete board-locked TIM4 polling-delay transaction without reproducing its mapping in the application.
	ASSERT_DRIVER_STATUS(BSP_InitTimerUSDelay());
#endif /* APP_ENABLE_TIMER_US_DELAY */

#if (APP_ENABLE_ONBOARD_LED == 1U)
	//! Request the complete BSP-owned RCC/GPIO transaction; successful initialization leaves the active-low LED off.
	ASSERT_DRIVER_STATUS(BSP_InitOBLED());
#endif /* APP_ENABLE_ONBOARD_LED */

#if (APP_ENABLE_DEBUG == 1U)
	//! Debug implicitly acquires the complete BSP USART transport before application behavior begins.
	ASSERT_DRIVER_STATUS(BSP_InitUSART());
#endif /* APP_ENABLE_DEBUG */

	return DRIVER_STATUS_SUCCESS;
}
