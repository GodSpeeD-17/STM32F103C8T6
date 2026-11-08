/**
 * @file i2c.c
 * @author Shrey Shah
 * @brief I2C Driver Implementation
 * @version v1.0
 * @date 28-08-2026
 * @details
 * Implements the contract-frozen I2C Driver surface for STM32F103C8T6
 * I2C1/I2C2.
 */

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx.h"
#include "stm32f1xx_rcc.h"
#include "i2c.h"
#include "i2c_codec.h"
#include "i2c_ll.h"
#include "rcc.h"

/** @brief Poll budget per millisecond for I2C driver operations @def I2C_DRIVER_POLL_BUDGET_PER_MS */
#define I2C_DRIVER_POLL_BUDGET_PER_MS		((uint32_t) 100UL)
/** @brief Slot 0 of a two-slot register image array, holding the pre-transaction hardware snapshot @def I2C_DRIVER_REG_IMAGE_CURRENT */
#define I2C_DRIVER_REG_IMAGE_CURRENT		((uint8_t) 0U)
/** @brief Slot 1 of a two-slot register image array, holding the staged post-transaction value @def I2C_DRIVER_REG_IMAGE_TARGET */
#define I2C_DRIVER_REG_IMAGE_TARGET			((uint8_t) 1U)

// ==================================================================================================== //
// I2C Driver Topology Helpers
// ==================================================================================================== //

/**
 * @brief Gets the APB1 clock enable mask for an I2C peripheral
 * @details
 * Returns the appropriate RCC clock enable bit mask for the given I2C peripheral.
 * This function maps I2C1 and I2C2 to their respective RCC enable bits.
 * @param[in] I2Cx I2C peripheral instance
 * Expected Values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @returns RCC clock enable mask for the specified I2C peripheral
 * @note - Does not validate if input is NULL pointer because it is static funtion
 * @note - Returns `0x00UL` in case @p `I2Cx` is not within expected range
 */
__STATIC reg _I2C_GetAPB1ClockEnableMask(const I2C_TypeDef* const I2Cx)
{
	reg clockMask = 0x00000000UL;

	switch ((uintptr_t) I2Cx)
	{
		case (uintptr_t) I2C1:
		{
			clockMask = RCC_APB1ENR_I2C1EN;
			break;
		}
		case (uintptr_t) I2C2:
		{
			clockMask = RCC_APB1ENR_I2C2EN;
			break;
		}
		default:
		{
			clockMask = 0x00000000UL;
			break;
		}
	}

	return clockMask;
}

/**
 * @brief Gets the APB1 peripheral reset mask for an I2C peripheral
 * @details
 * Returns the appropriate RCC peripheral reset bit mask for the given I2C peripheral.
 * This function maps I2C1 and I2C2 to their respective RCC reset bits.
 * @param[in] I2Cx I2C peripheral instance
 * Expected Values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @returns RCC peripheral reset mask for the specified I2C peripheral
 * @note - Does not validate if input is NULL pointer because it is static funtion
 * @note - Returns `0x00UL` in case @p `I2Cx` is not within expected range
 */
__STATIC reg _I2C_GetAPB1PeripheralResetMask(const I2C_TypeDef* const I2Cx)
{
	reg resetMask = 0x00000000UL;

	switch ((uintptr_t) I2Cx)
	{
		case (uintptr_t) I2C1:
		{
			resetMask = RCC_APB1RSTR_I2C1RST;
			break;
		}
		case (uintptr_t) I2C2:
		{
			resetMask = RCC_APB1RSTR_I2C2RST;
			break;
		}
		default:
		{
			resetMask = 0x00000000UL;
			break;
		}
	}

	return resetMask;
}

// ==================================================================================================== //
// I2C Transaction Helpers
// ==================================================================================================== //

