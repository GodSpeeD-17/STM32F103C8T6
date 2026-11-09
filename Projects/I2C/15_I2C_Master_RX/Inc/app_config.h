/**
 * @file	app_config.h
 * @author	Shrey Shah
 * @brief	Defines the compile-time policy shared by application services
 * @version	v1.0
 * @date	26-08-2026
 *
 * @details
 * @section APP_CONFIG_H_HIERARCHY Application Hierarchy
 * Dependencies flow downward through four runtime layers:
 * - Layer 4 - Entry: `app_startup` performs reset handling
 * - Layer 3 - Orchestration: `app_init` initializes services and `main` runs
 *   application behavior
 * - Layer 2 - Service: `app_time` owns timebase and blocking-delay operations
 * - Layer 1 - Hardware access: shared SysTick, RCC, Timer, GPIO, USART, and
 *   selected BSP capabilities
 * Layer 1 is always closest to hardware. Increasing layer numbers represent
 * progressively more software-only behavior, policy, and orchestration.
 *
 * @section APP_CONFIG_H_POSITION Position of This File
 * `app_config.h` is cross-cutting compile-time policy, not a runtime layer.
 * Layers 2 and 3 read it; it does not call or initialize any layer. This
 * Template copy is the canonical baseline: repository-wide policy changes are
 * verified here before propagation to existing projects.
 *
 * @section APP_CONFIG_H_USERS Direct Users
 * `app_init.c`, `main.c`, and `app_time.h` include this file.
 *
 * @section APP_CONFIG_H_BOUNDARY Dependency Boundary
 * This file includes no project, Core, or Driver header. It selects features
 * and publishes constants only. Layer 3 orders enabled services; Layer 2
 * services request their Layer 1 hardware transactions. Board mappings remain
 * owned by their capability headers and are never republished here.
 */

// Header Guard
#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

/**
 * @brief Application-owned Template services
 * @defgroup App_Template Application Template
 */

/**
 * @brief Compile-time application feature and timing policy
 * @defgroup App_Config Application Configuration
 * @ingroup App_Template
 * @details
 * These macros select timing, board-capability, and diagnostic behavior. The
 * timebase source selects the hardware that generates the service's fixed
 * 1 ms tick; the independently selected TIM4 microsecond-delay service remains
 * BSP-owned.
 * Defining a macro publishes policy only, while Layer 3 orders initialization
 * and the Layer 2 time service owns its selected Layer 1 transactions.
 * @{
 */

// ==================================================================================================== //
// Application Timebase Configuration
// ==================================================================================================== //

/** @brief Disables the interrupt-driven application timebase @def APP_TIME_SOURCE_NONE */
#define APP_TIME_SOURCE_NONE			(0U)
/** @brief Selects the Cortex-M3 SysTick application timebase @def APP_TIME_SOURCE_SYSTICK */
#define APP_TIME_SOURCE_SYSTICK			(1U)
/** @brief Selects TIM2 as the application timebase @def APP_TIME_SOURCE_TIM2 */
#define APP_TIME_SOURCE_TIM2			(2U)
/** @brief Selects TIM3 as the application timebase @def APP_TIME_SOURCE_TIM3 */
#define APP_TIME_SOURCE_TIM3			(3U)
/** @brief Selects TIM4 as the application timebase @def APP_TIME_SOURCE_TIM4 */
#define APP_TIME_SOURCE_TIM4			(4U)

#ifndef APP_TIMEBASE_SOURCE

/**
 * @brief Selects the interrupt-driven application timebase source
 * @def APP_TIMEBASE_SOURCE
 * @details
 * CMake normally publishes this token from its `APP_TIMEBASE_SOURCE` cache
 * selection and resolves only the matching Driver modules. App_Init()
 * passes it explicitly to App_TimeInit() after the RCC clock tree is stable.
 * This fallback selects SysTick for direct builds that do not inject policy.
 * Accepted values:
 * - @ref `APP_TIME_SOURCE_NONE`: Leave every timebase source unconfigured
 * - @ref `APP_TIME_SOURCE_SYSTICK`: Use the undivided processor clock and SysTick exception
 * - @ref `APP_TIME_SOURCE_TIM2`: Use the TIM2 update event and external IRQ
 * - @ref `APP_TIME_SOURCE_TIM3`: Use the TIM3 update event and external IRQ
 * - @ref `APP_TIME_SOURCE_TIM4`: Use the TIM4 update event and external IRQ
 * @note Selecting @ref `APP_TIME_SOURCE_NONE` causes App_DelayMs() to report
 * @ref `DRIVER_STATUS_ERROR_STATE`
 * @note A selected TIM2, TIM3, or TIM4 source is reserved exclusively by the
 * project-local application time service for its complete initialized lifetime
 * @warning TIM4 cannot simultaneously serve as the application timebase and
 * the BSP-owned microsecond-delay Timer
 */
