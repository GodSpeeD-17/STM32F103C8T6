/**
 * @file	rcc.h
 * @author	Shrey Shah
 * @brief	Declares RCC operations and observable clock state
 * @version	v5.0
 * @date	23-08-2026
 *
 * @details
 * @section RCC_H_HIERARCHY Hierarchy
 * - Position: Layer 3 RCC Driver orchestration
 * - Used by: Applications and peer peripheral Drivers
 * - Uses: RCC-owned types, configuration vocabulary, and query constants
 *
 * @section RCC_H_RESPONSIBILITY Responsibility
 * This interface validates configuration requests, sequences clock-source and
 * Flash transitions, manages peripheral gates and reset pulses, and publishes
 * derived clock observations. Configuration construction is intentionally
 * separated into `rcc_config.h`.
 *
 * @section RCC_H_BOUNDARY Dependency Boundary
 * Applications include this header, never Codec or LL headers. The Driver may
 * call Codec and LL, while neither lower layer includes or calls the Driver.
 * This header exposes no raw RCC register-field values.
 *
 * @see @ref RCC_Peripheral_Guide "STM32F103C8T6 RCC Architecture"
 */

// Header Guard
#ifndef RCC_H_
#define RCC_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "rcc_data_types.h"
#include "rcc_config.h"
#include "rcc_defines.h"