/**
 * @brief Waits for one bounded status event inside the shared poll budget
 * @details
 * Samples `SR1` once per iteration and decodes the error flags before testing
 * @p `eventFlag`, so a hardware error ends the wait immediately instead of
 * being reported later as a plain timeout. Every sample consumes one poll from
 * @p `pPollBudget`, which the caller owns for the complete transaction; the
 * loop therefore cannot outlive the single timeout the public API promised.
 * A composite selector such as @ref `I2C_EVENT_FLAG_TRANSFER_COMPLETE` only
 * succeeds once every selected flag is set together.
 * The `SR2` sample is gated by one flat condition on the two selectors that
 * name an `SR2` condition: @ref `I2C_EVENT_FLAG_BUS_IDLE` needs it every
 * iteration because bus ownership has no `SR1` flag, and
 * @ref `I2C_EVENT_FLAG_ADDRESS_SENT` needs it because RM0008's ordered
 * read-`SR1`-then-read-`SR2` pair is what releases `ADDR` and ends `SCL`
 * stretching. Every other selector stays `SR1`-only, since a speculative
 * `SR2` sample could clear an `ADDR` that its owning transaction step has not
 * reached yet; for the two admitted selectors the pair-clear is harmless or
 * exactly the required action.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in] eventFlag Status event selector to wait for
 * Accepted values:
 * - Any selector admitted by @ref `I2C_IS_VALID_EVENT_FLAG`, including the
 *   @ref `I2C_EVENT_FLAG_BUS_IDLE` selector naming the `SR2` bus condition
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Decremented once per `SR1` sample
 * @param[out] pError Optional caller-owned report of the latched `SR1` error flag
 * Expected values:
 * - Any error report owned by the caller:
 *   - On a hardware failure @ref `DRIVER_STATUS_ERROR_FAIL`: The latched error selector
 *   - On the error-decode failure exit: @ref `I2C_ERROR_NONE`
 *   - `NULL`: The report is skipped
 *   - Any other exit leaves the caller's value untouched
 * @returns @ref driver_status_t "Event-wait status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The selected event was observed
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `eventFlag` is not an admitted selector
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: An `SR1` error flag was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: @p `pPollBudget` was exhausted
 * @note The error flag behind an @ref `DRIVER_STATUS_ERROR_FAIL` result is left
 * latched and reported through @p `pError` when provided, because the caller
 * still has to preserve and acknowledge it
 * @note The @ref `I2C_EVENT_FLAG_ADDRESS_SENT` selector expresses the
 * master-transmitter `EV6` only: the wait releases `ADDR` by the ordered pair
 * and proves master mode from that same `SR2` sample. The receive-path
 * `EV6_3` transaction that must interpose `CR1` action writes between the two
 * reads cannot reuse this selector and will get its own step when receive is
 * admitted
 * @pre The peripheral is enabled and the caller holds the instance exclusively
 */
