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

/** @brief Volatile busy-spin iterations one poll waits before sampling `SR1`
 * @def I2C_DRIVER_POLL_PACING_ITERATIONS
 * Calibrated so one loop iteration passes ~one SCL bit time (~10 us) at the 100
 * kHz standard-mode bus this Driver configures: at 8-9 cycles per volatile
 * decrement on the 72 MHz kernel clock, 72 iterations spend approximately that
 * dwell. The period is a pacing target, never a calibrated time base.
 */
#define I2C_DRIVER_POLL_PACING_ITERATIONS	((uint32_t) 72UL)
/** @brief Poll budget per millisecond for I2C driver operations @def I2C_DRIVER_POLL_BUDGET_PER_MS */
#define I2C_DRIVER_POLL_BUDGET_PER_MS		((uint32_t) 50UL)
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
		case I2C1_BASE_ADDRESS:
		{
			clockMask = RCC_APB1ENR_I2C1EN;
			break;
		}
		case I2C2_BASE_ADDRESS:
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
 * @note - Does not validate if input is NULL pointer because it is static function
 * @note - Returns `0x00UL` in case @p `I2Cx` is not within expected range
 */
__STATIC reg _I2C_GetAPB1PeripheralResetMask(const I2C_TypeDef* const I2Cx)
{
	reg resetMask = 0x00000000UL;

	switch ((uintptr_t) I2Cx)
	{
		case I2C1_BASE_ADDRESS:
		{
			resetMask = RCC_APB1RSTR_I2C1RST;
			break;
		}
		case I2C2_BASE_ADDRESS:
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
 * @brief Spends one poll-iteration dwell before sampling status registers
 * @details
 * A bounded volatile busy-spin that cannot be optimized away at any `-O`
 * level, pacing the sampling bus loop to roughly one SCL bit time at 100 kHz
 * (see @ref `I2C_DRIVER_POLL_PACING_ITERATIONS`). The dwell is deliberately
 * implemented without a timer dependency: the Driver layer owns no timebase,
 * and the wait loop only needs the sample cadence to sit inside the hardware
 * event window, not a calibrated delay contract.
 * @note This is signal pacing, not calibrated time: the dwell period is a
 * compile-time spin duration and must not be used for protocol timing.
 */
__STATIC_FORCEINLINE void _I2C_PollPacingDelay(volatile uint32_t pacingCounter)
{
	//! Volatile is the structurally load-bearing guarantee here: it makes every
	//! load/store real memory traffic, so even an aggressive optimizer cannot
	//! collapse this loop to nothing.
	while (pacingCounter > 0U)
	{
		pacingCounter--;
	}
}

/**
 * @brief Enters a bounded critical section and reports the prior interrupt mask
 * @details
 * Saves `PRIMASK` and masks processor interrupt delivery, so the caller can
 * run a sequence of status-register reads, `CR1` action writes, and `DR`
 * accesses as one uninterrupted hardware moment. The receive tails use this
 * around exactly the windows RM0008's polling method 2 and erratum ES096
 * 2.8.1-2.8.2 require, and never around bounded waits, so a bug inside the
 * window cannot stall the processor for more than a few register accesses.
 * @returns Prior `PRIMASK` value; pass it unchanged to
 * @ref `_I2C_ExitCritical`
 * @pre The caller runs in thread mode with a single-instance ownership contract
 * (no ISR or DMA touches the same I2C instance during the window)
 */
__STATIC_FORCEINLINE uint32_t _I2C_EnterCritical(void)
{
	//! Snapshot before masking, so restore always reproduces the caller's own
	//! state instead of assuming interrupts were enabled.
	const uint32_t priorPrimask = __get_PRIMASK();

	//! One atomic mask write, kept together with nothing else by contract.
	__disable_irq();

	return priorPrimask;
}

/**
 * @brief Exits a critical section by restoring the caller's prior interrupt mask
 * @details
 * Writes the `PRIMASK` snapshot produced by @ref `_I2C_EnterCritical` back
 * unchanged, so a call made while interrupts were already masked does not
 * experience a spurious re-enable when the window closes.
 * @param[in] priorPrimask Value returned by the matching
 * @ref `_I2C_EnterCritical` call
 */
__STATIC_FORCEINLINE void _I2C_ExitCritical(const uint32_t priorPrimask)
{
	//! Restore the observed prior state instead of an assumed enabled state.
	__set_PRIMASK(priorPrimask);
}

/**
 * @brief Maps one latched `SR1` error selector onto its transport failure status
 * @details
 * Pure Driver-side vocabulary translation from the error selector decoded by
 * @ref `Codec_I2C_ExtractError` to the generic transport status vocabulary.
 * Keeping the mapping in one helper guarantees that the unified event engine
 * answers every latched flag with the same transport status instead of
 * drifting into mismatched error narratives across its wait routes.
 * @param[in] sampledError Error selector decoded from a sampled `SR1` image
 * Accepted values:
 * - Any @ref `i2c_error_t` value except @ref `I2C_ERROR_NONE`; the caller
 *   tests for a latched error before invoking this helper
 * @returns @ref driver_status_t "Mapped transport failure status"
 * @retval - @ref `DRIVER_STATUS_ERROR_NO_ACK`: The address/data NACK flag (`AF`) was latched
 * @retval - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: The arbitration-lost flag (`ARLO`) was latched
 * @retval - @ref `DRIVER_STATUS_ERROR_OVERRUN`: The overrun flag (`OVR`) was latched
 * @retval - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level flag (BERR, PECERR, SMBALERT, or SMBus TIMEOUT) was latched
 * @note @ref `I2C_ERROR_NONE` never reaches this helper; a caller that passed
 * it would receive @ref `DRIVER_STATUS_ERROR_LINE_FAULT` rather than success,
 * which is why every caller first tests the decoded selector
 */
__STATIC_FORCEINLINE driver_status_t _I2C_MapSampledError(const i2c_error_t sampledError)
{
	// Local Variable
	driver_status_t mappedStatus = DRIVER_STATUS_ERROR_LINE_FAULT;

	//! The initializer above already names the line-fault family, so the switch
	//! only has to redirect the three specific transport failures to their own
	//! status names.
	switch (sampledError)
	{
		case I2C_ERROR_DATA_NACK:
		{
			mappedStatus = DRIVER_STATUS_ERROR_NO_ACK;
			break;
		}

		case I2C_ERROR_ARBITRATION_LOST:
		{
			mappedStatus = DRIVER_STATUS_ERROR_ARBITRATION_LOST;
			break;
		}

		case I2C_ERROR_OVERRUN:
		{
			mappedStatus = DRIVER_STATUS_ERROR_OVERRUN;
			break;
		}

		default:
		{
			mappedStatus = DRIVER_STATUS_ERROR_LINE_FAULT;
			break;
		}
	}

	return mappedStatus;
}

/**
 * @brief Waits for one bounded status event inside the shared poll budget
 * @details
 * One unified engine owns the entire wait: each paced iteration draws one poll
 * from the shared transaction budget, samples `SR1` once, and decodes the
 * hardware error flags before evaluating the requested event — so a hardware
 * failure is named immediately instead of surfacing later as a plain timeout.
 * Every selector owns a unique application value (even though `SR1.ADDR` and
 * `SR2.BUSY` sit on the same raw bit position), so the per-iteration routing
 * below is provably exclusive and the engine answers exactly the choreography
 * the requested condition demands:
 * - @ref `I2C_EVENT_FLAG_ADDRESS_SENT` — the ordered `SR1`-then-`SR2` pair.
 *   RM0008 releases the address-phase `ADDR` event only through the mandatory
 *   second read, and that release also ends the `SCL` stretched pause. The
 *   release read is therefore gated on the same sampled `SR1` image that
 *   observed `ADDR`: an image that never latched the event must not trigger
 *   the read that would erase it. The same `SR2` sample that releases `ADDR`
 *   proves the peripheral is still master (`MSL`), so a released address
 *   phase is never reported for a transaction that lost bus ownership.
 * - @ref `I2C_EVENT_FLAG_BUS_IDLE` — the `SR2` bus condition. Bus ownership
 *   has no `SR1` flag at all, so `BUSY` is the primary `SR2` sample of every
 *   iteration; no release gating is needed because a bus-idle caller owns no
 *   address phase, so the `SR1` error decode can never erase a latched `ADDR`.
 * - Every other selector is an `SR1` condition answered by
 *   @ref `Codec_I2C_IsEventPending`; a composite selector such as
 *   @ref `I2C_EVENT_FLAG_TRANSFER_COMPLETE` succeeds only when every selected
 *   flag is set together.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in] eventFlag Status event selector to wait for
 * Accepted values:
 * - Any selector admitted by @ref `I2C_IS_VALID_EVENT_FLAG`
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Decremented once per `SR1` sample
 * @param[out] pError Optional caller-owned report of the latched `SR1` error flag
 * Expected values:
 * - On a hardware-failure or error-decode failure exit: The latched error selector
 *   (@ref `I2C_ERROR_NONE` when decoding itself fails)
 * - `NULL`: The report is skipped
 * - Any other exit leaves the caller's value untouched
 * @returns @ref driver_status_t "Event-wait status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The selected event was observed (and completed through its own choreography where applicable)
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `eventFlag` is not admitted by @ref `I2C_IS_VALID_EVENT_FLAG`
 * @retval - @ref `DRIVER_STATUS_ERROR_NO_ACK`: The NACK flag (`AF`) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: The arbitration-lost flag (`ARLO`) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_OVERRUN`: The overrun flag (`OVR`) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level flag (BERR, PECERR, SMBALERT, or SMBus TIMEOUT) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: On the address route, `ADDR` was observed but its releasing `SR2` sample proved the peripheral no longer owns the bus as master
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: @p `pPollBudget` was exhausted
 * @note A reported error flag stays latched: the status names the failure
 * family and @p `pError` names the exact flag, because the caller still has
 * to preserve and acknowledge it
 * @note The receive-path `EV6_3` sequence that must interpose `CR1` action
 * writes between the two status reads cannot reuse the address route's
 * pairing: the receive transaction owns that step, and it observes the
 * address latch through the dedicated @ref `_I2C_WaitForAddressLatch` helper
 * instead of through this engine's release route
 * @pre The peripheral is enabled and the caller holds the instance exclusively; on the address route, the address phase has already shifted the address byte out
 */
__STATIC driver_status_t _I2C_WaitForEvent
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
	driver_status_t exitStatus = DRIVER_STATUS_ERROR_TIMEOUT;

	uint32_t pollBudget = 0UL;

	//! Validate caller-owned poll budget.
	if (pPollBudget == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Preserve the caller's remaining transaction budget.
	pollBudget = *pPollBudget;

	//! Validate the selector once so every per-iteration route below routes on
	//! selector equality only; the application vocabulary never mirrors raw
	//! register masks, so each route is provably exclusive of the others.
	if (I2C_IS_VALID_EVENT_FLAG(eventFlag) == 0U)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! Bounded polling loop.
	while (pollBudget > 0U)
	{
		//! Pace the next hardware status sample.
		_I2C_PollPacingDelay(I2C_DRIVER_POLL_PACING_ITERATIONS);

		//! Sample SR1 exactly once per iteration.
		sr1RegImage = LL_I2C_ReadSR1(I2Cx);

		pollBudget--;

		//! Decode hardware error flags before checking the requested event.
		sampleStatus = Codec_I2C_ExtractError
		(
			sr1RegImage,
			&sampledError
		);

		if (sampleStatus != DRIVER_STATUS_SUCCESS)
		{
			if (pError != NULL)
			{
				*pError = sampledError;
			}

			exitStatus = sampleStatus;
			break;
		}

		//! Handle any observed hardware error.
		if (sampledError != I2C_ERROR_NONE)
		{
			if (pError != NULL)
			{
				*pError = sampledError;
			}

			exitStatus = _I2C_MapSampledError(sampledError);
			break;
		}

		//! Address route: ordered SR1 -> SR2 release choreography.
		if (eventFlag == I2C_EVENT_FLAG_ADDRESS_SENT)
		{
			//! Gate the release read on this snapshot: an image that never
			//! observed ADDR must not trigger the read that would erase the
			//! latched event.
			if (Codec_I2C_IsAddressPending(sr1RegImage) == DRIVER_STATUS_SUCCESS)
			{
				//! Complete the mandatory SR1 -> SR2 pair; this is the read
				//! that releases ADDR and ends the SCL stretch.
				sr2RegImage = LL_I2C_ReadSR2(I2Cx);

				//! Prove master ownership from the same release sample.
				if (Codec_I2C_IsMasterMode(sr2RegImage) != DRIVER_STATUS_SUCCESS)
				{
					exitStatus = DRIVER_STATUS_ERROR_FAIL;
					break;
				}

				//! ADDR was observed, released, and master ownership confirmed.
				exitStatus = DRIVER_STATUS_SUCCESS;
				break;
			}

			//! ADDR is not latched in this snapshot; the next paced dwell gives
			//! the event its chance to latch before the next sample.
		}
		else
		{
			//! Bus-idle route: the SR2 condition is the primary sample of every
			//! iteration; no release gating is needed because a bus-idle caller
			//! owns no address phase, so this SR1 decode can never erase one.
			if (eventFlag == I2C_EVENT_FLAG_BUS_IDLE)
			{
				sr2RegImage = LL_I2C_ReadSR2(I2Cx);

				//! The Codec answers ERROR exactly when BUSY is clear, which is
				//! the idle proof this route exists to produce.
				if (Codec_I2C_IsBusBusy(sr2RegImage) == DRIVER_STATUS_ERROR)
				{
					exitStatus = DRIVER_STATUS_SUCCESS;
					break;
				}

				//! BUSY stays set; give the releasing controller another paced
				//! window.
			}
			else
			{
				//! SR1-predicate route: the generic pending test.
				sampleStatus = Codec_I2C_IsEventPending(sr1RegImage, eventFlag);
				if (sampleStatus == DRIVER_STATUS_SUCCESS)
				{
					exitStatus = DRIVER_STATUS_SUCCESS;
					break;
				}

				//! Only the expected "event not pending" result may continue
				//! polling.
				if (sampleStatus != DRIVER_STATUS_ERROR)
				{
					exitStatus = sampleStatus;
					break;
				}
			}
		}
	}

	//! Publish the remaining shared transaction budget.
	*pPollBudget = pollBudget;

	return exitStatus;
}

/**
 * @brief Waits for the address-sent event to latch without releasing it
 * @details
 * Observation-only counterpart of the unified engine's @ref
 * `I2C_EVENT_FLAG_ADDRESS_SENT` route. RM0008's single-byte master-reception
 * sequence (`EV6_3`, Figure 277) must program `ACK=0` *before* clearing
 * `ADDR` and a STOP request *just after* clearing it, so the address release
 * cannot happen inside the wait the way the multi-byte route performs it.
 * This helper therefore paces, samples `SR1` once per iteration, and decodes
 * hardware errors first — exactly like the unified engine — but only tests
 * whether `ADDR` is latched and never reads `SR2`: the address release is left
 * latched for the caller's `EV6_3` window, which owns the paired clear with
 * its interposed `CR1` writes.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Decremented once per `SR1` sample
 * @returns @ref driver_status_t "Address-latch wait status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: `ADDR` was observed latched and is left latched for the caller's `EV6_3` sequence
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_NO_ACK`: The NACK flag (`AF`) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: The arbitration-lost flag (`ARLO`) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_OVERRUN`: The overrun flag (`OVR`) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level flag (BERR, PECERR, SMBALERT, or SMBus TIMEOUT) was observed first
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: @p `pPollBudget` was exhausted
 * @note Unlike the unified engine's address route, this helper performs no
 * master-mode proof: the caller's `EV6_3` window performs the release itself
 * and the subsequent waits judge the failed-ownership cause through the
 * shared cleanup path
 * @pre The peripheral is enabled, the caller holds the instance exclusively,
 * and the address byte with `R/W=1` has already been written to `DR`
 */
__STATIC driver_status_t _I2C_WaitForAddressLatch(I2C_TypeDef* const I2Cx, uint32_t* const pPollBudget)
{
	// Local Variables
	reg sr1RegImage = 0x00000000UL;
	i2c_error_t sampledError = I2C_ERROR_NONE;
	driver_status_t sampleStatus = DRIVER_STATUS_ERROR;
	driver_status_t exitStatus = DRIVER_STATUS_ERROR_TIMEOUT;

	uint32_t pollBudget = 0UL;

	//! Validate caller-owned poll budget.
	if (pPollBudget == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Preserve the caller's remaining transaction budget.
	pollBudget = *pPollBudget;

	//! Bounded polling loop shares the pacing and error decode of the unified
	//! engine so both waits report every latched failure the same way.
	while (pollBudget > 0U)
	{
		//! Pace the next hardware status sample.
		_I2C_PollPacingDelay(I2C_DRIVER_POLL_PACING_ITERATIONS);

		//! Sample SR1 exactly once per iteration.
		sr1RegImage = LL_I2C_ReadSR1(I2Cx);

		pollBudget--;

		//! Decode hardware error flags before testing the event.
		sampleStatus = Codec_I2C_ExtractError(sr1RegImage, &sampledError);

		if (sampleStatus != DRIVER_STATUS_SUCCESS)
		{
			exitStatus = sampleStatus;
			break;
		}

		//! Handle any observed hardware error before the event test.
		if (sampledError != I2C_ERROR_NONE)
		{
			exitStatus = _I2C_MapSampledError(sampledError);
			break;
		}

		//! Test the latch only: an observed ADDR stays latched because this
		//! helper deliberately performs no `SR2` release read.
		if (Codec_I2C_IsAddressPending(sr1RegImage) == DRIVER_STATUS_SUCCESS)
		{
			exitStatus = DRIVER_STATUS_SUCCESS;
			break;
		}

		//! ADDR is not latched in this snapshot; the next paced dwell gives
		//! the event its chance to latch before the next sample.
	}

	//! Publish the remaining shared transaction budget.
	*pPollBudget = pollBudget;

	return exitStatus;
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
 * @retval - @ref `DRIVER_STATUS_ERROR_NO_ACK`: A transfer NACK flag (`AF`) was observed before `SB`
 * @retval - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: An arbitration-lost flag (`ARLO`) was observed before `SB`
 * @retval - @ref `DRIVER_STATUS_ERROR_OVERRUN`: An overrun flag (`OVR`) was observed before `SB`
 * @retval - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level flag (BERR, PECERR, SMBALERT, or SMBus TIMEOUT) was observed before `SB`
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: @p `pPollBudget` was exhausted
 * @note @ref `DRIVER_STATUS_ERROR_INVALID_ARG` cannot occur here because this
 * helper always waits for the fixed @ref `I2C_EVENT_FLAG_START_GENERATED` selector
 * @pre The peripheral is enabled and neither `START` nor `STOP` is still pending
 */
__STATIC driver_status_t _I2C_GenerateStart(I2C_TypeDef* const I2Cx, uint32_t* const pPollBudget)
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
 * `CR1.STOP` itself, so this helper closes the request through the unified
 * engine's `SR2`
 * @ref `I2C_EVENT_FLAG_BUS_IDLE` route, and returns success only after the
 * lines are observed free.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Decremented once per poll iteration by the bus-idle wait
 * @returns @ref driver_status_t "STOP request status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: STOP was requested and the bus was observed idle
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_NO_ACK`: A transfer NACK flag (`AF`) was observed during the idle wait
 * @retval - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: An arbitration-lost flag (`ARLO`) was observed during the idle wait
 * @retval - @ref `DRIVER_STATUS_ERROR_OVERRUN`: An overrun flag (`OVR`) was observed during the idle wait
 * @retval - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level flag (BERR, PECERR, SMBALERT, or SMBus TIMEOUT) was observed during the idle wait
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: `BUSY` stayed set until @p `pPollBudget` was exhausted
 * @note No failure other than these is reachable, because this helper owns the
 * register image it passes on and always waits on the fixed idle condition
 * @pre The peripheral is enabled, still master, and at a byte boundary
 */
__STATIC driver_status_t _I2C_GenerateStop(I2C_TypeDef* const I2Cx, uint32_t* const pPollBudget)
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
	//! idle wait is what proves completion; every caller's "STOP succeeded" answer
	//! is backed by the same unified-engine idle evidence.
	//! The optional error report is not consumed here for the same reason as the
	//! START wait: the cleanup path re-snapshots `SR1` and never needs plumbing.
	return _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_BUS_IDLE, pPollBudget, NULL);
}

/**
 * @brief Restores the master baseline ACK and POS control bits in `CR1`
 * @details
 * Re-applies the configured master-I2C acknowledgement baseline (`ACK=1`,
 * `POS=0`) that a receive transaction's tail deliberately overwrote: the
 * one- and multi-byte tails stage `ACK=0` to NACK the final byte, and the
 * two-byte tail additionally leaves `POS=1`. Restoring it in one helper keeps
 * the receive close path and the failure cleanup path consistent, so the next
 * transfer never inherits a tail's NACK configuration. The write is skipped
 * when the staged image already equals the snapshot, because `ACK` and `POS`
 * are ordinary read-write bits whose repeated value would change nothing.
 * The caller owns proving that no `CR1` action (START or STOP) is still
 * pending before calling this helper.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @returns @ref driver_status_t "Baseline-restore status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The baseline `ACK=1`/`POS=0` state was applied (or already held)
 * @note All other `CR1` bits are preserved; no operation-state, action, or
 * IRQ-source state is touched
 */
__STATIC driver_status_t _I2C_RestoreBaseline(I2C_TypeDef* const I2Cx)
{
	// Local Variables
	reg cr1RegImage = 0x00000000UL;
	reg cr1TargetImage = 0x00000000UL;

	//! Snapshot once so the staged baseline is compared against the same
	//! hardware moment it will overwrite.
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);
	cr1TargetImage = cr1RegImage;
	ASSERT_DRIVER_STATUS(Codec_I2C_StageAck(&cr1TargetImage, 1U));
	ASSERT_DRIVER_STATUS(Codec_I2C_StagePos(&cr1TargetImage, 0U));

	//! Ordinary RW bits: skip the write when the staged image already matches,
	//! so a transfer run on the configured baseline costs no CR1 traffic.
	if (cr1RegImage != cr1TargetImage)
	{
		LL_I2C_WriteCR1(I2Cx, cr1TargetImage);
	}

	return DRIVER_STATUS_SUCCESS;
}

/**
 * @brief Preserves the primary fault and returns the peripheral to idle
 * @details
 * The single cleanup path for every failure after START was requested. It
 * snapshots `SR1` first, because acknowledging a flag would erase the only
 * record of why the transaction stopped, and decodes that snapshot into the
 * primary error. The observed error flags are cleared next, so the optional
 * abort STOP below is issued from a clean `SR1` state and its bounded
 * bus-idle wait judges recovery on fresh hardware rather than resurrecting
 * the failure this path is recovering from; the clear also follows RM0008's
 * NACK handling, which acknowledges `AF` before STOP. An abort STOP is
 * requested only when the hardware can still legally accept one. Every
 * recovery step is best effort:
 * @p `primaryStatus` is returned unchanged so a cleanup side effect can never
 * turn a failed transfer into a reported success or replace its cause.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in] primaryStatus Failure status of the transaction step that stopped the transfer
 * Accepted values:
 * - @ref `DRIVER_STATUS_ERROR_NO_ACK`: A transfer NACK was observed
 * - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: Arbitration was lost to another master
 * - @ref `DRIVER_STATUS_ERROR_OVERRUN`: A data-overrun condition was observed
 * - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level error flag was observed
 * - @ref `DRIVER_STATUS_ERROR_FAIL`: A protocol violation was observed without an `SR1` error flag
 * - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: The shared poll budget was exhausted
 * - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: A required pointer was missing (passed through unchanged)
 * - Any other @ref `driver_status_t` value: Passed through unchanged
 * @param[in,out] pPollBudget Remaining transaction poll budget
 * Expected values:
 * - Non-`NULL`: Consumed by the optional abort-STOP bus-idle wait
 * @returns @ref driver_status_t "Preserved primary transaction failure"
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pPollBudget` was `NULL`
 * @retval - Any other @ref `driver_status_t` value: @p `primaryStatus` is returned unchanged
 * @note Arbitration loss is never followed by an abort STOP, because hardware
 * has already released the lines and cleared the master state
 * @note When the action bits permit a `CR1` write again, the master baseline
 * `ACK=1`/`POS=0` is restored best effort, so an interrupted receive tail
 * cannot leak its NACK staging into the next transfer
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

	//! Decode the snapshot into a local value, so this Codec can only fail if the
	//! contract changes; treat that as "no primary error" instead of returning
	//! early and abandoning the acknowledgement and bus release below.
	if (Codec_I2C_ExtractError(sr1Snapshot, &latchedError) != DRIVER_STATUS_SUCCESS)
	{
		latchedError = I2C_ERROR_NONE;
	}

	//! Acknowledge the observed error set before the abort STOP is requested. The
	//! write-zero-to-clear image derives from the snapshot, so the decoded set
	//! stays exact, and clearing first is what lets the STOP helper's idle wait
	//! actually wait: with the flags still latched its `SR1` sample would report
	//! the old failure on iteration one and abandon the bus release. This also
	//! follows RM0008's NACK handling, which clears `AF` before STOP.
	if (Codec_I2C_StageClearErrors(&sr1WriteImage, sr1Snapshot) == DRIVER_STATUS_SUCCESS)
	{
		LL_I2C_WriteSR1(I2Cx, sr1WriteImage);
	}

	//! An abort STOP is only legal while this peripheral still owns the bus and no
	//! earlier action is pending. Arbitration loss is excluded explicitly because
	//! hardware has already released SDA/SCL and returned to slave mode. By the
	//! time this runs, the observed error flags from the snapshot are already
	//! cleared, so the STOP helper's bounded idle wait judges the fresh `SR1`
	//! state instead of resurrecting the failure this path is recovering from.
	if
	(
		(latchedError != I2C_ERROR_ARBITRATION_LOST) &&
		(Codec_I2C_ValidateActionIdle(LL_I2C_ReadCR1(I2Cx)) == DRIVER_STATUS_SUCCESS) &&
		(Codec_I2C_IsMasterMode(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS)
	)
	{
		//! Recovery is best effort by contract: a stuck-busy result from the STOP
		//! helper is the case ES096 leaves to board-level recovery, so the whole
		//! result is discarded and can never replace the cause reported to the
		//! caller.
		(void) _I2C_GenerateStop(I2Cx, pPollBudget);
	}

	//! A receive-failure cleanup can run while `ACK=0` (or `POS=1`, on the
	//! two-byte tail) is still staged from a tail that was interrupted before it
	//! could restore its baseline, so the restore is attempted whenever the
	//! action bits permit a `CR1` write again. The result stays best effort and
	//! only removes the tail's leftover, never altering the primary status.
	if (Codec_I2C_ValidateActionIdle(LL_I2C_ReadCR1(I2Cx)) == DRIVER_STATUS_SUCCESS)
	{
		(void) _I2C_RestoreBaseline(I2Cx);
	}

	return primaryStatus;
}

/**
 * @brief Validates the shared preconditions of a whole-message master transfer
 * @details
 * The common preflight every transfer API performs before any transaction
 * MMIO: caller-owned pointers, instance and target-address vocabulary,
 * message-size and timeout bounds, the application-owned `APB1` gate, the
 * enabled and action-idle `CR1` state, a free bus, and the absence of stale
 * `SR1` error flags left behind by an earlier aborted transaction. Snapshotting
 * `CR1` once makes the enabled and action-idle tests describe one hardware
 * moment. This helper performs no selective revalidation later: because the
 * caller holds the instance exclusively, the very same preconditions remain
 * satisfied until START is requested.
 * @param[in] I2Cx I2C peripheral instance
 * Expected values:
 * - @ref `I2C1`
 * - @ref `I2C2`
 * @param[in] slaveAddress Unshifted 7-bit I2C target address
 * Accepted values:
 * - `0x08U` through `0x77U`: Non-reserved 7-bit target address
 * @param[in] pData Payload buffer
 * Expected values:
 * - Non-`NULL`: Points to at least @p `size` valid bytes
 * @param[in] size Number of payload bytes
 * Accepted values:
 * - Nonzero: Complete message length in bytes
 * @param[in] timeout_ms Transaction-wide timeout in milliseconds
 * Accepted values:
 * - Nonzero: Total polling budget for the complete transaction
 * @returns @ref driver_status_t "Transfer-preflight validation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The preflight passed and no bus or register state was altered
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` or @p `pData` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx`, @p `slaveAddress`, @p `size`, or @p `timeout_ms` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The matching gate mask is unresolved, the bus is busy-checkable only
 *   through a valid image, the peripheral is disabled, a START/STOP action is pending, or a stale `SR1` error flag is latched
 * @retval - @ref `DRIVER_STATUS_ERROR_NOT_READY`: The application-owned APB1 clock gate is disabled
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The bus is busy
 * @pre The application owns the instance exclusively through the whole call
 * @note This helper observes the RCC gate through the clock-state service and
 * never mutates it, per the repository's application-owned-network-clock rule
 */
__STATIC driver_status_t _I2C_ValidateTransferPreconditions
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

	//! Validate pointers before any memory-mapped access.
	if ((I2Cx == NULL) || (pData == NULL))
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Validate instance and target-address vocabulary.
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
		//! A disabled gate is a missing application bring-up step, not a wrong live
		//! state, so it is reported with the dedicated NOT_READY status.
		return DRIVER_STATUS_ERROR_NOT_READY;
	}

	//! Snapshot CR1 once and answer both lifecycle preconditions from that single
	//! image, so the enabled and action-idle tests describe one hardware moment.
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);

	//! Validate the peripheral is enabled.
	if (Codec_I2C_ExtractOperationState(cr1RegImage) != DRIVER_STATUS_ON)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! A pending START or STOP bit means the peripheral has not retired that action
	//! yet, so a new message now would interleave two transactions.
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

	return DRIVER_STATUS_SUCCESS;
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
		//! A disabled gate is a missing application bring-up step, not a wrong live
		//! state, so it is reported with the dedicated NOT_READY status.
		return DRIVER_STATUS_ERROR_NOT_READY;
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
		//! A disabled gate is a missing application bring-up step, not a wrong live
		//! state, so it is reported with the dedicated NOT_READY status.
		return DRIVER_STATUS_ERROR_NOT_READY;
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
	i2c_data_length_t byteIndex = 0U;
	uint32_t pollBudget = 0x00000000UL;
	driver_status_t transactionStatus = DRIVER_STATUS_SUCCESS;

	//! Shared transfer preflight: pointers, vocabulary, bounds, gate, live state,
	//! bus, and stale-error refusal, all before any transaction MMIO.
	transactionStatus = _I2C_ValidateTransferPreconditions(I2Cx, slaveAddress, pData, size, timeout_ms);
	if (transactionStatus != DRIVER_STATUS_SUCCESS)
	{
		return transactionStatus;
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
		//! EV6 completes inside the unified engine's address route: the ordered
		//! `SR1`-then-`SR2` pair there releases `ADDR` (ending `SCL` stretching)
		//! and its master bit proves the address phase still owned the bus, so no
		//! separate status sample follows.
		transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_ADDRESS_SENT, &pollBudget, NULL);
	}

	//! EV8: with `ADDR` released the peripheral accepts one byte per `TXE`, so the
	//! payload is paced by the same shared budget instead of a per-byte timeout.
	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		for (byteIndex = 0U; byteIndex < size; byteIndex++)
		{
			transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_DATA_REGISTER_EMPTY, &pollBudget, NULL);
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
		transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_TRANSFER_COMPLETE, &pollBudget, NULL);
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

