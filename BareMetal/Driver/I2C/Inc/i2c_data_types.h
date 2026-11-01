/**
 * @file i2c_data_types.h
 * @author Shrey Shah
 * @brief I2C Driver Data Types
 * @version v1.0
 * @date 2026-08-28
 * @details
 * This file contains the scalar aliases, 7-bit address type, transfer size
 * type, error/event masks, and transfer result type for the I2C Driver.
 * These types are used across the Driver, Codec, and public API layers.
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
 * @brief Unshifted 7-bit I2C target address type
 * @typedef i2c_address_7bit_t
 * @details
 * Valid range is 0x08..0x77. Reserved addresses 0x00..0x07 and 0x78..0x7F
 * are rejected by the Driver. The Codec layer encodes this value into the
 * address byte by shifting left one bit and adding the R/W bit.
 */
typedef uint8_t									i2c_address_t;

/**
 * @brief I2C data byte type
 * @typedef i2c_data_t
 * @details
 * Single-byte payload for transmit or receive operations. The Driver
 * does not impose a maximum transfer size, but the application may
 * limit the number of bytes to send or receive.
 */
typedef uint8_t									i2c_data_t;

/**
 * @brief I2C register address type
 * @typedef i2c_reg_addr_t
 * @details
 * Single-byte register address for memory-mapped I2C devices.
 */
typedef uint8_t									i2c_reg_addr_t;

/**
 * @brief I2C transfer byte count type
 * @typedef i2c_transfer_size_t
 * @details
 * Number of bytes to transmit or receive. Zero-byte transfers are
 * rejected by the Driver. Maximum size is bounded by application
 * requirements and does not have a hardware limit other than
 * address-byte field width.
 */
typedef uint16_t								i2c_data_length_t;

/**
 * @brief I2C error type for transfer results
 * @typedef i2c_error_t
 * @details
 * Primary error cause reported in a transfer result. The Driver selects
 * a single primary error from the SR1 hardware flags and software
 * timeout/stuck-BUSY conditions.
 */
typedef uint8_t						i2c_error_t;

/** @brief No error @def I2C_ERROR_NONE */
#define I2C_ERROR_NONE						((i2c_error_t) 0U)
/** @brief Address NACK (AF before ADDR) @def I2C_ERROR_ADDRESS_NACK */
#define I2C_ERROR_ADDRESS_NACK				((i2c_error_t) 1U)
/** @brief Data NACK (AF after data) @def I2C_ERROR_DATA_NACK */
#define I2C_ERROR_DATA_NACK					((i2c_error_t) 2U)
/** @brief Arbitration lost @def I2C_ERROR_ARBITRATION_LOST */
#define I2C_ERROR_ARBITRATION_LOST			((i2c_error_t) 3U)
/** @brief Bus error @def I2C_ERROR_BUS_ERROR */
#define I2C_ERROR_BUS_ERROR					((i2c_error_t) 4U)
/** @brief Overrun/underrun @def I2C_ERROR_OVERRUN */
#define I2C_ERROR_OVERRUN					((i2c_error_t) 5U)
/** @brief Hardware timeout (SMBus) @def I2C_ERROR_HARDWARE_TIMEOUT */
#define I2C_ERROR_HARDWARE_TIMEOUT			((i2c_error_t) 6U)
/** @brief Software timeout (poll budget exhausted) @def I2C_ERROR_SOFTWARE_TIMEOUT */
#define I2C_ERROR_SOFTWARE_TIMEOUT			((i2c_error_t) 7U)
/** @brief Stuck BUSY before START @def I2C_ERROR_STUCK_BUSY */
#define I2C_ERROR_STUCK_BUSY				((i2c_error_t) 8U)
/** @brief Recovery failed @def I2C_ERROR_RECOVERY_FAILED */
#define I2C_ERROR_RECOVERY_FAILED			((i2c_error_t) 9U)

/**
 * @brief I2C transfer result structure
 * @typedef i2c_transfer_result_t
 * @details
 * Records exact transfer progress and the primary typed error. Published
 * by the Driver after START is requested; unchanged on preflight failure.
 */
typedef struct
{
	//! Number of bytes successfully transmitted
	uint16_t	txBytesTransferred;

	//! Number of bytes successfully received
	uint16_t	rxBytesTransferred;

	//! Primary error cause
	i2c_error_t	error;
} i2c_transfer_result_t;

/** @} */ // I2C_02_DataTypes

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_DATA_TYPES_H_ */
