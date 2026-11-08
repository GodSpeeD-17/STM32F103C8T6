/**
 * @file i2c_defines.h
 * @author Shrey Shah
 * @brief I2C Driver Public Selectors and Validation Macros
 * @version v1.0
 * @date 2026-08-28
 * @details
 * This file contains public selector values and pure validation macros
 * for the I2C Driver. Every typedef alias lives in `i2c_data_types.h`;
 * this file holds only the value macros of those types (beside their
 * validators) and the validated configuration-structure group. These are
 * used by the application layer and internally by the Driver for
 * validation.
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
 * transfer APIs and validates the status event selectors the Driver may wait
 * for. Reserved addresses are excluded at the Driver boundary so a transfer
 * request can never silently address a reserved device or another controller.
 * The event selectors use an application-domain vocabulary whose individual
 * values each own a unique bit, so equality dispatch in the Driver
 * choreography can never cross-register; the Selector Codec keeps the raw
 * register masks while the Driver names exactly which hardware event a step
 * is waiting for. @ref `I2C_EVENT_FLAG_BUS_IDLE` names an `SR2` condition
 * rather than an `SR1` mask and is answered only through `SR2`.
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
 * @brief START or repeated START was generated on the bus (`I2Cx_SR1.SB`)
 * @details
 * Set by hardware once the peripheral has driven a START or repeated START
 * onto the bus. A master clears it by reading `SR1` and then writing the
 * target address byte to `DR`, so the flag both announces the event and
 * releases the address phase; clearing `PE` clears it as well. The Selector
 * Codec translates this selector to the raw `SR1.SB` flag mask.
 * @def I2C_EVENT_FLAG_START_GENERATED
 */
#define I2C_EVENT_FLAG_START_GENERATED				((i2c_event_flag_t) 0x0001U)

/**
 * @brief Target address was sent and acknowledged (`I2Cx_SR1.ADDR`)
 * @details
 * Set by hardware after the address byte has shifted out and the target has
 * acknowledged it, which is the point where the peripheral may stretch `SCL`.
 * A master clears it with the ordered `SR1` then `SR2` read pair, so the flag
 * bounds the address phase instead of being cleared speculatively. The
 * Driver's wait helper answers this selector as one coherent
 * master-transmitter step: the pair inside the wait performs the mandatory
 * release and the same `SR2` sample proves master mode. The receive-path
 * `EV6_3` sequence that must interpose `CR1` action writes between the two
 * reads cannot reuse the selector. The Selector Codec translates this selector
 * to the raw `SR1.ADDR` flag mask; its application-domain value stays unique
 * beside every other selector, including the `SR2` bus-condition selector that
 * once collided with it when the vocabulary mirrored register layouts.
 * @def I2C_EVENT_FLAG_ADDRESS_SENT
 */
#define I2C_EVENT_FLAG_ADDRESS_SENT					((i2c_event_flag_t) 0x0002U)

/**
 * @brief Data register and shift register are both settled (`I2Cx_SR1.BTF`)
 * @details
 * Set once a byte has moved between `DR` and the shift register and the next
 * transfer step would need software service. In transmit it becomes set only
 * after the final byte leaves `DR` and drains from the shift register, so it
 * is the safe point to request STOP without truncating the last byte. The
 * Selector Codec translates this selector to the raw `SR1.BTF` flag mask.
 * @def I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED
 */
#define I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED		((i2c_event_flag_t) 0x0004U)

/**
 * @brief Transmit data register is empty (`I2Cx_SR1.TXE`)
 * @details
 * Set while `DR` can accept the next byte. It is cleared by the `DR` write
 * that fills it, by a START or STOP request, or by clearing `PE`, so it is the
 * pacing condition for every byte of a transmit payload. The application
 * vocabulary is decoupled from the `SR1` layout: the Selector Codec translates
 * this selector to the raw `TXE` flag mask, so the application value never has
 * to sit on the register bit position.
 * @def I2C_EVENT_FLAG_DATA_REGISTER_EMPTY
 */
#define I2C_EVENT_FLAG_DATA_REGISTER_EMPTY			((i2c_event_flag_t) 0x0008U)

