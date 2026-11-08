/**
 * @file i2c_data_types.h
 * @author Shrey Shah
 * @brief I2C Driver Data Types
 * @version v1.1
 * @date 2026-08-28
 * @details
 * This file contains the scalar aliases, 7-bit address type, transfer size
 * type, the error/event selector typedefs, and the speed-mode/duty-cycle
 * typedefs of the I2C Driver. It is the common ground for typedef aliases
 * only: the admitted value macros of these types are defined in
 * @ref `i2c_defines.h` beside their validation macros.
 */

// Header Guards
#ifndef I2C_DATA_TYPES_H_
#define I2C_DATA_TYPES_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx_data_types.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup I2C_02_DataTypes I2C Data Types
 * @ingroup I2C
 * @brief Type definitions for I2C driver
 * @details
 * Scalar aliases, address types, transfer sizes, error codes, and
 * transfer result structures used throughout the I2C driver stack.
 * @{
 */

/**
 * @brief I2C bus speed preset type
 * @typedef i2c_mode_t
 * @details
 * Selects the target SCL frequency preset for @ref `i2c_config_t`. The
 * admitted preset value macros are defined in @ref `i2c_defines.h` beside
 * their validation macro.
 */
typedef uint8_t								i2c_mode_t;

/**
 * @brief I2C Fast-mode duty cycle type
 * @typedef i2c_fast_duty_t
 * @details
 * Selects the SCL low/high period ratio in Fast mode (400 kHz) operation;
 * the Tlow/Thigh relationship affects rise-time margin and pull-up
 * requirements per RM0008 Section 26.4.7. The admitted duty-cycle value
 * macros are defined in @ref `i2c_defines.h` beside their validation macro.
 */
typedef uint8_t								i2c_fast_duty_t;

/**
 * @brief Unshifted 7-bit I2C target address type
 * @typedef i2c_address_7bit_t
 * @details
 * Valid range is 0x08..0x77. Reserved addresses 0x00..0x07 and 0x78..0x7F
 * are rejected by the Driver. The Codec layer encodes this value into the
 * address byte by shifting left one bit and adding the R/W bit.
 */
typedef uint8_t								i2c_address_t;

/**
 * @brief I2C data byte type
 * @typedef i2c_data_t
 * @details
 * Single-byte payload for transmit or receive operations. The Driver
 * does not impose a maximum transfer size, but the application may
 * limit the number of bytes to send or receive.
 */
typedef uint8_t								i2c_data_t;

/**
 * @brief I2C register address type
 * @typedef i2c_reg_addr_t
 * @details
 * Single-byte register address for memory-mapped I2C devices.
 */
typedef uint8_t								i2c_reg_addr_t;

/**
 * @brief I2C transfer byte count type
 * @typedef i2c_data_length_t
 * @details
 * Number of bytes to transmit or receive. Zero-byte transfers are
 * rejected by the Driver. Maximum size is bounded by application
 * requirements and does not have a hardware limit other than
 * address-byte field width.
 */
typedef uint16_t							i2c_data_length_t;

/**
 * @brief I2C error type for transfer results
 * @typedef i2c_error_t
 * @details
 * Primary error cause reported in a transfer result. The Driver selects
 * a single primary error from the `SR1` hardware flags and software
 * timeout/stuck-`BUSY` conditions. The admitted error value macros are
 * defined in @ref `i2c_defines.h`.
 */
typedef uint8_t								i2c_error_t;

/**
 * @brief Application-domain status event mask awaited by one transaction step
 * @typedef i2c_event_flag_t
 * @details
 * Names the status events a bounded transfer step waits for in the Driver's
 * application vocabulary, which deliberately does not mirror any register
 * layout. Bit negotiation between application events and physical registers
 * lives in the Codec: @ref `Codec_I2C_IsEventPending` translates an admitted
 * selector into its raw `SR1` flag mask, and the `SR2` bus condition is
 * answered through @ref `Codec_I2C_IsBusBusy`, so the Driver never performs
 * register-field work and no two application selectors can ever collide just
 * because `SR1` and `SR2` use overlapping bit positions. A single-bit mask
 * names one hardware event; a composite mask is formed from individual
 * selectors, so an event is complete only once every selected flag is set —
 * which is how the transmit tail expresses RM0008's `EV8_2` (`TXE` together
 * with `BTF`). Every individual selector owns a unique application-domain
 * bit, so selector-equality tests in the Driver choreography are sound. The
 * admitted selector value macros are defined in @ref `i2c_defines.h`.
 */
typedef uint16_t								i2c_event_flag_t;

/** @} */ // I2C_02_DataTypes

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_DATA_TYPES_H_ */
