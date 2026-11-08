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
 * @typedef i2c_transfer_size_t
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
 * a single primary error from the SR1 hardware flags and software
 * timeout/stuck-BUSY conditions.
 */
typedef uint8_t								i2c_error_t;

/** @brief No error @def I2C_ERROR_NONE */
#define I2C_ERROR_NONE								((i2c_error_t) 0U)
/** @brief Address NACK (AF before ADDR) @def I2C_ERROR_ADDRESS_NACK */
#define I2C_ERROR_ADDRESS_NACK						((i2c_error_t) 1U)
/** @brief Data NACK (AF after data) @def I2C_ERROR_DATA_NACK */
#define I2C_ERROR_DATA_NACK							((i2c_error_t) 2U)
/** @brief Arbitration lost @def I2C_ERROR_ARBITRATION_LOST */
#define I2C_ERROR_ARBITRATION_LOST					((i2c_error_t) 3U)
/** @brief Bus error @def I2C_ERROR_BUS_ERROR */
#define I2C_ERROR_BUS_ERROR							((i2c_error_t) 4U)
/** @brief Overrun/underrun @def I2C_ERROR_OVERRUN */
#define I2C_ERROR_OVERRUN							((i2c_error_t) 5U)
/** @brief Hardware timeout (SMBus) @def I2C_ERROR_HARDWARE_TIMEOUT */
#define I2C_ERROR_HARDWARE_TIMEOUT					((i2c_error_t) 6U)
/** @brief Software timeout (poll budget exhausted) @def I2C_ERROR_SOFTWARE_TIMEOUT */
#define I2C_ERROR_SOFTWARE_TIMEOUT					((i2c_error_t) 7U)
/** @brief Stuck BUSY before START @def I2C_ERROR_STUCK_BUSY */
#define I2C_ERROR_STUCK_BUSY						((i2c_error_t) 8U)
/** @brief Recovery failed @def I2C_ERROR_RECOVERY_FAILED */
#define I2C_ERROR_RECOVERY_FAILED					((i2c_error_t) 9U)

/**
 * @brief `I2Cx_SR1` event-flag mask waited for by one transaction step
 * @typedef i2c_event_flag_t
 * @details
 * Names the status flags a bounded transfer step waits for without exposing
 * raw register bits to the Driver. A single-bit mask names one hardware event;
 * a composite mask names an event that is only complete once every selected
 * flag is set, which is how the transmit tail expresses RM0008's EV8_2
 * (`TXE` together with `BTF`). @ref `I2C_EVENT_FLAG_BUS_IDLE` is the single
 * exception that names an `SR2` condition instead, because bus ownership has
 * no `SR1` flag. The Codec converts each selector into its flag test, so the
 * Driver never performs register-field work.
 */
typedef uint16_t								i2c_event_flag_t;

/**
 * @brief START or repeated START was generated on the bus (`I2Cx_SR1.SB`)
 * @details
 * Set by hardware once the peripheral has driven a START or repeated START
 * onto the bus. A master clears it by reading `SR1` and then writing the
 * target address byte to `DR`, so the flag both announces the event and
 * releases the address phase; clearing `PE` clears it as well.
 * @def I2C_EVENT_FLAG_START_GENERATED
 */
#define I2C_EVENT_FLAG_START_GENERATED				((i2c_event_flag_t) 0x0001U)

/**
 * @brief Target address was sent and acknowledged (`I2Cx_SR1.ADDR`)
 * @details
 * Set by hardware after the address byte has shifted out and the target has
 * acknowledged it, which is the point where the peripheral may stretch `SCL`.
 * A master clears it with the ordered `SR1` then `SR2` read pair, so the flag
 * bounds the address phase instead of being cleared speculatively.
 * @def I2C_EVENT_FLAG_ADDRESS_SENT
 */
#define I2C_EVENT_FLAG_ADDRESS_SENT					((i2c_event_flag_t) 0x0002U)

/**
 * @brief Data register and shift register are both settled (`I2Cx_SR1.BTF`)
 * @details
 * Set once a byte has moved between `DR` and the shift register and the next
 * transfer step would need software service. In transmit it becomes set only
 * after the final byte leaves `DR` and drains from the shift register, so it
 * is the safe point to request STOP without truncating the last byte.
 * @def I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED
 */
#define I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED		((i2c_event_flag_t) 0x0004U)

/**
 * @brief Transmit data register is empty (`I2Cx_SR1.TXE`)
 * @details
 * Set while `DR` can accept the next byte. It is cleared by the `DR` write
 * that fills it, by a START or STOP request, or by clearing `PE`, so it is the
 * pacing condition for every byte of a transmit payload.
 * @def I2C_EVENT_FLAG_DATA_REGISTER_EMPTY
 */
#define I2C_EVENT_FLAG_DATA_REGISTER_EMPTY			((i2c_event_flag_t) 0x0080U)

/**
 * @brief Transmit tail is complete: `TXE` and `BTF` are set together
 * @details
 * Composite selector matching RM0008's EV8_2. Requiring both flags at once
 * means the caller waits for the last byte to leave the shift register instead
 * of treating an empty `DR` alone as completion, which would let a STOP
 * request truncate the final byte.
 * @def I2C_EVENT_FLAG_TRANSFER_COMPLETE
 */
#define I2C_EVENT_FLAG_TRANSFER_COMPLETE			((i2c_event_flag_t) (I2C_EVENT_FLAG_DATA_REGISTER_EMPTY | I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED))

/**
 * @brief Peripherals released the bus (`I2Cx_SR2.BUSY` observed clear)
 * @details
 * The one selector that names an `SR2` condition instead of an `SR1` mask,
 * because bus ownership has no `SR1` flag at all. Its value is defined against
 * the `SR2` register layout (`BUSY`, bit 1), so it must never be bit-tested
 * against an `SR1` image; the Driver's wait helper branches on the selector
 * explicitly and answers it from `Codec_I2C_IsBusBusy`, keeping the raw bit
 * knowledge in the Codec. The helper reads `SR2` only for this selector,
 * because RM0008's ordered read-`SR1`-then-read-`SR2` pair also releases a
 * latched `ADDR` and clock stretching, so the `EV6` clear stays an explicit,
 * visible transaction step at the call site instead of becoming a speculative
 * side effect inside the generic wait.
 * @def I2C_EVENT_FLAG_BUS_IDLE
 */
#define I2C_EVENT_FLAG_BUS_IDLE						((i2c_event_flag_t) 0x0002U)

/** @} */ // I2C_02_DataTypes

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_DATA_TYPES_H_ */