#define APP_TIMEBASE_SOURCE				APP_TIME_SOURCE_SYSTICK

#endif /* APP_TIMEBASE_SOURCE */

#if \
	(APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_NONE) && \
	(APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_SYSTICK) && \
	(APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_TIM2) && \
	(APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_TIM3) && \
	(APP_TIMEBASE_SOURCE != APP_TIME_SOURCE_TIM4)
#error "APP_TIMEBASE_SOURCE must select NONE, SYSTICK, TIM2, TIM3, or TIM4."
#endif /* APP_TIMEBASE_SOURCE validity */

// ==================================================================================================== //
// Optional Timer Delay Configuration
// ==================================================================================================== //

#ifndef APP_ENABLE_TIMER_US_DELAY

/**
 * @brief Selects the optional dedicated-Timer microsecond delay capability
 * @def APP_ENABLE_TIMER_US_DELAY
 * @details
 * CMake normally defines this macro from `APP_ENABLE_TIMER_US_DELAY`. When it
 * is `1U`, the build adds the `BSP_TIMER` capability, app_time exposes
 * App_DelayUs(), and App_Init() requests the complete BSP-owned TIM4
 * initialization transaction. This fallback supports direct builds that do
 * not inject the option.
 * Accepted values:
 * - `0U`: Exclude the microsecond-delay API and leave BSP TIM4 unrequested
 * - `1U`: Compile, initialize, and expose the BSP TIM4 polling-delay service
 * @note This service polls Timer completion; it does not enable Timer IRQ
 * sources or NVIC delivery
 */
#define APP_ENABLE_TIMER_US_DELAY		(0U)

#endif /* APP_ENABLE_TIMER_US_DELAY */

#if (APP_ENABLE_TIMER_US_DELAY == 1U) && (APP_TIMEBASE_SOURCE == APP_TIME_SOURCE_TIM4)
#error "APP_TIME_SOURCE_TIM4 conflicts with the BSP-owned TIM4 microsecond-delay capability."
#endif /* APP_ENABLE_TIMER_US_DELAY && APP_TIME_SOURCE_TIM4 */

// ==================================================================================================== //
// Optional On-Board LED Configuration
// ==================================================================================================== //

#ifndef APP_ENABLE_ONBOARD_LED

/**
 * @brief Selects the optional BSP on-board LED initialization and fault indication
 * @def APP_ENABLE_ONBOARD_LED
 * @details
 * CMake normally defines this macro from `APP_ENABLE_ONBOARD_LED`. When it is
 * `1U`, App_Init() configures the board on-board LED GPIO and forces it to a
 * deterministic off state before application behavior begins.
 * This fallback definition supports builds that do not inject the CMake option.
 * Accepted values:
 * - `0U`: Leave the on-board LED GPIO untouched
 * - `1U`: Initialize the on-board LED GPIO and force it off; Template default
 * @note Application code owns every BSP_OB_LED_Set() / BSP_OB_LED_Reset() /
 * BSP_OB_LED_Toggle() call; this switch does not make BSP own application
 * control flow
 * @note CMake requests the LED-only `BSP_GPIO` capability only when this macro
 * is `1U`
 */
#define APP_ENABLE_ONBOARD_LED			(1U)

#endif /* APP_ENABLE_ONBOARD_LED */

// ==================================================================================================== //
// Optional Float and Debug Configuration
// ==================================================================================================== //

#ifndef APP_ENABLE_FLOAT

/**
 * @brief Selects application code that requires float or double arithmetic
 * @def APP_ENABLE_FLOAT
 * @details
 * CMake normally defines this policy from `APP_ENABLE_FLOAT`. It controls
 * whether the application may compile float-dependent behavior and whether
 * CMake links newlib-nano's `%f` formatter.
 * Accepted values:
 * - `0U`: Exclude float-dependent application behavior
 * - `1U`: Permit float-dependent application behavior
 * @note This switch alone does not enable diagnostics or call any BSP API
 */
#define APP_ENABLE_FLOAT				(0U)

#endif /* APP_ENABLE_FLOAT */

#ifndef APP_ENABLE_DEBUG

/**
 * @brief Selects optional application debug behavior
 * @def APP_ENABLE_DEBUG
 * @details
 * CMake normally defines this policy from `APP_ENABLE_DEBUG`. When enabled,
 * App_Init() acquires the complete BSP USART transport before `main()` starts.
 * When disabled, that implicit boot-only path is not compiled and application
 * code remains responsible for any USART configuration it requires.
 * Accepted values:
 * - `0U`: Exclude optional application debug behavior
 * - `1U`: Compile optional application debug behavior
 * @note This demonstration convenience controls implicit BSP USART acquisition
 * only; it does not restrict application-owned USART Driver configuration
 */
#define APP_ENABLE_DEBUG				(0U)

#endif /* APP_ENABLE_DEBUG */

/** @} */ // App_Config

#endif /* APP_CONFIG_H_ */