/**
 * @brief Transmit tail is complete: `TXE` and `BTF` are set together
 * @details
 * Composite selector matching RM0008's EV8_2. Requiring both flags at once
 * means the caller waits for the last byte to leave the shift register instead
 * of treating an empty `DR` alone as completion, which would let a STOP
 * request truncate the final byte. The Selector Codec translates each
 * selected individual flag and demands both at once.
 * @def I2C_EVENT_FLAG_TRANSFER_COMPLETE
 */
#define I2C_EVENT_FLAG_TRANSFER_COMPLETE			((i2c_event_flag_t) (I2C_EVENT_FLAG_DATA_REGISTER_EMPTY | I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED))

/**
 * @brief Peripherals released the bus (`I2Cx_SR2.BUSY` observed clear)
 * @details
 * Names the `SR2` bus condition, because bus ownership has no `SR1` flag at
 * all. The application vocabulary deliberately departs from the `SR2` register
 * layout: `SR2.BUSY` and `SR1.ADDR` occupy the same raw bit position (each is
 * its register's bit 1, both raw masks 0x0002), so selectors mirroring
 * registers would make the two application events collide into one value.
 * The Selector Codec routes this selector to `Codec_I2C_IsBusBusy` against an
 * `SR2` image and never to a raw `SR1` bit test. The Driver's bus-idle wait
 * helper branches on this selector explicitly and reads `SR2` for it every
 * iteration; no individual application selector shares this selector's bit,
 * so a choreography equality test can never dispatch to the wrong register.
 * @def I2C_EVENT_FLAG_BUS_IDLE
 */
#define I2C_EVENT_FLAG_BUS_IDLE						((i2c_event_flag_t) 0x0010U)

/**
 * @def I2C_IS_VALID_EVENT_FLAG
 * @brief Validates a status event-flag selector
 * @details
 * Admits exactly the published selectors, including the composite
 * @ref `I2C_EVENT_FLAG_TRANSFER_COMPLETE` mask that the transmit tail needs so
 * a partially satisfied completion request cannot be mistaken for an unknown
 * selector, and the @ref `I2C_EVENT_FLAG_BUS_IDLE` selector that names the
 * `SR2` bus condition. Every individual selector owns a unique
 * application-domain bit, so an equality test against one selector is always
 * exclusive of every other one; which register a selector belongs to is
 * decided by the Selector Codec, not by the selector's value.
 * @param[in] eventFlag Event-flag selector to validate
 * @returns I2C Event-Flag Validation Result
 * @retval - `0x00U`: Selector is not an admitted selector
 * @retval - `0x01U`: Selector is an admitted selector
 */
#define I2C_IS_VALID_EVENT_FLAG(eventFlag)						\
(																\
	((eventFlag) == I2C_EVENT_FLAG_START_GENERATED) ||			\
	((eventFlag) == I2C_EVENT_FLAG_ADDRESS_SENT) ||				\
	((eventFlag) == I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED) ||	\
	((eventFlag) == I2C_EVENT_FLAG_DATA_REGISTER_EMPTY) ||		\
	((eventFlag) == I2C_EVENT_FLAG_TRANSFER_COMPLETE) ||		\
	((eventFlag) == I2C_EVENT_FLAG_BUS_IDLE)					\
)

/** @} */ // I2C_02_Defines_04_Transfers

/**
 * @defgroup I2C_02_Defines_05_TransferErrors I2C Transfer Error Codes
 * @ingroup I2C_02_Defines
 * @brief Primary transfer error selectors
 * @details
 * Admitted values of @ref `i2c_error_t`, the primary error cause reported by a
 * failed transfer. The Driver selects one primary error per failure: hardware
 * `SR1` flag conditions are decoded by the error Codec from the sampled image, and
 * the software-only conditions (poll-budget timeout, stuck `BUSY`, failed
 * recovery) are decided by the Driver's wait and recovery choreography.
 * Value macros live in this file beside their siblings per the repository
 * convention, while the @ref `i2c_error_t` typedef stays in
 * `i2c_data_types.h` as the common type alias.
 * @{
 */

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

/** @} */ // I2C_02_Defines_05_TransferErrors

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
