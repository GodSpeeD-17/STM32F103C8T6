/**
 * @file i2c_defines.h
 * @author Shrey Shah
 * @brief I2C Driver Public Selectors and Validation Macros
 * @version v1.0
 * @date 2026-08-28
 * @details
 * This file contains public selector values and pure validation macros
 * for the I2C Driver. These are used by the application layer and
 * internally by the Driver for validation.
 */

// Header Guards
#ifndef I2C_DEFINES_H_
#define I2C_DEFINES_H_

// Includes
#include "i2c_data_types.h"

// C++ Compatibility
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup I2C_02_Defines I2C Public Selectors and Validation
 * @ingroup I2C
 * @brief Public selector values and validation macros
 * @details
 * Speed presets, duty cycle selectors, and pure validation macros
 * for I2C configuration and operation.
 * @{
 */

/**
 * @defgroup I2C_02_Defines_01_Validation I2C Validation Macros
 * @ingroup I2C_02_Defines
 * @brief Pure validation macros for I2C parameters
 * @details
 * These macros perform compile-time or runtime validation of I2C
 * parameters without side effects. They return boolean results
 * and are used by the Driver for preflight validation.
 * @{
 */

/**
 * @def I2C_IS_VALID_INSTANCE
 * @brief Validate an I2C instance pointer
 * @param[in] I2Cx Peripheral pointer to validate
 * @returns I2C Instance Validation Result
 * @retval - `0x00U`: Instance Invalid
 * @retval - `0x01U`: Instance Valid
 */
#define I2C_IS_VALID_INSTANCE(I2Cx)		\
	(((I2Cx) == I2C1) || ((I2Cx) == I2C2))

/** @} */ // I2C_02_Defines_01_Validation

/**
 * @defgroup I2C_02_Defines_02_SpeedPresets I2C Speed Presets
 * @ingroup I2C_02_Defines
 * @brief Admitted I2C bus speed presets
 * @details
 * The 88 kHz preset is the portable Standard-mode choice for repeated
 * START on affected silicon per ES096 2.8.5. The 100 kHz preset remains
 * valid for ordinary transfers but is rejected for combined operations
 * above 88 kHz Standard mode.
 * @{
 */

typedef uint8_t								i2c_mode_t;
/** @brief I2C - Standard Mode (100kHz) @def I2C_BUS_STANDARD_MODE */
#define I2C_BUS_STANDARD_MODE				((i2c_mode_t) 0U)
/** @brief I2C - Fast mode (400kHz) @def I2C_BUS_FAST_MODE */
#define I2C_BUS_FAST_MODE					((i2c_mode_t) 1U)
/**
 * @brief Validate an I2C mode
 * @def I2C_IS_VALID_MODE
 * @param[in] mode I2C Mode
 * @returns I2C Mode Validation Result
 * @retval - `0x00U`: Mode Invalid
 * @retval - `0x01U`: Mode Valid
 */
#define I2C_IS_VALID_MODE(mode)			\
	(((mode) == I2C_BUS_STANDARD_MODE) || ((mode) == I2C_BUS_FAST_MODE))

/** @} */ // I2C_02_Defines_02_SpeedPresets

/**
 * @defgroup I2C_02_Defines_03_FastDuty I2C Fast Mode Duty Cycle
 * @ingroup I2C_02_Defines
 * @brief Fast mode duty cycle selection
 * @details
 * Controls the SCL low/high period ratio in Fast mode. Meaningful only
 * when the speed preset is I2C_BUS_FAST_MODE.
 * @{
 */

/**
 * @brief I2C Fast mode duty cycle enumeration
 * @typedef i2c_fast_duty_t
 * @details
 * Duty cycle selection for Fast mode (400 kHz) operation. The Tlow/Thigh
 * relationship affects rise-time margin and pull-up requirements per
 * RM0008 Section 26.4.7.
 */
typedef uint8_t								i2c_fast_duty_t;
/** @brief DUTY=0: Tlow/Thigh = 2/1 (CCR = PCLK1 / (3 * FSCL)) @def I2C_FAST_DUTY_2_1 */
#define I2C_FAST_DUTY_2_1					((i2c_fast_duty_t) 0U)
/** @brief DUTY=1: Tlow/Thigh = 16/9 (CCR = PCLK1 / (25 * FSCL)) @def I2C_FAST_DUTY_16_9 */
#define I2C_FAST_DUTY_16_9					((i2c_fast_duty_t) 1U)
/**
 * @brief Validate an I2C fast mode duty cycle
 * @def I2C_IS_VALID_FAST_DUTY
 * @param[in] duty I2C Fast Mode Duty Cycle
 * @returns I2C Fast Mode Duty Cycle Validation Result
 * @retval - `0x00U`: Duty Cycle Invalid
 * @retval - `0x01U`: Duty Cycle Valid
 */
#define I2C_IS_VALID_FAST_DUTY(duty)		\
	(((duty) == I2C_FAST_DUTY_2_1) || ((duty) == I2C_FAST_DUTY_16_9))

/** @} */ // I2C_02_Defines_03_FastDuty

/**
 * @defgroup I2C_02_Defines_04_Transfers I2C Transfer Selectors
 * @ingroup I2C_02_Defines
 * @brief Admitted target-address range and transaction event-flag selectors
 * @details
 * Bounds the 7-bit target-address vocabulary accepted by the synchronous
 * transfer APIs and validates the `SR1` event masks the Driver may wait for.
 * Reserved addresses are excluded at the Driver boundary so a transfer request
 * can never silently address a reserved device or another controller. The
 * event selectors keep the raw status bits in the Codec while still letting
 * the Driver name exactly which hardware event a step is waiting for.
 * @{
 */

/** @brief Lowest address in the admitted non-reserved 7-bit target range @def I2C_ADDRESS_MIN_7BIT */
#define I2C_ADDRESS_MIN_7BIT				((i2c_address_t) 0x08U)
/** @brief Highest address in the admitted non-reserved 7-bit target range @def I2C_ADDRESS_MAX_7BIT */
#define I2C_ADDRESS_MAX_7BIT				((i2c_address_t) 0x77U)

/**
 * @def I2C_IS_VALID_ADDRESS
 * @brief Validates an unshifted 7-bit I2C target address
 * @details
 * Accepts only the non-reserved 7-bit range, so the two reserved blocks
 * `0x00`..`0x07` and `0x78`..`0x7F` are refused before any bus activity.
 * @param[in] slaveAddress Unshifted 7-bit target address to validate
 * @returns I2C Target Address Validation Result
 * @retval - `0x00U`: Address is reserved or outside the 7-bit range
 * @retval - `0x01U`: Address is inside `0x08U`..`0x77U`
 */
#define I2C_IS_VALID_ADDRESS(slaveAddress)	\
	(((slaveAddress) >= I2C_ADDRESS_MIN_7BIT) && ((slaveAddress) <= I2C_ADDRESS_MAX_7BIT))

/**
 * @def I2C_IS_VALID_EVENT_FLAG
 * @brief Validates an `SR1` event-flag selector
 * @details
 * Admits exactly the published selectors, including the composite
 * @ref `I2C_EVENT_FLAG_TRANSFER_COMPLETE` mask that the transmit tail needs so
 * a partially satisfied completion request cannot be mistaken for an unknown
 * selector.
 * @param[in] eventFlag Event-flag mask to validate
 * @returns I2C Event-Flag Validation Result
 * @retval - `0x00U`: Mask is not an admitted selector
 * @retval - `0x01U`: Mask is an admitted selector
 */
#define I2C_IS_VALID_EVENT_FLAG(eventFlag)	\
	(((eventFlag) == I2C_EVENT_FLAG_START_GENERATED) ||	\
	((eventFlag) == I2C_EVENT_FLAG_ADDRESS_SENT) ||	\
	((eventFlag) == I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED) ||	\
	((eventFlag) == I2C_EVENT_FLAG_DATA_REGISTER_EMPTY) ||	\
	((eventFlag) == I2C_EVENT_FLAG_TRANSFER_COMPLETE))

/** @} */ // I2C_02_Defines_04_Transfers

/** @} */ // I2C_02_Defines

/**
 * @defgroup I2C_02_Config I2C Configuration Structure
 * @ingroup I2C
 * @brief Instance-independent timing configuration
 * @details
 * Timing configuration structure for I2C peripheral initialization.
 * Contains bus speed preset and fast mode duty cycle selection.
 * @{
 */

/**
 * @brief I2C timing configuration structure
 * @typedef i2c_config_t
 * @details
 * Instance-independent timing configuration for I2C peripheral.
 * The Driver uses this structure to compute CR2.FREQ, CCR, and TRISE
 * register values based on the live APB1 clock frequency.
 *
 * **Speed Presets:**
 * - 88 kHz: ES096-safe Standard mode for repeated START
 * - 100 kHz: Standard mode for ordinary transfers
 * - 400 kHz: Fast mode (requires duty cycle selection)
 *
 * **Fast Mode Duty Cycle:**
 * - 2:1: Tlow/Thigh = 2/1 (DUTY=0, CCR = PCLK1 / (3 * FSCL))
 * - 16:9: Tlow/Thigh = 16/9 (DUTY=1, CCR = PCLK1 / (25 * FSCL))
 *
 * The 88 kHz preset is the portable Standard-mode choice for combined
 * write-then-read operations on silicon affected by ES096 2.8.5.
 * The 100 kHz preset is valid for ordinary single-phase transfers but
 * is rejected for combined operations in Standard mode above 88 kHz.
 *
 * @note Configuration requires PE=0 and a free bus. The APB1 clock
 *       must be enabled by the application before calling I2C_Init.
 * @see I2C_ARCHITECTURE.md Section "Configuration Model" for timing equations
 */
typedef struct
{
	/**
	 * @brief Bus speed preset
	 * @details
	 * Selects the target SCL frequency. The Driver computes CR2.FREQ,
	 * CCR, and TRISE values based on this preset and the live APB1 clock.
	 * Accepted Values:
	 * - @ref `I2C_BUS_STANDARD_MODE`
	 * - @ref `I2C_BUS_FAST_MODE`
	 */
	i2c_mode_t			busMode: 1;

	/**
	 * @brief Fast mode duty cycle
	 * @details
	 * Meaningful only when bus_speed is @ref `I2C_BUS_FAST_MODE`.
	 * Selects the SCL low/high period ratio in Fast mode.
	 * Accepted Values:
	 * - @ref `I2C_FAST_DUTY_2_1`
	 * - @ref `I2C_FAST_DUTY_16_9`
	 */
	i2c_fast_duty_t		fastModeDuty: 1;
} i2c_config_t;

/**
 * @brief	Validates an I2C configuration structure
 * @def		I2C_IS_VALID_CONFIG
 * @param[in] pConfig Pointer to an I2C configuration structure
 * @returns I2C Configuration Validation Result
 * @retval - `0x00U`: Configuration Invalid
 * @retval - `0x01U`: Configuration Valid
 */
#define I2C_IS_VALID_CONFIG(pConfig)		\
	((I2C_IS_VALID_MODE((pConfig)->busMode)) && (I2C_IS_VALID_FAST_DUTY((pConfig)->fastModeDuty)))

/** @} */ // I2C_02_Config

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_DEFINES_H_ */
