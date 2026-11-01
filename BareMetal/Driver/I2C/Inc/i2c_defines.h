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
 * @defgroup I2C_02_Defines_01_SpeedPresets I2C Speed Presets
 * @ingroup I2C_02_Defines
 * @brief Admitted I2C bus speed presets
 * @details
 * The 88 kHz preset is the portable Standard-mode choice for repeated
 * START on affected silicon per ES096 2.8.5. The 100 kHz preset remains
 * valid for ordinary transfers but is rejected for combined operations
 * above 88 kHz Standard mode.
 * @{
 */

/** @brief I2C - Standard Mode (100kHz) @def I2C_SPEED_STANDARD_MODE */
#define I2C_SPEED_STANDARD_MODE				((frequency_t) 100U * FREQ_1kHz)
/** @brief I2C - Fast mode (400kHz) @def I2C_SPEED_FAST_MODE */
#define I2C_SPEED_FAST_MODE					((frequency_t) 400U * FREQ_1kHz)
/**
 * @brief Validate an I2C bus speed
 * @def I2C_IS_VALID_SPEED
 * @param[in] speed I2C Bus Speed
 * @returns I2C Speed Validation Result
 * @retval - `0x00U`: Speed Invalid
 * @retval - `0x01U`: Speed Valid
 */
#define I2C_IS_VALID_SPEED(speed)			\
	((speed) == I2C_SPEED_STANDARD_MODE || (speed) == I2C_SPEED_FAST_MODE)

/** @} */ // I2C_02_Defines_01_SpeedPresets

/**
 * @defgroup I2C_02_Defines_02_FastDuty I2C Fast Mode Duty Cycle
 * @ingroup I2C_02_Defines
 * @brief Fast mode duty cycle selection
 * @details
 * Controls the SCL low/high period ratio in Fast mode. Meaningful only
 * when the speed preset is I2C_SPEED_FAST_MODE.
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
	((duty) == I2C_FAST_DUTY_2_1 || (duty) == I2C_FAST_DUTY_16_9)

/** @} */ // I2C_02_Defines_02_FastDuty

/**
 * @defgroup I2C_02_Defines_03_Validation I2C Validation Macros
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

/** @} */ // I2C_02_Defines_03_Validation

/** @} */ // I2C_02_Defines

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_DEFINES_H_ */