__STATIC_FORCEINLINE driver_status_t _I2C_WaitForEvent
(
	I2C_TypeDef* const			I2Cx,
	const i2c_event_flag_t		eventFlag,
	uint32_t* const				pPollBudget,
	i2c_error_t* const			pError
)
{
	// Local Variables
	reg sr1RegImage = 0x00000000UL;
	reg sr2RegImage = 0x00000000UL;
	i2c_error_t sampledError = I2C_ERROR_NONE;
	driver_status_t sampleStatus = DRIVER_STATUS_ERROR;

	//! A missing caller budget would turn every wait into an unbounded loop, so it
	//! is refused before the first status sample is taken.
	if (pPollBudget == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Bounded Loop
	while (*pPollBudget > 0U)
	{
		//! One sample per iteration: the same `SR1` image answers both the error
		//! question and the event question, so no flag can appear between them.
		sr1RegImage = LL_I2C_ReadSR1(I2Cx);
		(*pPollBudget)--;

		//! Errors are decoded first because a NACK or bus error also leaves the
		//! awaited event clear; reporting the timeout instead would hide the cause.
		sampleStatus = Codec_I2C_ExtractError(sr1RegImage, &sampledError);
		if (sampleStatus != DRIVER_STATUS_SUCCESS)
		{
			//! A Codec-contract decode violation sampled no error flag, so the
			//! definite NONE value is what reaches the caller's report here.
			if (pError != NULL)
			{
				*pError = sampledError;
			}
			return sampleStatus;
		}
		if (sampledError != I2C_ERROR_NONE)
		{
			//! This is the one exit that names a cause: the latched selector is
			//! reported before the flag is handed to cleanup for acknowledgement.
			if (pError != NULL)
			{
				*pError = sampledError;
			}
			return DRIVER_STATUS_ERROR_FAIL;
		}

		//! Deal with SR1 first, then read SR2 if the selector names an SR2
		//! condition: bus ownership has no SR1 flag and the ADDRESS_SENT
		//! release needs the ordered pair, so these two are the only selectors
		//! that earn the sample; everything else must not speculatively take it.
		if ((eventFlag == I2C_EVENT_FLAG_ADDRESS_SENT) || (eventFlag == I2C_EVENT_FLAG_BUS_IDLE))
		{
			sr2RegImage = LL_I2C_ReadSR2(I2Cx);
		}

		//! The Codec reports "busy" as success, so the idle answer is the error
		//! value and every other outcome keeps polling.
		if (eventFlag == I2C_EVENT_FLAG_BUS_IDLE)
		{
			if (Codec_I2C_IsBusBusy(sr2RegImage) == DRIVER_STATUS_ERROR)
			{
				return DRIVER_STATUS_SUCCESS;
			}
			continue;
		}

		sampleStatus = Codec_I2C_IsEventPending(sr1RegImage, eventFlag);
		if (sampleStatus == DRIVER_STATUS_SUCCESS)
		{
			//! EV6: the `SR1` read above is the first half of the mandatory ordered
			//! pair, so the `SR2` sample already taken in this iteration is what
			//! releases `ADDR` and ends `SCL` stretching; its master bit also
			//! proves the peripheral still owns the bus before the transmit
			//! payload phase starts.
			if (eventFlag == I2C_EVENT_FLAG_ADDRESS_SENT)
			{
				if (Codec_I2C_IsMasterMode(sr2RegImage) != DRIVER_STATUS_SUCCESS)
				{
					return DRIVER_STATUS_ERROR_FAIL;
				}
			}
			return DRIVER_STATUS_SUCCESS;
		}
		//! Only the plain "not pending yet" answer may keep polling: a rejected
		//! selector is a programming error and must not burn the whole budget.
		if (sampleStatus != DRIVER_STATUS_ERROR)
		{
			return sampleStatus;
		}
	}

	return DRIVER_STATUS_ERROR_TIMEOUT;
}

/**
 * @brief Requests a START condition and waits for the SB event
 * @details
 * Reads `CR1`, stages `START`, and writes the image unconditionally, because
 * `START` is an action request whose equal-value write still matters and must
 * never be suppressed by a compare-and-write optimisation. The completion wait
 * samples `SR1` for `SB`, which is the first half of RM0008's EV5 sequence, so
 * the caller's following `DR` write is what completes the event.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Decremented once per `SR1` sample
 * @returns @ref driver_status_t "START generation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: START was requested and `SB` was observed
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: An `SR1` error flag was observed before `SB`
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: @p `pPollBudget` was exhausted
 * @note @ref `DRIVER_STATUS_ERROR_INVALID_ARG` cannot occur here because this
 * helper always waits for the fixed @ref `I2C_EVENT_FLAG_START_GENERATED` selector
 * @pre The peripheral is enabled and neither `START` nor `STOP` is still pending
 */
__STATIC_FORCEINLINE driver_status_t _I2C_GenerateStart(I2C_TypeDef* const I2Cx, uint32_t* const pPollBudget)
{
	// Local Variable
	reg cr1RegImage = 0x00000000UL;

	//! Stage on the caller-owned image first: an action request is only published
	//! once the whole image is known to be valid.
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);
	ASSERT_DRIVER_STATUS(Codec_I2C_StageStart(&cr1RegImage));

	//! Write unconditionally so the request reaches the peripheral even when the
	//! staged image matches the snapshot of an already-requested START.
	LL_I2C_WriteCR1(I2Cx, cr1RegImage);

	//! The latched error behind any SB-wait failure is recovered by the caller's
	//! cleanup path, which re-snapshots `SR1` itself, so the optional report is
	//! deliberately not consumed here.
	return _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_START_GENERATED, pPollBudget, NULL);
}

