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
// I2C Helper Functions
// ==================================================================================================== //

/**
 * @brief Generates a START condition on the I2C bus
 * @details
 * Stages a START condition in the I2C control register and waits for the start
 * condition to be detected. The function polls the status register until the
 * start condition is detected or the timeout expires.
 *
 * @param[in] I2Cx			I2C peripheral instance
 * @param[in] timeout_ms	Timeout in milliseconds
 * @returns @ref driver_status_t "I2C start generation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: START condition was generated successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: Timeout occurred while waiting for start condition
 */
__STATIC driver_status_t _I2C_GenerateStart(I2C_TypeDef* const I2Cx, const uint32_t timeout_ms)
{
	// Local Variables
	reg cr1RegImage = 0x00000000UL;
	uint32_t pollBudget = (timeout_ms * I2C_DRIVER_POLL_BUDGET_PER_MS);

	//! Read current CR1 register image, stage a START condition, and write it back to the hardware.
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);
	ASSERT_DRIVER_STATUS(Codec_I2C_StageStart(&cr1RegImage));
	LL_I2C_WriteCR1(I2Cx, cr1RegImage);

	//! Bounded Wait
	while (pollBudget > 0U)
	{
		//! Read SR1 to detect START Condition
		if (Codec_I2C_IsStartCondition(LL_I2C_ReadSR1(I2Cx)) == DRIVER_STATUS_SUCCESS)
		{
			return DRIVER_STATUS_SUCCESS;
		}
		pollBudget--;
	}

	return DRIVER_STATUS_ERROR_TIMEOUT;
}

/**
 * @brief Generates a STOP condition on the I2C bus
 * @details
 * Stages a STOP condition in the I2C control register and waits for the bus
 * to become idle. The function polls the status register until the bus is
 * no longer busy or the timeout expires.
 * @param[in] I2Cx			I2C peripheral instance
 * @param[in] timeout_ms	Timeout in milliseconds
 * @returns @ref driver_status_t "I2C stop generation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: STOP condition was generated successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: Timeout occurred while waiting for bus idle
 */
__STATIC driver_status_t _I2C_GenerateStop(I2C_TypeDef* const I2Cx, const uint32_t timeout_ms)
{
	// Local Variables
	reg cr1RegImage = 0x00000000UL;
	uint32_t pollBudget = (timeout_ms * I2C_DRIVER_POLL_BUDGET_PER_MS);

	//! Read current CR1 register image, stage a STOP condition, and write it back to the hardware.
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);
	ASSERT_DRIVER_STATUS(Codec_I2C_StageStop(&cr1RegImage));
	LL_I2C_WriteCR1(I2Cx, cr1RegImage);

	//! Bounded Wait
	while (pollBudget > 0U)
	{
		//! Read SR2 for BUSY bit to detect if the bus is idle
		if (Codec_I2C_IsBusBusy(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS)
		{
			return DRIVER_STATUS_SUCCESS;
		}
		pollBudget--;
	}

	return DRIVER_STATUS_ERROR_TIMEOUT;
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