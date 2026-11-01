/**
 * @file i2c_config.h
 * @author Shrey Shah
 * @brief I2C Driver Configuration Structure
 * @version v1.0
 * @date 2026-08-28
 * @details
 * This file contains the instance-independent timing configuration structure
 * for the I2C Driver. The configuration includes bus speed preset and
 * fast mode duty cycle selection.
 */

// Header Guards
#ifndef I2C_CONFIG_H_
#define I2C_CONFIG_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "i2c_defines.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
 *       must be enabled by the application before calling I2C_Config.
 * @see I2C_ARCHITECTURE.md Section "Configuration Model" for timing equations
 */
typedef struct
{
	/**
	 * @brief Bus speed preset
	 * @details
	 * Selects the target SCL frequency. The Driver computes CR2.FREQ,
	 * CCR, and TRISE values based on this preset and the live APB1 clock.
	 */
	i2c_speed_preset_t bus_speed;

	/**
	 * @brief Fast mode duty cycle
	 * @details
	 * Meaningful only when bus_speed is I2C_SPEED_PRESET_400_KHZ.
	 * Selects the SCL low/high period ratio in Fast mode.
	 */
	i2c_fast_duty_t fast_mode_duty;
} i2c_config_t;

/** @} */ // I2C_02_Config

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_CONFIG_H_ */