/**
 * @brief Requests a STOP condition and waits until the bus is released
 * @details
 * Reads `CR1`, stages `STOP`, and writes the image unconditionally, so the
 * equal-value write of an already-pending request is never suppressed. `STOP`
 * is only a request: hardware retires it at the next byte boundary and clears
 * `CR1.STOP` itself, so this helper closes the request through the same
 * bounded wait helper the START path uses, selecting the `SR2`
 * @ref `I2C_EVENT_FLAG_BUS_IDLE` condition, and returns success only after
 * the lines are observed free.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Decremented once per `SR2` sample
 * @returns @ref driver_status_t "STOP request status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: STOP was requested and the bus was observed idle
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: An `SR1` error flag was observed during the idle wait
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: `BUSY` stayed set until @p `pPollBudget` was exhausted
 * @note No failure other than these is reachable, because this helper owns the
 * register image it passes on and always waits on the fixed idle condition
 * @pre The peripheral is enabled, still master, and at a byte boundary
 */
__STATIC_FORCEINLINE driver_status_t _I2C_GenerateStop(I2C_TypeDef* const I2Cx, uint32_t* const pPollBudget)
{
	// Local Variable
	reg cr1RegImage = 0x00000000UL;

	//! `STOP` is an action bit that hardware clears once the condition is on the
	//! wire, so this write is never skipped even when the staged image equals the
	//! snapshot taken just before it.
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);
	ASSERT_DRIVER_STATUS(Codec_I2C_StageStop(&cr1RegImage));
	LL_I2C_WriteCR1(I2Cx, cr1RegImage);

	//! The request only releases the bus at the next byte boundary, so the bounded
	//! wait is what proves completion; routing it through the shared helper keeps
	//! every caller's "STOP succeeded" answer backed by the same idle evidence.
	//! The optional error report is not consumed here for the same reason as the
	//! START wait: the cleanup path re-snapshots `SR1` and never needs plumbing.
	return _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_BUS_IDLE, pPollBudget, NULL);
}

/**
 * @brief Preserves the primary fault and returns the peripheral to idle
 * @details
 * The single cleanup path for every failure after START was requested. It
 * snapshots `SR1` first, because acknowledging a flag would erase the only
 * record of why the transaction stopped. The snapshot is decoded into the
 * primary error, an abort STOP is requested only when the hardware can still
 * legally accept one, and the observed error flags are then cleared with a
 * semantic write-zero-to-clear image. Every recovery step is best effort:
 * @p `primaryStatus` is returned unchanged so a cleanup side effect can never
 * turn a failed transfer into a reported success or replace its cause.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in] primaryStatus Failure status of the transaction step that stopped the transfer
 * Accepted values:
 * - @ref `DRIVER_STATUS_ERROR_FAIL`: A hardware error or protocol violation was observed
 * - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: The shared poll budget was exhausted
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Consumed by the optional abort-STOP bus-idle wait
 * @returns @ref driver_status_t "Preserved primary transaction failure"
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: @p `primaryStatus` was a protocol failure
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: @p `primaryStatus` was a budget exhaustion
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @note Arbitration loss is never followed by an abort STOP, because hardware
 * has already released the lines and cleared the master state
 * @note This helper never requests a software reset; CR1.SWRST recovery is a
 * separate admission decision and remains out of scope for this transaction
 * @pre The caller has already requested START at least once
 */