/**
 * @brief Receives one complete bounded message from a 7-bit I2C target
 * @details
 * Implements RM0008's polling "method 2" master-receiver sequences
 * (Section 27.3.5, Figures 275-277) so the final-byte NACK, the STOP request,
 * and the reads that consume the last bytes cannot corrupt or truncate the
 * tail on this erratum-affected silicon (ES096 2.8.1-2.8.2):
 * - @p `size == 1` (`EV6_3`): the address event is latched by an
 *   observation-only wait and an interrupt-masked window stages `ACK=0`,
 *   releases `ADDR` through the mandatory `SR1`-then-`SR2` read pair, and sets
 *   `STOP` just after the stretch releases. Only then is the single byte
 *   consumed on `RXNE`.
 * - @p `size == 2` (`EV6_1` + `EV7_3`): `POS=1`/`ACK=1` are staged before the
 *   address phase is scheduled, `ACK` is cleared right after the paired
 *   address release, `BTF` proves both final bytes are latched, and one
 *   interrupt-masked window programs `STOP` and reads both bytes back to back.
 * - @p `size > 2` (`EV7_2`): ordinary bytes are consumed on `RXNE` while more
 *   than three remain, then `BTF` stretches the bus with the second-to-last
 *   byte in `DR`; `ACK` is cleared so the final byte is NACKed, and one
 *   interrupt-masked window reads that byte, programs `STOP`, and reads the
 *   next-to-last byte without interruption before the final byte is consumed
 *   on `RXNE`.
 * Every successful tail closes by waiting for the bus to go idle and then
 * restoring the `ACK=1`/`POS=0` master baseline, so the next transfer never
 * inherits a tail's NACK staging. @p `timeout_ms` is transaction-wide exactly
 * as in @ref `I2C_MasterTransmit`: START, the address phase, every payload
 * byte, the receive tail, the STOP retirement, and the closing idle wait all
 * draw from one poll budget.
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @param[in] slaveAddress Unshifted 7-bit I2C target address
 * Accepted values:
 * - `0x08U` through `0x77U`: Non-reserved 7-bit target address
 * @param[out] pData Destination data buffer
 * Expected values:
 * - Non-`NULL`: Destination of @p `size` bytes; receives the bytes read from the bus
 * @param[in] size Number of bytes to receive
 * Accepted values:
 * - Nonzero: Complete message length in bytes
 * @param[in] timeout_ms Transaction-wide timeout in milliseconds
 * Accepted values:
 * - Nonzero: Total polling budget for the complete transaction
 * @returns @ref driver_status_t "I2C controller receive operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: All requested bytes were received and STOP was generated and retired
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` or @p `pData` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx`, @p `slaveAddress`, @p `size`, or @p `timeout_ms` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_NOT_READY`: The APB1 clock gate is disabled
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: I2C is not enabled, a START/STOP request is still pending, or a stale `SR1` error flag is latched
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The bus is busy or another owner holds the I2C instance
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: The transaction-wide timeout expired
 * @retval - @ref `DRIVER_STATUS_ERROR_NO_ACK`: The target did not acknowledge the address
 * @retval - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: Arbitration was lost to another master after START
 * @retval - @ref `DRIVER_STATUS_ERROR_OVERRUN`: An `OVR` condition was flagged after START
 * @retval - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level error (BERR, PECERR, SMBALERT, or SMBus TIMEOUT) was flagged after START
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: The transfer left master mode without an `SR1` error flag
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre I2C is configured, enabled, idle, and exclusively owned by the caller
 * @pre Every `SR1` error flag from an earlier aborted transaction was already acknowledged by the application
 * @note The final received byte is always NACKed by the tail; the destination
 * prefix received before a failure is left untouched and is not rolled back,
 * because boundary rollback would require caller-provided scratch storage
 * @note The interrupt-masked tail windows save and restore the caller's
 * `PRIMASK`; they mask delivery for a handful of register accesses only
 * @note A failure after START enters the same single best-effort cleanup path
 * as the transmit path: the primary cause is preserved, an abort STOP is
 * requested only while the peripheral still reports master mode and no action
 * is pending, only the observed `SR1` error flags are cleared, and the master
 * `ACK`/`POS` baseline is restored whenever the action bits permit a `CR1`
 * write again
 */