// C++ Compatibility
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup RCC_03_Driver
 * @{
 */

// ==================================================================================================== //
// RCC Clock Gate and Reset APIs
// ==================================================================================================== //

/**
 * @brief RCC Clock Gate and Reset APIs
 * @defgroup RCC_03_Driver_07_ClockReset RCC Driver Clock Enable and Reset APIs
 * @ingroup RCC_03_Driver
 * @{
 */

/**
 * @brief Returns the combined state of selected peripheral clock gates
 * @details Reads the clock-enable register selected by @p bus and returns ON
 * only when every bit selected by @p clockMask is enabled.
 * @param[in] bus Physical RCC bus containing the requested clock gates
 * Accepted values:
 * - @ref `RCC_AHB_BUS`
 * - @ref `RCC_APB1_BUS`
 * - @ref `RCC_APB2_BUS`
 * @param[in] clockMask Non-zero clock-enable mask belonging to @p bus
 * Accepted values:
 * - `1UL..0xFFFFFFFFUL`: Any non-zero raw mask valid for the selected clock-enable register
 * @returns @ref driver_status_t "Peripheral clock-gate state"
 * @retval - @ref `DRIVER_STATUS_OFF`: One or more requested peripheral clock gates are disabled
 * @retval - @ref `DRIVER_STATUS_ON`: Every requested peripheral clock gate is enabled
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p bus or @p clockMask is invalid
 * @warning Pair @p clockMask with its owning bus; raw mask values are not self-identifying
 */
driver_status_t RCC_GetPeripheralClockState(const rcc_bus_t bus, const reg clockMask);

/**
 * @brief Enables or disables selected peripheral clock gates
 * @details Dispatches the requested state to the clock-enable register selected
 * by @p bus without changing gates outside @p clockMask.
 * @param[in] bus Physical RCC bus containing the requested clock gates
 * Accepted values:
 * - @ref `RCC_AHB_BUS`
 * - @ref `RCC_APB1_BUS`
 * - @ref `RCC_APB2_BUS`
 * @param[in] clockMask Non-zero clock-enable mask belonging to @p bus
 * Accepted values:
 * - `1UL..0xFFFFFFFFUL`: Any non-zero raw mask valid for the selected clock-enable register
 * @param[in] clockState Requested peripheral clock-gate state
 * Accepted values:
 * - @ref `DRIVER_STATUS_OFF`: Disable every selected peripheral clock gate
 * - @ref `DRIVER_STATUS_ON`: Enable every selected peripheral clock gate
 * @returns @ref driver_status_t "Peripheral clock-gate operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Requested peripheral clock gates were updated
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p bus, @p clockMask, or @p clockState is invalid
 * @warning Pair @p clockMask with its owning bus; raw mask values are not self-identifying
 */
driver_status_t RCC_SetPeripheralClockState
(
	const rcc_bus_t			bus,
	const reg				clockMask,
	const driver_status_t	clockState
);

/**
 * @brief Asserts and releases selected peripheral reset bits
 * @details Dispatches one complete reset pulse to the APB reset register
 * selected by @p bus. AHB has no supported peripheral-reset register.
 * @param[in] bus Physical RCC bus containing the requested reset bits
 * Accepted values:
 * - @ref `RCC_APB1_BUS`
 * - @ref `RCC_APB2_BUS`
 * @param[in] resetMask Non-zero peripheral-reset mask belonging to @p bus
 * Accepted values:
 * - `1UL..0xFFFFFFFFUL`: Any non-zero raw mask valid for the selected reset register
 * @returns @ref driver_status_t "Peripheral reset-pulse operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Requested peripherals were reset-pulsed
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p bus or @p resetMask is invalid
 * @warning Pair @p resetMask with its owning bus; raw mask values are not self-identifying
 */
driver_status_t RCC_PulsePeripheralReset(const rcc_bus_t bus, const reg resetMask);

/** @} */ // RCC_03_Driver_07_ClockReset

// ==================================================================================================== //
// RCC Frequency APIs
// ==================================================================================================== //

/**
 * @addtogroup RCC_03_Driver_02_Frequency
 * @{
 */

/**
 * @brief Returns the current SYSCLK frequency before AHB division
 * @returns @ref frequency_t "Core clock frequency in hertz"
 * @retval - @ref `RCC_FREQ_ZERO`: The current frequency snapshot could not be derived
 * @retval - `1UL..RCC_SYSCLK_MAX_FREQ`: Current SYSCLK frequency
 */
frequency_t RCC_GetCoreClockFrequency(void);

/**
 * @brief Returns the current derived frequency of a requested physical bus
 * @param[in] bus Target physical bus selector
 * Accepted values:
 * - @ref `RCC_AHB_BUS`: AHB bus
 * - @ref `RCC_APB1_BUS`: APB1 bus
 * - @ref `RCC_APB2_BUS`: APB2 bus
 * @returns @ref frequency_t "Bus clock frequency in hertz"
 * @retval - @ref `RCC_FREQ_ZERO`: The clock is inactive, the selector is invalid, or snapshot refresh failed
 * @retval - `1UL..RCC_SYSCLK_MAX_FREQ`: Current selected clock-domain frequency
 */
frequency_t RCC_GetBusFrequency(const rcc_bus_t bus);

/**
 * @brief Returns one coherent cached or hardware-derived clock snapshot
 * @param[out] pClockFrequencies Destination for the complete clock snapshot
 * Expected values:
 * - Non-`NULL`: Snapshot is published only after a successful refresh when required
 * @returns @ref driver_status_t "Clock-frequency snapshot operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The clock frequency snapshot was returned successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pClockFrequencies was a null pointer
 * @retval - @ref `DRIVER_STATUS_ERROR`: Hardware status could not be read while building the snapshot
 */
driver_status_t RCC_GetClockFrequencies(rcc_clock_frequencies_t* const pClockFrequencies);

/** @} */ // RCC_03_Driver_02_Frequency

// ==================================================================================================== //
// RCC Flash APIs
// ==================================================================================================== //

/**
 * @addtogroup RCC_03_Driver_03_Flash
 * @{
 */

/**
 * @brief Applies one validated Flash latency and prefetch domain
 * @details Validates both selectors before the first `FLASH_ACR` mutation.
 * @param[in] pFlashConfig Flash configuration to apply
 * Expected values:
 * - Non-`NULL`: Complete Flash latency and prefetch policy
 * @returns @ref driver_status_t "Flash-configuration operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Flash latency and prefetch configuration were applied
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pFlashConfig was a null pointer
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: Flash configuration fields were invalid
 */
driver_status_t RCC_ConfigFlash(const rcc_flash_config_t* const pFlashConfig);

/** @} */ // RCC_03_Driver_03_Flash

// ==================================================================================================== //
// RCC System Clock APIs
// ==================================================================================================== //

/**
 * @addtogroup RCC_03_Driver_04_SystemClock
 * @{
 */

/**
 * @brief Returns the active hardware-confirmed SYSCLK source
 * @returns Current system-clock source as @ref rcc_system_clock_t
 * @retval - @ref `RCC_SYS_CLK_HSI`: HSI is active or the raw status was unrecognized
 * @retval - @ref `RCC_SYS_CLK_HSE`: HSE is active
 * @retval - @ref `RCC_SYS_CLK_PLL`: PLL is active
 */
rcc_system_clock_t RCC_GetSystemClockSource(void);

/**
 * @brief Selects SYSCLK and validates the transition with a timeout
 * @details
 * HSI and HSE paths enable the requested oscillator and wait for readiness.
 * The PLL path requires an already-configured, ready PLL. Every path programs
 * `CFGR.SW`, waits for `CFGR.SWS` confirmation, and refreshes the cached
 * frequency snapshot.
 * @param[in] source Requested system clock source
 * Accepted values:
 * - @ref `RCC_SYS_CLK_HSI`: Enables HSI and switches SYSCLK to HSI
 * - @ref `RCC_SYS_CLK_HSE`: Enables HSE and switches SYSCLK to HSE
 * - @ref `RCC_SYS_CLK_PLL`: Switches SYSCLK to the already-configured PLL
 * @returns @ref driver_status_t "System clock source selection status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The requested source became ready and SYSCLK switched successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p source is not a supported selector
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: @ref `RCC_SYS_CLK_PLL` was requested while the PLL was not ready
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: Source readiness or SYSCLK confirmation timed out
 * @retval - @ref `DRIVER_STATUS_ERROR`: Hardware status could not be read during the transition
 * @note Selecting @ref `RCC_SYS_CLK_PLL` requires the PLL to already be configured and ready
 */
driver_status_t RCC_SetSystemClockSource(const rcc_system_clock_t source);

/**
 * @brief Returns the configured PLL input source
 * @returns Current PLL source as @ref rcc_pll_src_t
 * @retval - @ref `RCC_PLL_SRC_HSI`: HSI divided by two is selected
 * @retval - @ref `RCC_PLL_SRC_HSE`: HSE is selected
 */
rcc_pll_src_t RCC_GetPLLSource(void);

/**
 * @brief Returns the effective PLL input prescaler selection
 * @returns Current PLL source prescaler as @ref rcc_pll_src_psc_t
 * @retval - @ref `RCC_PLL_SRC_HSI_DIV_2`: Fixed HSI divide-by-two path is selected
 * @retval - @ref `RCC_PLL_SRC_HSE_DIV_1`: Undivided HSE path is selected
 * @retval - @ref `RCC_PLL_SRC_HSE_DIV_2`: HSE divide-by-two path is selected
 */
rcc_pll_src_psc_t RCC_GetPLLSourcePrescaler(void);

/**
 * @brief Returns the configured PLL multiplication factor
 * @returns Current PLL multiplication factor as @ref rcc_pll_mul_t
 * @retval - @ref `RCC_PLL_MUL_2`: Multiplication by 2 is configured or the raw field was unrecognized
 * @retval - @ref `RCC_PLL_MUL_3`: Multiplication by 3 is configured
 * @retval - @ref `RCC_PLL_MUL_4`: Multiplication by 4 is configured
 * @retval - @ref `RCC_PLL_MUL_5`: Multiplication by 5 is configured
 * @retval - @ref `RCC_PLL_MUL_6`: Multiplication by 6 is configured
 * @retval - @ref `RCC_PLL_MUL_7`: Multiplication by 7 is configured
 * @retval - @ref `RCC_PLL_MUL_8`: Multiplication by 8 is configured
 * @retval - @ref `RCC_PLL_MUL_9`: Multiplication by 9 is configured
 * @retval - @ref `RCC_PLL_MUL_10`: Multiplication by 10 is configured
 * @retval - @ref `RCC_PLL_MUL_11`: Multiplication by 11 is configured
 * @retval - @ref `RCC_PLL_MUL_12`: Multiplication by 12 is configured
 * @retval - @ref `RCC_PLL_MUL_13`: Multiplication by 13 is configured
 * @retval - @ref `RCC_PLL_MUL_14`: Multiplication by 14 is configured
 * @retval - @ref `RCC_PLL_MUL_15`: Multiplication by 15 is configured
 * @retval - @ref `RCC_PLL_MUL_16`: Multiplication by 16 is configured
 */
rcc_pll_mul_t RCC_GetPLLMultiplier(void);

/** @} */ // RCC_03_Driver_04_SystemClock

// ==================================================================================================== //
// RCC Prescaler APIs
// ==================================================================================================== //

/**
 * @addtogroup RCC_03_Driver_05_Prescalers
 * @{
 */

/**
 * @brief Applies AHB, APB1, and APB2 prescalers as one domain
 * @param[in] pBusConfig Bus-prescaler configuration to apply
 * Expected values:
 * - Non-`NULL`: Complete AHB, APB1, and APB2 policy
 * @returns @ref driver_status_t "Bus-prescaler operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Requested bus prescalers were applied
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pBusConfig was a null pointer
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: One or more bus prescaler values were invalid
 * @retval - @ref `DRIVER_STATUS_ERROR`: Hardware state could not be read while refreshing derived frequencies
 */
driver_status_t RCC_ConfigBusPrescalers(const rcc_bus_config_t* const pBusConfig);

/**
 * @brief Applies ADC and USB prescalers as one component domain
 * @param[in] pComponentConfig Component-prescaler configuration to apply
 * Expected values:
 * - Non-`NULL`: Complete ADC and USB policy
 * @returns @ref driver_status_t "Component-prescaler operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Requested component prescalers were applied
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pComponentConfig was a null pointer
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: One or more component prescaler values were invalid
 * @retval - @ref `DRIVER_STATUS_ERROR`: Hardware state could not be read while refreshing derived frequencies
 */
driver_status_t RCC_ConfigComponentPrescalers(const rcc_component_config_t* const pComponentConfig);

/**
 * @brief Returns the configured prescaler selector for a requested bus
 * @param[in] bus Target bus selector
 * Accepted values:
 * - @ref `RCC_AHB_BUS`: AHB bus
 * - @ref `RCC_APB1_BUS`: APB1 bus
 * - @ref `RCC_APB2_BUS`: APB2 bus
 * @returns Configured bus prescaler selector
 * @retval - @ref `RCC_AHB_DIV_1`: AHB divide-by-one is configured, hardware read failed, or @p bus is invalid
 * @retval - @ref `RCC_AHB_DIV_2`: AHB divide-by-two is configured
 * @retval - @ref `RCC_AHB_DIV_4`: AHB divide-by-four is configured
 * @retval - @ref `RCC_AHB_DIV_8`: AHB divide-by-eight is configured
 * @retval - @ref `RCC_AHB_DIV_16`: AHB divide-by-sixteen is configured
 * @retval - @ref `RCC_AHB_DIV_64`: AHB divide-by-sixty-four is configured
 * @retval - @ref `RCC_AHB_DIV_128`: AHB divide-by-128 is configured
 * @retval - @ref `RCC_AHB_DIV_256`: AHB divide-by-256 is configured
 * @retval - @ref `RCC_AHB_DIV_512`: AHB divide-by-512 is configured
 * @retval - @ref `RCC_APB1_DIV_2`: APB1 divide-by-two is configured
 * @retval - @ref `RCC_APB1_DIV_4`: APB1 divide-by-four is configured
 * @retval - @ref `RCC_APB1_DIV_8`: APB1 divide-by-eight is configured
 * @retval - @ref `RCC_APB1_DIV_16`: APB1 divide-by-sixteen is configured
 * @retval - @ref `RCC_APB2_DIV_2`: APB2 divide-by-two is configured
 * @retval - @ref `RCC_APB2_DIV_4`: APB2 divide-by-four is configured
 * @retval - @ref `RCC_APB2_DIV_8`: APB2 divide-by-eight is configured
 * @retval - @ref `RCC_APB2_DIV_16`: APB2 divide-by-sixteen is configured
 * @note The three divide-by-one selector macros share the same scalar value
 */
rcc_bus_prescaler_t RCC_GetBusPrescaler(const rcc_bus_t bus);

/** @} */ // RCC_03_Driver_05_Prescalers

// ==================================================================================================== //
// RCC Config APIs
// ==================================================================================================== //

/**
 * @addtogroup RCC_03_Driver_06_Config
 * @{
 */

/**
 * @brief Validates a complete RCC and Flash transition request
 * @details Performs selector, derived-frequency, USB, and Flash-latency checks without hardware mutation.
 * @param[in] pRCCConfig Root configuration to validate
 * Expected values:
 * - Non-`NULL`: Complete Flash and clock-tree policy
 * @returns @ref driver_status_t "RCC configuration validation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Configuration is valid and internally consistent
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRCCConfig was a null pointer
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: Configuration contains an invalid source, divider,
 * multiplier, or limit violation
 */
driver_status_t RCC_ValidateConfig(const rcc_config_t* const pRCCConfig);

/**
 * @brief Applies one complete clock-tree configuration transaction
 * @details
 * Validates every selector before the first oscillator or register mutation,
 * moves SYSCLK away from PLL before reconfiguration, applies prescalers,
 * configures and starts PLL when requested, switches SYSCLK, and refreshes the
 * derived-frequency cache.
 * @param[in] pClockTreeConfig Clock-tree configuration to apply
 * Expected values:
 * - Non-`NULL`: Complete system, bus, and component policy
 * @returns @ref driver_status_t "Clock-tree configuration operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Clock-tree sequencing completed successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pClockTreeConfig was a null pointer
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: Configuration contained invalid clock-tree values
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: PLL lost ready state before a requested PLL switch
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: A source ready-state or switch confirmation timed out
 * @retval - @ref `DRIVER_STATUS_ERROR`: Hardware status could not be read during sequencing
 * @note An invalid configuration causes no hardware write
 * @warning A timeout after sequencing begins may leave an intermediate safe clock state; no rollback is attempted
 */
driver_status_t RCC_ConfigClockTree(const rcc_clock_tree_config_t* const pClockTreeConfig);

/**
 * @brief Applies one complete RCC and Flash transition transaction
 * @details
 * Validates the entire request first. Flash latency is raised before increasing
 * SYSCLK and lowered only after reducing SYSCLK, preserving the access-timing
 * constraint in both transition directions.
 * @param[in] pRCCConfig Root configuration to apply
 * Expected values:
 * - Non-`NULL`: Complete Flash and clock-tree policy
 * @returns @ref driver_status_t "RCC configuration operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Flash and clock-tree configuration completed successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRCCConfig was a null pointer
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: Configuration contained invalid fields or violated frequency limits
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: PLL lost ready state before a requested PLL switch
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: A source ready-state or switch confirmation timed out
 * @retval - @ref `DRIVER_STATUS_ERROR`: Hardware status could not be read during sequencing
 * @note An invalid configuration causes no hardware write
 */
driver_status_t RCC_Config(const rcc_config_t* const pRCCConfig);

/**
 * @brief Loads the default 72 MHz Blue Pill configuration preset
 * @param[out] pRCCConfig Destination for the complete preset
 * Expected values:
 * - Non-`NULL`: Every configuration member is populated
 * @returns @ref driver_status_t "Default-configuration load status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Complete preset was loaded
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRCCConfig was a null pointer
 */
driver_status_t RCC_Load72MHzDefaultConfig(rcc_config_t* const pRCCConfig);

/**
 * @brief Applies the default 72 MHz Blue Pill RCC configuration
 * @returns @ref driver_status_t "Default RCC configuration operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Default 72 MHz configuration completed successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: Default configuration validation failed
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: PLL lost ready state before the system-clock switch
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: A source ready-state or switch confirmation timed out
 * @retval - @ref `DRIVER_STATUS_ERROR`: Hardware status could not be read during sequencing
 */
driver_status_t RCC_Config72MHz(void);

/** @} */ // RCC_03_Driver_06_Config

/** @} */ // RCC_03_Driver

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RCC_H_ */