__STATIC driver_status_t _I2C_AcknowledgeErrors
(
	I2C_TypeDef* const		I2Cx,
	const driver_status_t	primaryStatus,
	uint32_t* const			pPollBudget
)
{
	// Local Variables
	reg sr1Snapshot = 0x00000000UL;
	reg sr1WriteImage = 0x00000000UL;
	i2c_error_t latchedError = I2C_ERROR_NONE;

	//! A missing budget would make the abort-STOP wait unbounded, so the caller's
	//! contract violation is reported instead of guessed around.
	if (pPollBudget == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Snapshot before any acknowledgement: `SR1` is a live status word, so the
	//! flag that caused this failure would disappear the moment it is cleared.
	sr1Snapshot = LL_I2C_ReadSR1(I2Cx);

	//! The destination is a local image, so this decode can only fail if the Codec
	//! contract changes. Treat that as "no primary error" instead of returning
	//! early and abandoning the acknowledgement and bus release below.
	if (Codec_I2C_ExtractError(sr1Snapshot, &latchedError) != DRIVER_STATUS_SUCCESS)
	{
		latchedError = I2C_ERROR_NONE;
	}

	//! An abort STOP is only legal while this peripheral still owns the bus and no
	//! earlier action is pending. Arbitration loss is excluded explicitly because
	//! hardware has already released SDA/SCL and returned to slave mode.
	if ((latchedError != I2C_ERROR_ARBITRATION_LOST) &&
		(Codec_I2C_ValidateActionIdle(LL_I2C_ReadCR1(I2Cx)) == DRIVER_STATUS_SUCCESS) &&
		(Codec_I2C_IsMasterMode(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS))
	{
		//! Recovery is best effort by contract: a stuck-busy result from the STOP
		//! helper is the case ES096 leaves to board-level recovery, so the whole
		//! result is discarded and can never replace the cause reported to the
		//! caller.
		(void) _I2C_GenerateStop(I2Cx, pPollBudget);
	}

	//! Acknowledge the observed error set with a semantic write-zero-to-clear image.
	//! The Codec owns which flags that set contains, so cleanup cannot clear an
	//! error it never inspected or leave the observed one latched.
	if (Codec_I2C_StageClearErrors(&sr1WriteImage, sr1Snapshot) == DRIVER_STATUS_SUCCESS)
	{
		LL_I2C_WriteSR1(I2Cx, sr1WriteImage);
	}

	return primaryStatus;
}

// ==================================================================================================== //
// I2C Root Configuration APIs
// ==================================================================================================== //

driver_status_t I2C_Init(I2C_TypeDef* const I2Cx, const i2c_config_t* const pConfig)
{
	// Local Variables
	reg clockEnableMask = 0x00000000UL;
	//! Each register image is a two-slot array: slot 0 keeps the pre-transaction
	//! hardware snapshot for comparison, slot 1 is the only image the Codec stages
	//! mutate, so a staged result can never overwrite the value it is diffed against.
	reg cr1[2] = {0x00000000UL, 0x00000000UL};
	reg cr2[2] = {0x00000000UL, 0x00000000UL};
	reg ccr[2] = {0x00000000UL, 0x00000000UL};
	reg trise[2] = {0x00000000UL, 0x00000000UL};
	frequency_t apb1BusFrequency = 0x00000000UL;

	//! Reject an unrepresentable request before the first hardware access.
	if ((I2Cx == NULL) || (pConfig == NULL))
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if ((I2C_IS_VALID_INSTANCE(I2Cx) == 0U) || (I2C_IS_VALID_CONFIG(pConfig) == 0U))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! The APB1 gate is application-owned, so observe it instead of enabling it: a
	//! successful return must never hide a missing application clock-enable step.
	clockEnableMask = _I2C_GetAPB1ClockEnableMask(I2Cx);
	if (clockEnableMask == 0x00000000UL)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}
	if (RCC_GetPeripheralClockState(RCC_APB1_BUS, clockEnableMask) != DRIVER_STATUS_ON)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! Snapshot CR1 once so the disable and pending-action checks below describe a
	//! single coherent hardware moment instead of two unrelated reads.
	cr1[I2C_DRIVER_REG_IMAGE_CURRENT] = LL_I2C_ReadCR1(I2Cx);

	//! PE must already be 0. Forcing it here would discard the caller's CR1 image
	//! and could abort an in-flight transfer, so the application owns the explicit
	//! I2C_SetOperationState(OFF) transition instead.
	if (Codec_I2C_ExtractOperationState(cr1[I2C_DRIVER_REG_IMAGE_CURRENT]) != DRIVER_STATUS_OFF)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! A pending START or STOP means the peripheral has not retired that action yet,
	//! so retiming CCR/TRISE now would corrupt the transfer it belongs to. The Codec
	//! owns the flag interpretation and reports it as a state failure.
	ASSERT_DRIVER_STATUS(Codec_I2C_ValidateActionIdle(cr1[I2C_DRIVER_REG_IMAGE_CURRENT]));

	//! A busy bus means another controller currently owns the lines, so no timing
	//! write may be attempted until it is released.
	if (Codec_I2C_IsBusBusy(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_ERROR_BUSY;
	}

	//! Resolve the live APB1 clock before staging, because CCR and TRISE are both
	//! derived from it and a zero snapshot cannot produce a legal timing image.
	apb1BusFrequency = RCC_GetBusFrequency(RCC_APB1_BUS);
	if (apb1BusFrequency == RCC_FREQ_ZERO)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! Snapshot the remaining timing registers once, only after every fallible
	//! precondition above has already passed.
	cr2[I2C_DRIVER_REG_IMAGE_CURRENT] = LL_I2C_ReadCR2(I2Cx);
	ccr[I2C_DRIVER_REG_IMAGE_CURRENT] = LL_I2C_ReadCCR(I2Cx);
	trise[I2C_DRIVER_REG_IMAGE_CURRENT] = LL_I2C_ReadTRISE(I2Cx);

	//! Seed each target slot with its own snapshot so the Codec stages only mutate
	//! the paired copy and no hardware image is touched until every fallible Codec
	//! transformation has succeeded.
	cr1[I2C_DRIVER_REG_IMAGE_TARGET] = cr1[I2C_DRIVER_REG_IMAGE_CURRENT];
	cr2[I2C_DRIVER_REG_IMAGE_TARGET] = cr2[I2C_DRIVER_REG_IMAGE_CURRENT];
	ccr[I2C_DRIVER_REG_IMAGE_TARGET] = ccr[I2C_DRIVER_REG_IMAGE_CURRENT];
	trise[I2C_DRIVER_REG_IMAGE_TARGET] = trise[I2C_DRIVER_REG_IMAGE_CURRENT];

	//! Stage Timing related registers (CR2, CCR, TRISE) based on the configuration and live APB1 clock
	ASSERT_DRIVER_STATUS
	(
		Codec_I2C_StageTiming
		(
			&cr2[I2C_DRIVER_REG_IMAGE_TARGET],
			&ccr[I2C_DRIVER_REG_IMAGE_TARGET],
			&trise[I2C_DRIVER_REG_IMAGE_TARGET],
			pConfig,
			apb1BusFrequency
		)
	);

	//! Stage the master-I2C baseline (SMBUS=0, ACK=1, POS=0, clock stretching enabled).
	ASSERT_DRIVER_STATUS(Codec_I2C_StageBaseline(&cr1[I2C_DRIVER_REG_IMAGE_TARGET], 0U));

	//! CR2 carries FREQ and must land before CCR/TRISE are retimed; CR1 is written
	//! last so PE stays 0 for the whole commit. Each write is skipped when the
	//! staged slot already equals its snapshot, so an unchanged re-initialization
	//! costs no redundant register traffic.
	if (cr2[I2C_DRIVER_REG_IMAGE_TARGET] != cr2[I2C_DRIVER_REG_IMAGE_CURRENT])
	{
		LL_I2C_WriteCR2(I2Cx, cr2[I2C_DRIVER_REG_IMAGE_TARGET]);
	}
	if (ccr[I2C_DRIVER_REG_IMAGE_TARGET] != ccr[I2C_DRIVER_REG_IMAGE_CURRENT])
	{
		LL_I2C_WriteCCR(I2Cx, ccr[I2C_DRIVER_REG_IMAGE_TARGET]);
	}
	if (trise[I2C_DRIVER_REG_IMAGE_TARGET] != trise[I2C_DRIVER_REG_IMAGE_CURRENT])
	{
		LL_I2C_WriteTRISE(I2Cx, trise[I2C_DRIVER_REG_IMAGE_TARGET]);
	}
	if (cr1[I2C_DRIVER_REG_IMAGE_TARGET] != cr1[I2C_DRIVER_REG_IMAGE_CURRENT])
	{
		LL_I2C_WriteCR1(I2Cx, cr1[I2C_DRIVER_REG_IMAGE_TARGET]);
	}

	//! Leave PE=0; enabling operation is a separate application decision.
	return DRIVER_STATUS_SUCCESS;
}

driver_status_t I2C_DeInit(I2C_TypeDef* const I2Cx)
{
	// Local Variables
	reg clockEnableMask = 0x00000000UL;
	reg resetMask = 0x00000000UL;

	//! Reject an unrepresentable instance before the first hardware or peer access.
	if (I2Cx == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if (I2C_IS_VALID_INSTANCE(I2Cx) == 0U)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! The APB1 gate is application-owned, so observe it instead of enabling it.
	//! Pulsing a reset behind a disabled gate would leave the register bank
	//! unreadable and would hide the missing application clock-enable step. The gate
	//! must also be proven on before SR2 is read, because a powered-down peripheral
	//! returns undefined data that could fake either a free or a busy bus.
	clockEnableMask = _I2C_GetAPB1ClockEnableMask(I2Cx);
	if (clockEnableMask == 0x00000000UL)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}
	if (RCC_GetPeripheralClockState(RCC_APB1_BUS, clockEnableMask) != DRIVER_STATUS_ON)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! A busy bus means another controller currently owns the lines, so resetting
	//! the controller now would abandon a transfer mid-byte instead of releasing it.
	if (Codec_I2C_IsBusBusy(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_ERROR_BUSY;
	}

	//! Decode the paired reset bit only after the gate is proven enabled, so a zero
	//! mask here describes unsupported topology rather than a missing clock.
	resetMask = _I2C_GetAPB1PeripheralResetMask(I2Cx);
	if (resetMask == 0x00000000UL)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! One RCC reset pulse restores the whole I2C register bank, so this transaction
	//! carries no I2C register image and needs no Codec transformation. The clock
	//! gate, GPIO/AFIO route, and NVIC delivery stay application-owned.
	ASSERT_DRIVER_STATUS(RCC_PulsePeripheralReset(RCC_APB1_BUS, resetMask));

	return DRIVER_STATUS_SUCCESS;
}

// ==================================================================================================== //
// I2C Operation State APIs
// ==================================================================================================== //

driver_status_t I2C_GetOperationState(I2C_TypeDef* const I2Cx)
{
	//! Validate Input
	if(I2Cx == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	//! Validate the range for input parameters
	if (I2C_IS_VALID_INSTANCE(I2Cx) == 0U)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! Extract the Operation State
	return Codec_I2C_ExtractOperationState(LL_I2C_ReadCR1(I2Cx));

}

driver_status_t I2C_SetOperationState(I2C_TypeDef* const I2Cx, const driver_status_t operationState)
{
	// Local Variable
	reg cr1 = 0x00UL;

	//! Validate Input
	if(I2Cx == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	//! Validate the range for input parameters
	if (I2C_IS_VALID_INSTANCE(I2Cx) == 0U)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}
	if ((operationState != DRIVER_STATUS_ON) && (operationState != DRIVER_STATUS_OFF))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! Extract the CR1 register value
	cr1 = LL_I2C_ReadCR1(I2Cx);
	//! Set or clear the PE bit based on the operation state
	ASSERT_DRIVER_STATUS(Codec_I2C_StageOperationState(&cr1, operationState));
	//! Write back to CR1 register
	LL_I2C_WriteCR1(I2Cx, cr1);

	return DRIVER_STATUS_SUCCESS;
}

// ==================================================================================================== //
// I2C Controller Transfer APIs
// ==================================================================================================== //

driver_status_t I2C_MasterTransmit
(
	I2C_TypeDef* const			I2Cx,
	const i2c_address_t			slaveAddress,
	const i2c_data_t* const		pData,
	const i2c_data_length_t		size,
	const uint32_t				timeout_ms
)
{
	// Local Variables
	reg clockEnableMask = 0x00000000UL;
	reg cr1RegImage = 0x00000000UL;
	reg sr1Snapshot = 0x00000000UL;
	i2c_error_t latchedError = I2C_ERROR_NONE;
	i2c_data_length_t byteIndex = 0U;
	uint32_t pollBudget = 0x00000000UL;
	driver_status_t transactionStatus = DRIVER_STATUS_SUCCESS;

	// Validate Input
	if ((I2Cx == NULL) || (pData == NULL))
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	// Validate the range for input parameters
	if ((I2C_IS_VALID_INSTANCE(I2Cx) == 0U) || (I2C_IS_VALID_ADDRESS(slaveAddress) == 0U))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}
	//! A zero-byte message and a zero-millisecond budget cannot describe a transfer,
	//! and a millisecond value whose poll product would overflow the 32-bit counter is
	//! refused here instead of wrapping into a premature timeout mid-message.
	if ((size == 0U) || (timeout_ms == 0U) || (timeout_ms > (UINT32_MAX / I2C_DRIVER_POLL_BUDGET_PER_MS)))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! The APB1 gate is application-owned, so observe it instead of enabling it. The
	//! gate must be proven on before the first register read, because a powered-down
	//! peripheral returns undefined data that could fake an enabled, idle controller.
	clockEnableMask = _I2C_GetAPB1ClockEnableMask(I2Cx);
	if (clockEnableMask == 0x00000000UL)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}
	if (RCC_GetPeripheralClockState(RCC_APB1_BUS, clockEnableMask) != DRIVER_STATUS_ON)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! Snapshot `CR1` once and answer both lifecycle preconditions from that single
	//! image, so the enabled and action-idle tests describe one hardware moment.
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);

	//! Validate Driver is Enabled
	if (Codec_I2C_ExtractOperationState(cr1RegImage) != DRIVER_STATUS_ON)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! A START or STOP bit that hardware has not retired yet belongs to a sequence
	//! already in flight, so a second message now would interleave two transactions.
	if (Codec_I2C_ValidateActionIdle(cr1RegImage) != DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! A busy bus means another controller owns the lines, so requesting START here
	//! would corrupt the message currently on the wire.
	if (Codec_I2C_IsBusBusy(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_ERROR_BUSY;
	}

	//! An error flag latched by an earlier aborted transaction would be decoded by
	//! this message's first event wait as its own failure, so it is refused while no
	//! transaction MMIO has happened and the application can still clear it.
	sr1Snapshot = LL_I2C_ReadSR1(I2Cx);
	if (Codec_I2C_ExtractError(sr1Snapshot, &latchedError) != DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}
	if (latchedError != I2C_ERROR_NONE)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! One budget for the whole message: START, the address phase, every payload
	//! byte, the EV8_2 tail, the STOP request, and the final bus-idle wait all draw
	//! from this counter, so the API honours exactly one timeout argument.
	pollBudget = (timeout_ms * I2C_DRIVER_POLL_BUDGET_PER_MS);

	//! START is the boundary after which every failure must leave through the single
	//! cleanup path below instead of a plain early return.
	transactionStatus = _I2C_GenerateStart(I2Cx, &pollBudget);

	//! EV5: the `SB` sample inside the START wait is the required first half, so the
	//! encoded address byte written here completes the event and commits this
	//! controller to the write direction.
	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		LL_I2C_WriteDR(I2Cx, (reg) Codec_I2C_EncodeAddressByte(slaveAddress, 0U));
		//! EV6 completes inside the wait: the ordered `SR1`-then-`SR2` pair there
		//! releases `ADDR` (ending `SCL` stretching) and its master bit proves the
		//! address phase still owned the bus, so no separate status sample follows.
		transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_ADDRESS_SENT, &pollBudget, NULL);
	}

	//! EV8: with `ADDR` released the peripheral accepts one byte per `TXE`, so the
	//! payload is paced by the same shared budget instead of a per-byte timeout.
	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		for (byteIndex = 0U; byteIndex < size; byteIndex++)
		{
			transactionStatus = _I2C_WaitForEvent
			(
				I2Cx,
				I2C_EVENT_FLAG_DATA_REGISTER_EMPTY,
				&pollBudget,
				NULL
			);
			if (transactionStatus != DRIVER_STATUS_SUCCESS)
			{
				break;
			}
			LL_I2C_WriteDR(I2Cx, (reg) pData[byteIndex]);
		}
	}

	//! EV8_2: `TXE` and `BTF` together prove the last byte has left the shift
	//! register, so the STOP requested next cannot truncate the message.
	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		transactionStatus = _I2C_WaitForEvent
		(
			I2Cx,
			I2C_EVENT_FLAG_TRANSFER_COMPLETE,
			&pollBudget,
			NULL
		);
	}

	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		//! The STOP helper itself closes with the bounded `BUSY` wait, so this one
		//! call both releases the lines and proves they were actually released
		//! before the transaction below can report success.
		transactionStatus = _I2C_GenerateStop(I2Cx, &pollBudget);
	}

	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_SUCCESS;
	}

	//! Single cleanup path for every post-START failure: it preserves the primary
	//! cause, releases the bus when the hardware still allows it, and clears only the
	//! error flags it observed.
	return _I2C_AcknowledgeErrors(I2Cx, transactionStatus, &pollBudget);
}

driver_status_t I2C_MasterReceive
(
	I2C_TypeDef* const			I2Cx,
	const i2c_address_t			slaveAddress,
	const i2c_reg_addr_t		startRegAddress,
	i2c_data_t* const			pData,
	const i2c_data_length_t		size,
	const uint32_t				timeout_ms
)
{
	
}