driver_status_t I2C_MasterReceive
(
	I2C_TypeDef* const			I2Cx,
	const i2c_address_t			slaveAddress,
	i2c_data_t* const			pData,
	const i2c_data_length_t		size,
	const uint32_t				timeout_ms
)
{
	// Local Variables
	i2c_data_length_t byteIndex = 0U;
	i2c_data_length_t bytesRemaining = size;

	reg cr1RegImage = 0x00000000UL;
	reg cr1NackImage = 0x00000000UL;
	reg cr1TargetImage = 0x00000000UL;

	uint32_t pollBudget = 0x00000000UL;
	uint32_t priorPrimask = 0x00000000UL;

	driver_status_t transactionStatus = DRIVER_STATUS_SUCCESS;

	//! Shared transfer preflight: pointers, vocabulary, bounds, gate, live state,
	//! bus, and stale-error refusal, all before any transaction MMIO.
	transactionStatus = _I2C_ValidateTransferPreconditions(I2Cx, slaveAddress, pData, size, timeout_ms);
	if (transactionStatus != DRIVER_STATUS_SUCCESS)
	{
		return transactionStatus;
	}

	//! One budget for the whole message: START, the address phase, every payload
	//! byte, the receive tail, the STOP retirement, and the closing bus-idle wait
	//! all draw from this counter, so the API honours exactly one timeout argument.
	pollBudget = (timeout_ms * I2C_DRIVER_POLL_BUDGET_PER_MS);

	//! The two-byte tail (EV7_3) must have ACK=1/POS=1 applied before the
	//! address reception starts, because POS repoints which byte the ACK
	//! control honours and a CR1 write would be illegal once START is pending
	//! with the action still unresolved.
	if (bytesRemaining == 2U)
	{
		cr1RegImage = LL_I2C_ReadCR1(I2Cx);
		cr1TargetImage = cr1RegImage;
		ASSERT_DRIVER_STATUS(Codec_I2C_StageAck(&cr1TargetImage, 1U));
		ASSERT_DRIVER_STATUS(Codec_I2C_StagePos(&cr1TargetImage, 1U));

		//! Ordinary RW bits, so the preflight write is skipped when the staged
		//! image already matches the snapshot of a freshly configured baseline.
		if (cr1RegImage != cr1TargetImage)
		{
			LL_I2C_WriteCR1(I2Cx, cr1TargetImage);
		}
	}

	//! START is the boundary after which every failure must leave through the single
	//! cleanup path below instead of a plain early return.
	transactionStatus = _I2C_GenerateStart(I2Cx, &pollBudget);

	//! EV5: the SB sample inside the START wait is the required first half, so the
	//! address byte written here completes the event and commits this controller
	//! to the read direction.
	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		LL_I2C_WriteDR(I2Cx, (reg) Codec_I2C_EncodeAddressByte(slaveAddress, 1U));
	}

	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		if (bytesRemaining == 1U)
		{
			//! EV6_3 route: latch the address event without releasing it, because
			//! the paired clear must happen inside the window below with ACK=0
			//! staged before it and STOP staged just after it.
			transactionStatus = _I2C_WaitForAddressLatch(I2Cx, &pollBudget);

			if (transactionStatus == DRIVER_STATUS_SUCCESS)
			{
				//! Snapshot `CR1` and stage both ordered tail images before the
				//! window: the ACK and STOP staging is pure Codec work over
				//! local images, so every fallible call completes while delivery
				//! is still enabled and the masked window contains only register
				//! traffic that cannot fail or early-return through the
				//! assertion macro.
				cr1RegImage = LL_I2C_ReadCR1(I2Cx);
				cr1TargetImage = cr1RegImage;
				ASSERT_DRIVER_STATUS(Codec_I2C_StageAck(&cr1TargetImage, 0U));
				cr1NackImage = cr1TargetImage;
				ASSERT_DRIVER_STATUS(Codec_I2C_StageStop(&cr1TargetImage));

				//! EV6_3 window: the NACK for the only byte and the STOP request
				//! bracket the paired ADDR release. Masking delivery closes the
				//! ES096 2.8.1 window between the latched observation and the
				//! release pair.
				priorPrimask = _I2C_EnterCritical();

				//! Ordinary RW bit: skipped when the staged image already
				//! matches, in which case the NACK request is already staged.
				if (cr1RegImage != cr1NackImage)
				{
					LL_I2C_WriteCR1(I2Cx, cr1NackImage);
				}

				//! Complete the mandatory SR1 -> SR2 pair here: this is the read
				//! pair that releases ADDR and ends the SCL stretch.
				(void) LL_I2C_ReadSR1(I2Cx);
				(void) LL_I2C_ReadSR2(I2Cx);

				//! STOP is a self-clearing action request, so this write is
				//! unconditional even when the staged image matches a snapshot
				//! taken just before it.
				LL_I2C_WriteCR1(I2Cx, cr1TargetImage);

				_I2C_ExitCritical(priorPrimask);

				//! EV7: the only byte, consumed on RXNE after the tail staged
				//! its NACK and STOP.
				transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_DATA_REGISTER_FULL, &pollBudget, NULL);
				if (transactionStatus == DRIVER_STATUS_SUCCESS)
				{
					pData[byteIndex] = (i2c_data_t) LL_I2C_ReadDR(I2Cx);
				}
			}
		}
		else
		{
			//! EV6 for multi-byte reception: the paired release inside the
			//! unified engine's address route is exactly the plain clear this
			//! length needs; the same SR2 sample proves master ownership.
			transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_ADDRESS_SENT, &pollBudget, NULL);

			if (transactionStatus == DRIVER_STATUS_SUCCESS)
			{
				if (bytesRemaining == 2U)
				{
					//! EV6_1: clear ACK right after the paired release; with
					//! POS=1 staged before the address phase, this NACK honours
					//! the second byte instead of the first.
					cr1RegImage = LL_I2C_ReadCR1(I2Cx);
					cr1TargetImage = cr1RegImage;
					ASSERT_DRIVER_STATUS(Codec_I2C_StageAck(&cr1TargetImage, 0U));

					//! Ordinary RW bits: skip the write when the staged image
					//! already matches the observed one.
					if (cr1RegImage != cr1TargetImage)
					{
						LL_I2C_WriteCR1(I2Cx, cr1TargetImage);
					}
				}
			}

			//! EV7: consume ordinary bytes while more than three remain, so the
			//! tail keeps RM0008's three-byte problem intact from here on.
			while ((transactionStatus == DRIVER_STATUS_SUCCESS) && (bytesRemaining > 3U))
			{
				transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_DATA_REGISTER_FULL, &pollBudget, NULL);
				if (transactionStatus == DRIVER_STATUS_SUCCESS)
				{
					pData[byteIndex] = (i2c_data_t) LL_I2C_ReadDR(I2Cx);
					byteIndex++;
					bytesRemaining--;
				}
			}

			if (transactionStatus == DRIVER_STATUS_SUCCESS)
			{
				if (bytesRemaining == 2U)
				{
					//! EV7_3: BTF proves DR and the shift register hold the final
					//! two bytes, which is the stretched safe state to close from.
					transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED, &pollBudget, NULL);
					if (transactionStatus == DRIVER_STATUS_SUCCESS)
					{
						//! Stage the STOP image before the window so the masked
						//! sequence below is fallible-free register traffic.
						cr1RegImage = LL_I2C_ReadCR1(I2Cx);
						cr1TargetImage = cr1RegImage;
						ASSERT_DRIVER_STATUS(Codec_I2C_StageStop(&cr1TargetImage));

						//! ES096 2.8.2 window: STOP and both DR reads must be one
						//! uninterrupted sequence, since hardware retires the STOP
						//! request right after the NACKed byte and neither DR read
						//! can be reordered around an interruption.
						priorPrimask = _I2C_EnterCritical();

						//! STOP is an action request, so this write is
						//! unconditional even when the staged image matches.
						LL_I2C_WriteCR1(I2Cx, cr1TargetImage);

						//! Both final bytes are already latched, so the reads are
						//! taken back to back exactly as RM0008 orders them.
						pData[byteIndex] = (i2c_data_t) LL_I2C_ReadDR(I2Cx);
						byteIndex++;
						pData[byteIndex] = (i2c_data_t) LL_I2C_ReadDR(I2Cx);

						_I2C_ExitCritical(priorPrimask);
					}
				}
				else
				{
					//! EV7_2: BTF proves the second-to-last byte sits in DR while
					//! the next-to-last sleeps in the shift register, with the
					//! bus stretched so no further byte can be received yet.
					transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_BYTE_TRANSFER_FINISHED, &pollBudget, NULL);
					if (transactionStatus == DRIVER_STATUS_SUCCESS)
					{
						//! Snapshot `CR1` and stage both ordered tail images
						//! before the window: clearing ACK and requesting STOP
						//! are pure Codec staging over local images, so the
						//! masked sequence is fallible-free register traffic.
						cr1RegImage = LL_I2C_ReadCR1(I2Cx);
						cr1TargetImage = cr1RegImage;
						ASSERT_DRIVER_STATUS(Codec_I2C_StageAck(&cr1TargetImage, 0U));
						cr1NackImage = cr1TargetImage;
						ASSERT_DRIVER_STATUS(Codec_I2C_StageStop(&cr1TargetImage));

						priorPrimask = _I2C_EnterCritical();

						//! Ordinary RW bit: skipped when the staged image
						//! already matches, in which case the NACK is in place.
						if (cr1RegImage != cr1NackImage)
						{
							LL_I2C_WriteCR1(I2Cx, cr1NackImage);
						}

						//! First consumed byte of the tail: reading DR while
						//! stretched also transfers the next-to-last byte into
						//! DR and launches the final byte's reception.
						pData[byteIndex] = (i2c_data_t) LL_I2C_ReadDR(I2Cx);
						byteIndex++;

						//! STOP is an action request, so this write is
						//! unconditional even when the staged image matches.
						LL_I2C_WriteCR1(I2Cx, cr1TargetImage);

						//! Second consumed byte of the tail, read exactly where
						//! RM0008 places it: just after the STOP request.
						pData[byteIndex] = (i2c_data_t) LL_I2C_ReadDR(I2Cx);

						_I2C_ExitCritical(priorPrimask);

						//! EV7: the final NACKed byte arrives after the window
						//! closes, so its consumption is bounded and unmasked.
						if (transactionStatus == DRIVER_STATUS_SUCCESS)
						{
							transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_DATA_REGISTER_FULL, &pollBudget, NULL);
							if (transactionStatus == DRIVER_STATUS_SUCCESS)
							{
								byteIndex++;
								pData[byteIndex] = (i2c_data_t) LL_I2C_ReadDR(I2Cx);
							}
						}
					}
				}
			}
		}
	}

	//! Every successful tail leaves STOP requested; the bounded idle wait proves
	//! hardware actually released the lines before success is published.
	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		transactionStatus = _I2C_WaitForEvent(I2Cx, I2C_EVENT_FLAG_BUS_IDLE, &pollBudget, NULL);
	}

	//! Restore the baseline only after the idle proof, because a pending STOP
	//! request forbids the CR1 write the restore performs.
	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		transactionStatus = _I2C_RestoreBaseline(I2Cx);
	}

	if (transactionStatus == DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_SUCCESS;
	}

	//! Single cleanup path for every post-START failure: it preserves the primary
	//! cause, releases the bus when the hardware still allows it, clears only the
	//! error flags it observed, and restores the ACK/POS baseline when permitted.
	return _I2C_AcknowledgeErrors(I2Cx, transactionStatus, &pollBudget);
}
