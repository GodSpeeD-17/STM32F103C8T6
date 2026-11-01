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

#ifndef I2C_DEFINES_H_
#define I2C_DEFINES_H_

// Includes
#include "i2c_data_types.h"

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

/**
 * @brief I2C speed preset enumeration
 * @typedef i2c_speed_preset_t
 * @details
 * Admitted bus speed presets for I2C configuration. The 88 kHz preset
 * is ES096-safe for repeated START in Standard mode. The 100 kHz preset
 * is valid for ordinary transfers but not for combined write-then-read
 * operations in Standard mode.
 */
typedef enum
{
	//! 88 kHz Standard mode (ES096-safe for repeated START)
	I2C_SPEED_PRESET_88_KHZ = 0,

	//! 100 kHz Standard mode (ordinary transfers only)
	I2C_SPEED_PRESET_100_KHZ,

	//! 400 kHz Fast mode
	I2C_SPEED_PRESET_400_KHZ
} i2c_speed_preset_t;

/** @} */ // I2C_02_Defines_01_SpeedPresets

/**
 * @defgroup I2C_02_Defines_02_FastDuty I2C Fast Mode Duty Cycle
 * @ingroup I2C_02_Defines
 * @brief Fast mode duty cycle selection
 * @details
 * Controls the SCL low/high period ratio in Fast mode. Meaningful only
 * when the speed preset is I2C_SPEED_PRESET_400_KHZ.
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
typedef enum
{
	//! DUTY=0: Tlow/Thigh = 2/1 (CCR = PCLK1 / (3 * FSCL))
	I2C_FAST_DUTY_2_1 = 0,

	//! DUTY=1: Tlow/Thigh = 16/9 (CCR = PCLK1 / (25 * FSCL))
	I2C_FAST_DUTY_16_9
} i2c_fast_duty_t;

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
 * @def I2C_IS_VALID_7BIT_ADDRESS(addr)
 * @brief Validate a 7-bit I2C address
 * @param addr Address to validate (i2c_address_7bit_t)
 * @details
 * Returns true if the address is in the valid range 0x08..0x77.
 * Reserved addresses 0x00..0x07 and 0x78..0x7F are rejected.
 */
#define I2C_IS_VALID_7BIT_ADDRESS(addr) \
	(((addr) >= 0x08U) && ((addr) <= 0x77U))

/**
 * @def I2C_IS_RESERVED_ADDRESS(addr)
 * @brief Check if an address is reserved
 * @param addr Address to check (i2c_address_7bit_t)
 * @details
 * Returns true if the address is in the reserved range 0x00..0x07
 * or 0x78..0x7F. Reserved addresses include general call (0x00),
 * CBUS addresses (0x01..0x02), and other reserved ranges.
 */
#define I2C_IS_RESERVED_ADDRESS(addr) \
	(((addr) <= 0x07U) || ((addr) >= 0x78U))

/**
 * @def I2C_IS_VALID_TRANSFER_SIZE(size)
 * @brief Validate a transfer size
 * @param size Size to validate (i2c_transfer_size_t)
 * @details
 * Returns true if the size is non-zero. Zero-byte transfers are
 * rejected by the Driver as they have no defined protocol meaning.
 */
#define I2C_IS_VALID_TRANSFER_SIZE(size) \
	((size) > 0U)

/**
 * @def I2C_IS_VALID_POLL_BUDGET(budget)
 * @brief Validate a poll budget
 * @param budget Budget to validate (uint32_t)
 * @details
 * Returns true if the budget is non-zero. A zero poll budget would
 * cause immediate timeout and is rejected by the Driver.
 */
#define I2C_IS_VALID_POLL_BUDGET(budget) \
	((budget) > 0U)

/**
 * @def I2C_IS_VALID_SPEED_PRESET(preset)
 * @brief Validate a speed preset
 * @param preset Preset to validate (i2c_speed_preset_t)
 * @details
 * Returns true if the preset is one of the admitted values.
 */
#define I2C_IS_VALID_SPEED_PRESET(preset) \
	(((preset) == I2C_SPEED_PRESET_88_KHZ) || \
	 ((preset) == I2C_SPEED_PRESET_100_KHZ) || \
	 ((preset) == I2C_SPEED_PRESET_400_KHZ))

/**
 * @def I2C_IS_VALID_FAST_DUTY(duty)
 * @brief Validate a fast mode duty cycle
 * @param duty Duty to validate (i2c_fast_duty_t)
 * @details
 * Returns true if the duty is one of the admitted values.
 */
#define I2C_IS_VALID_FAST_DUTY(duty) \
	(((duty) == I2C_FAST_DUTY_2_1) || \
	 ((duty) == I2C_FAST_DUTY_16_9))

/**
 * @def I2C_IS_INSTANCE_I2C1(I2Cx)
 * @brief Check if a pointer is I2C1
 * @param I2Cx Peripheral pointer to check
 * @details
 * Returns true if the pointer matches I2C1 base address.
 */
#define I2C_IS_INSTANCE_I2C1(I2Cx) \
	((I2Cx) == I2C1)

/**
 * @def I2C_IS_INSTANCE_I2C2(I2Cx)
 * @brief Check if a pointer is I2C2
 * @param I2Cx Peripheral pointer to check
 * @details
 * Returns true if the pointer matches I2C2 base address.
 */
#define I2C_IS_INSTANCE_I2C2(I2Cx) \
	((I2Cx) == I2C2)

/**
 * @def I2C_IS_VALID_INSTANCE(I2Cx)
 * @brief Validate an I2C instance pointer
 * @param I2Cx Peripheral pointer to validate
 * @details
 * Returns true if the pointer is either I2C1 or I2C2.
 */
#define I2C_IS_VALID_INSTANCE(I2Cx) \
	(I2C_IS_INSTANCE_I2C1(I2Cx) || I2C_IS_INSTANCE_I2C2(I2Cx))

/** @} */ // I2C_02_Defines_03_Validation

/** @} */ // I2C_02_Defines

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_DEFINES_H_ */
