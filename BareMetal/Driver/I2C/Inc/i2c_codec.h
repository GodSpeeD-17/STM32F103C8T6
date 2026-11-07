/**
 * @file i2c_codec.h
 * @author Shrey Shah
 * @brief I2C Codec Layer - Pure Register-Image Transformations
 * @version v1.0
 * @date 2026-08-28
 * @details
 * Pure transformation layer between the I2C Driver and the I2C LL. Owns
 * selector-to-field encoding, address-byte construction, timing calculation,
 * error extraction, and CR1 action staging over caller-owned reg images.
 * No MMIO, polling, topology, clock, reset, NVIC, or sequencing work.
 */

// Header Guards
#ifndef I2C_CODEC_H_
#define I2C_CODEC_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx.h"
#include "i2c_data_types.h"
#include "i2c_defines.h"

// C++ Compatibility
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup I2C_04_Codec I2C Codec APIs
 * @ingroup I2C
 * @brief Pure I2C register-image transformations
 * @{
 */

// ==================================================================================================== //
// I2C Operation State Codec
// ==================================================================================================== //

/**
 * @brief Extracts the I2C peripheral enable state from a CR1 image
 * @param[in] cr1RegImage I2C `CR1` register image
 * Expected values:
 * - Any valid `I2Cx_CR1` image captured by the Driver
 * @returns Extracted I2C operation state
 * @retval - @ref `DRIVER_STATUS_ON`: I2C Peripheral Disabled
 * @retval - @ref `DRIVER_STATUS_OFF`: I2C Peripheral Enabled
 * @note This Codec performs no MMIO and does not validate the instance
 */
driver_status_t Codec_I2C_ExtractOperationState(const reg cr1RegImage);

/**
 * @brief Stages the I2C peripheral enable state into a CR1 image
 * @param[in,out] pCr1RegImage Caller-owned `CR1` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in] operationState Requested I2C operation state
 * Accepted values:
 * - @ref `DRIVER_STATUS_OFF`: Clear `I2Cx_CR1.PE`
 * - @ref `DRIVER_STATUS_ON`: Set `I2Cx_CR1.PE`
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Operation state was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pCr1RegImage` was `NULL`
 * @note Unrelated `CR1` bits are preserved
 */
driver_status_t Codec_I2C_StageOperationState(reg* const pCr1RegImage, const driver_status_t operationState);

// ==================================================================================================== //
// I2C Timing Codec
// ==================================================================================================== //

/**
 * @brief Stages I2C timing (`CR2.FREQ`, `CCR`, `TRISE`) from a config and APB1 frequency
 * @details
 * Computes the peripheral input clock in MHz, the SCL clock-control divider
 * using ceiling division, and the maximum rise-time count, then stages all
 * three into the caller-owned register images while preserving unrelated bits.
 * @param[in,out] pCr2RegImage Caller-owned `CR2` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in,out] pCcrRegImage Caller-owned `CCR` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in,out] pTriseRegImage Caller-owned `TRISE` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in] pConfig I2C timing configuration
 * Expected values:
 * - Non-`NULL`: `bus_speed` and `fast_mode_duty` are admitted selectors
 * @param[in] apb1FrequencyHz Live APB1 frequency in hertz
 * Accepted values:
 * - Integer megahertz value in the admitted range for the selected mode
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Timing was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: A required pointer was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: A selector, frequency, or divider was invalid
 * @note Unrelated bits in all three images are preserved
 */
driver_status_t Codec_I2C_StageTiming
(
	reg* const						pCr2RegImage,
	reg* const						pCcrRegImage,
	reg* const						pTriseRegImage,
	const i2c_config_t* const		pConfig,
	const frequency_t				apb1FrequencyHz
);

// ==================================================================================================== //
// I2C CR1 Baseline Codec
// ==================================================================================================== //

/**
 * @brief Stages the master-I2C baseline into a CR1 image
 * @details
 * Clears `SMBUS`, sets `ACK`, clears `POS`, and applies the clock-stretching
 * control, preserving all other `CR1` bits.
 * @param[in,out] pCr1RegImage Caller-owned `CR1` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in] disableClockStretching Clock-stretching control
 * Accepted values:
 * - `0`: Enable clock stretching (clear `NOSTRETCH`)
 * - Nonzero: Disable clock stretching (set `NOSTRETCH`)
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Baseline was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pCr1RegImage` was `NULL`
 * @note Unrelated `CR1` bits are preserved
 */
driver_status_t Codec_I2C_StageBaseline(reg* const pCr1RegImage, const uint8_t disableClockStretching);

// ==================================================================================================== //
// I2C Address Codec
// ==================================================================================================== //

/**
 * @brief Encodes a 7-bit address into an I2C address byte
 * @param[in] address Unshifted 7-bit I2C target address
 * Accepted values:
 * - `0x00U` through `0x7FU`: Any 7-bit value (reserved filtering is a Driver concern)
 * @param[in] isRead Read/write selector
 * Accepted values:
 * - `0`: Write transfer (R/W bit cleared)
 * - Nonzero: Read transfer (R/W bit set)
 * @returns Encoded address byte `(address << 1) | R/W`
 * @retval - `0x00U` through `0xFFU`: Encoded address byte
 * @note Reserved-address rejection is a Driver validation concern, not a Codec concern
 */
uint8_t Codec_I2C_EncodeAddressByte(const i2c_address_t address, const uint8_t isRead);

// ==================================================================================================== //
// I2C CR1 Action Codec
// ==================================================================================================== //

/**
 * @brief Stages the START condition request into a CR1 image
 * @param[in,out] pCr1RegImage Caller-owned `CR1` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: START was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pCr1RegImage` was `NULL`
 * @note Sets `I2Cx_CR1.START`; unrelated bits are preserved
 */
driver_status_t Codec_I2C_StageStart(reg* const pCr1RegImage);

/**
 * @brief Stages the STOP condition request into a CR1 image
 * @param[in,out] pCr1RegImage Caller-owned `CR1` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: STOP was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pCr1RegImage` was `NULL`
 * @note Sets `I2Cx_CR1.STOP`; unrelated bits are preserved
 */
driver_status_t Codec_I2C_StageStop(reg* const pCr1RegImage);

/**
 * @brief Stages the ACK bit into a CR1 image
 * @param[in,out] pCr1RegImage Caller-owned `CR1` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in] ack Acknowledge control
 * Accepted values:
 * - `0`: Clear `I2Cx_CR1.ACK` (NACK)
 * - Nonzero: Set `I2Cx_CR1.ACK`
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: ACK was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pCr1RegImage` was `NULL`
 * @note Unrelated `CR1` bits are preserved
 */
driver_status_t Codec_I2C_StageAck(reg* const pCr1RegImage, const uint8_t ack);

/**
 * @brief Stages the POS bit into a CR1 image
 * @param[in,out] pCr1RegImage Caller-owned `CR1` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in] pos POS control
 * Accepted values:
 * - `0`: Clear `I2Cx_CR1.POS`
 * - Nonzero: Set `I2Cx_CR1.POS`
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: POS was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pCr1RegImage` was `NULL`
 * @note Unrelated `CR1` bits are preserved
 */
driver_status_t Codec_I2C_StagePos(reg* const pCr1RegImage, const uint8_t pos);

/**
 * @brief Stages the software-reset bit into a CR1 image
 * @param[in,out] pCr1RegImage Caller-owned `CR1` register image
 * Expected values:
 * - Non-`NULL`: Image to update in place
 * @param[in] reset Software-reset control
 * Accepted values:
 * - `0`: Clear `I2Cx_CR1.SWRST`
 * - Nonzero: Set `I2Cx_CR1.SWRST`
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: SWRST was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pCr1RegImage` was `NULL`
 * @note Unrelated `CR1` bits are preserved
 */
driver_status_t Codec_I2C_StageSoftwareReset(reg* const pCr1RegImage, const uint8_t reset);

/**
 * @brief Validates that no START or STOP action is still pending in a CR1 image
 * @details
 * Tests `I2Cx_CR1.START` and `I2Cx_CR1.STOP` without exposing the raw register
 * bits to the Driver. Both bits are write-only action requests that self-clear
 * once the peripheral retires the sequence they describe, so a set bit means an
 * action is still in progress and the timing registers must not be retimed.
 * @param[in] cr1RegImage I2C `CR1` register image
 * Expected values:
 * - Any valid `I2Cx_CR1` image captured by the Driver
 * @returns @ref driver_status_t "CR1 action-idle validation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Neither START nor STOP is pending
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: START or STOP is still pending
 * @note This Codec performs no MMIO and does not validate the instance
 */
driver_status_t Codec_I2C_ValidateActionIdle(const reg cr1RegImage);

// ==================================================================================================== //
// I2C SR1 Error Codec
// ==================================================================================================== //

/**
 * @brief Extracts the primary error from an SR1 image
 * @details
 * Inspects the `SR1` error flags in priority order and reports a single
 * primary @ref i2c_error_t. The Driver decides address-versus-data NACK
 * context; this Codec reports `AF` as @ref `I2C_ERROR_DATA_NACK`.
 * @param[in] sr1RegImage I2C `SR1` register image
 * Expected values:
 * - Any valid `I2Cx_SR1` image captured by the Driver
 * @param[out] pError Extracted primary error
 * Expected values:
 * - Non-`NULL`: Receives @ref `I2C_ERROR_NONE` when no error flag is set
 * @returns @ref driver_status_t "Codec extraction status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Error was extracted
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pError` was `NULL`
 */
driver_status_t Codec_I2C_ExtractError(const reg sr1RegImage, i2c_error_t* const pError);

/**
 * @brief Stages a write-zero-to-clear error image from an SR1 snapshot
 * @details
 * Clears only the selected error bits by writing `0` to them while
 * preserving all other bits from the snapshot (W0C semantics).
 * @param[out] pWriteImage Caller-owned output image (value to write)
 * Expected values:
 * - Non-`NULL`: Receives the staged clear image
 * @param[in] sr1Snapshot `SR1` snapshot to derive the clear image from
 * Expected values:
 * - Any valid `I2Cx_SR1` image captured by the Driver
 * @param[in] errorsToClear Mask of error bits to clear
 * Accepted values:
 * - Any combination of `I2C_SR1_*` error flag masks
 * @returns @ref driver_status_t "Codec staging status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Clear image was staged
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pWriteImage` was `NULL`
 * @note Unrelated bits are preserved from the snapshot
 */
driver_status_t Codec_I2C_StageClearErrors(reg* const pWriteImage, const reg sr1Snapshot, const reg errorsToClear);

// ==================================================================================================== //
// I2C SR1/SR2 Status Flag Codec
// ==================================================================================================== //

/**
 * @brief Reports whether the START condition has been generated (SB set)
 * @details
 * Tests `I2Cx_SR1.SB` without exposing the raw register bit to callers.
 * Returns @ref `DRIVER_STATUS_SUCCESS` when SB is set and
 * @ref `DRIVER_STATUS_ERROR` otherwise. The Driver uses this to
 * boundedly poll for START generation completion.
 * @param[in] sr1RegImage I2C `SR1` register image
 * Expected values:
 * - Any valid `I2Cx_SR1` image captured by the Driver
 * @returns @ref driver_status_t "START-condition status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: SB is set (START generated)
 * @retval - @ref `DRIVER_STATUS_ERROR`: SB is clear
 * @note This Codec performs no MMIO and does not validate the instance
 */
driver_status_t Codec_I2C_IsStartCondition(const reg sr1RegImage);

/**
 * @brief Reports whether the I2C bus is busy (BUSY set)
 * @details
 * Tests `I2Cx_SR2.BUSY` without exposing the raw register bit to callers.
 * Returns @ref `DRIVER_STATUS_SUCCESS` when BUSY is set and
 * @ref `DRIVER_STATUS_ERROR` otherwise. The Driver uses this to
 * boundedly poll for bus idle after a STOP condition.
 * @param[in] sr2RegImage I2C `SR2` register image
 * Expected values:
 * - Any valid `I2Cx_SR2` image captured by the Driver
 * @returns @ref driver_status_t "Bus-busy status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: BUSY is set (bus busy)
 * @retval - @ref `DRIVER_STATUS_ERROR`: BUSY is clear (bus idle)
 * @note This Codec performs no MMIO and does not validate the instance
 */
driver_status_t Codec_I2C_IsBusBusy(const reg sr2RegImage);

/** @} */ // I2C_04_Codec

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_CODEC_H_ */
