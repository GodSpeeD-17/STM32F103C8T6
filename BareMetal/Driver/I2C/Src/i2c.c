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
#include "stm32f1xx_i2c.h"
#include "stm32f1xx_rcc.h"
#include "i2c.h"
#include "i2c_codec.h"
#include "i2c_ll.h"
#include "rcc.h"

/** @brief Poll budget per millisecond for I2C driver operations @def I2C_DRIVER_POLL_BUDGET_PER_MS */
#define I2C_DRIVER_POLL_BUDGET_PER_MS		((uint32_t) 100UL)

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
static reg _I2C_GetAPB1ClockEnableMask(const I2C_TypeDef* const I2Cx)
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
static reg _I2C_GetAPB1PeripheralResetMask(const I2C_TypeDef* const I2Cx)
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

driver_status_t I2C_Init(I2C_TypeDef* const I2Cx, const i2c_config_t* const pConfig)
{
	// Local Variables
	reg temp = 0x00UL;
	reg cr1RegImage = 0x00UL;
	reg cr2RegImage = 0x00UL;
	reg ccrRegImage = 0x00UL;
	reg triseRegImage = 0x00UL;
	frequency_t apb1BusFrequency = 0x00UL;

	//! Validate Input
	if((I2Cx == NULL) || (pConfig == NULL))
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	//! Validate the range for input parameters
	if (I2C_IS_VALID_INSTANCE(I2Cx) == 0U)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}
	if ((I2C_IS_VALID_MODE(pConfig->busMode) == 0U) || (I2C_IS_VALID_FAST_DUTY(pConfig->fastModeDuty) == 0U))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! Enable Clock for Peripheral
	temp = _I2C_GetAPB1ClockEnableMask(I2Cx);
	if (temp == 0x00000000UL)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}
	else
	{
		ASSERT_DRIVER_STATUS(RCC_SetPeripheralClockState(RCC_APB1_BUS, temp, DRIVER_STATUS_ON));
	}

	//! Reset Peripheral
	temp = _I2C_GetAPB1PeripheralResetMask(I2Cx);
	if (temp == 0x00000000UL)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}
	else
	{
		ASSERT_DRIVER_STATUS(RCC_PulsePeripheralReset(RCC_APB1_BUS, temp));
	}

	//! Disable the Peripheral (PE=0)
	LL_I2C_WriteCR1(I2Cx, 0x00000000UL);

	//! Verify that the bus is not busy
	if (Codec_I2C_IsBusBusy(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS)
	{
		return DRIVER_STATUS_ERROR_BUSY;
	}

	//! Retrieve the APB1 Bus Frequency
	apb1BusFrequency = RCC_GetBusFrequency(RCC_APB1_BUS);
	if (apb1BusFrequency == RCC_FREQ_ZERO)
	{
		return DRIVER_STATUS_ERROR_STATE;
	}

	//! Snapshot all timing registers once
	cr2RegImage = LL_I2C_ReadCR2(I2Cx);
	ccrRegImage = LL_I2C_ReadCCR(I2Cx);
	triseRegImage = LL_I2C_ReadTRISE(I2Cx);
	cr1RegImage = LL_I2C_ReadCR1(I2Cx);

	//! Stage timing configuration (CR2.FREQ, CCR, TRISE) using the Codec
	ASSERT_DRIVER_STATUS(Codec_I2C_StageTiming(&cr2RegImage, &ccrRegImage, &triseRegImage, pConfig, apb1BusFrequency));

	//! Stage master-I2C baseline into CR1 (SMBUS=0, ACK=1, POS=0, clock stretching enabled)
	ASSERT_DRIVER_STATUS(Codec_I2C_StageBaseline(&cr1RegImage, 0U));

	//! Commit registers in the correct order while PE=0: CR2, CCR, TRISE, then CR1
	LL_I2C_WriteCR2(I2Cx, cr2RegImage);
	LL_I2C_WriteCCR(I2Cx, ccrRegImage);
	LL_I2C_WriteTRISE(I2Cx, triseRegImage);
	LL_I2C_WriteCR1(I2Cx, cr1RegImage);

	//! Leave PE=0; the application explicitly enables operation afterward
	return DRIVER_STATUS_SUCCESS;
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
static driver_status_t _I2C_GenerateStart(I2C_TypeDef* const I2Cx, const uint32_t timeout_ms)
{
	reg cr1RegImage = 0x00000000UL;
	uint32_t pollBudget = (timeout_ms * I2C_DRIVER_POLL_BUDGET_PER_MS);

	cr1RegImage = LL_I2C_ReadCR1(I2Cx);
	ASSERT_DRIVER_STATUS(Codec_I2C_StageStart(&cr1RegImage));
	LL_I2C_WriteCR1(I2Cx, cr1RegImage);

	while (Codec_I2C_IsStartCondition(LL_I2C_ReadSR1(I2Cx)) != DRIVER_STATUS_SUCCESS)
	{
		if (pollBudget == 0U)
		{
			return DRIVER_STATUS_ERROR_TIMEOUT;
		}
		pollBudget--;
	}

	return DRIVER_STATUS_SUCCESS;
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
static driver_status_t _I2C_GenerateStop(I2C_TypeDef* const I2Cx, const uint32_t timeout_ms)
{
	reg cr1RegImage = 0x00000000UL;
	uint32_t pollBudget = (timeout_ms * I2C_DRIVER_POLL_BUDGET_PER_MS);

	cr1RegImage = LL_I2C_ReadCR1(I2Cx);
	ASSERT_DRIVER_STATUS(Codec_I2C_StageStop(&cr1RegImage));
	LL_I2C_WriteCR1(I2Cx, cr1RegImage);

	while (Codec_I2C_IsBusBusy(LL_I2C_ReadSR2(I2Cx)) == DRIVER_STATUS_SUCCESS)
	{
		if (pollBudget == 0U)
		{
			return DRIVER_STATUS_ERROR_TIMEOUT;
		}
		pollBudget--;
	}

	return DRIVER_STATUS_SUCCESS;
}