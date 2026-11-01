/**
 * @file i2c_codec.c
 * @author Shrey Shah
 * @brief I2C Codec Layer Implementation
 * @version v1.0
 * @date 2026-08-28
 * @details
 * Pure transformations over caller-owned reg images. No MMIO, polling,
 * topology, clock, reset, NVIC, or sequencing work.
 */

// Header File Includes
#include "i2c_codec.h"

// ==================================================================================================== //
// I2C Operation State Codec
// ==================================================================================================== //

driver_status_t Codec_I2C_ExtractOperationState(const reg cr1RegImage, driver_status_t* const pOperationState)
{
	// Validate Output Pointer
	if (pOperationState == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	// Extract PE bit
	if ((cr1RegImage & I2C_CR1_PE) != 0x00000000UL)
	{
		*pOperationState = DRIVER_STATUS_ON;
	}
	else
	{
		*pOperationState = DRIVER_STATUS_OFF;
	}

	return DRIVER_STATUS_SUCCESS;
}

driver_status_t Codec_I2C_StageOperationState(reg* const pCr1RegImage, const driver_status_t operationState)
{
	// Validate Pointer
	if (pCr1RegImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	// Stage PE bit
	if (operationState == DRIVER_STATUS_ON)
	{
		*pCr1RegImage |= I2C_CR1_PE;
	}
	else
	{
		*pCr1RegImage &= ~I2C_CR1_PE;
	}

	return DRIVER_STATUS_SUCCESS;
}

// ==================================================================================================== //
// I2C Timing Codec
// ==================================================================================================== //

driver_status_t Codec_I2C_StageTiming
(
	reg* const						pCr2RegImage,
	reg* const						pCcrRegImage,
	reg* const						pTriseRegImage,
	const i2c_config_t* const		pConfig,
	const frequency_t				apb1FrequencyHz
)
{
	// Local Variables
	reg freqMHz = 0U;
	reg sclHz = 0U;
	reg ccrValue = 0U;
	reg triseValue = 0U;
	uint8_t isFastMode = 0U;

	// Validate Pointers
	if ((pCr2RegImage == NULL) || (pCcrRegImage == NULL) || (pTriseRegImage == NULL) || (pConfig == NULL))
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	// Validate Speed Preset
	if (!I2C_IS_VALID_SPEED_PRESET(pConfig->bus_speed))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	// Validate Fast Duty when Fast mode
	if (pConfig->bus_speed == I2C_SPEED_PRESET_400_KHZ)
	{
		if (!I2C_IS_VALID_FAST_DUTY(pConfig->fast_mode_duty))
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
		isFastMode = 1U;
	}

	// APB1 must be an integer MHz value
	if ((apb1FrequencyHz % 1000000U) != 0U)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	freqMHz = (reg)(apb1FrequencyHz / 1000000U);

	// Range check
	if (isFastMode != 0U)
	{
		if ((freqMHz < 4U) || (freqMHz > 36U))
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
	}
	else
	{
		if ((freqMHz < 2U) || (freqMHz > 36U))
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
	}

	// Determine SCL target
	if (pConfig->bus_speed == I2C_SPEED_PRESET_88_KHZ)
	{
		sclHz = 88000U;
	}
	else if (pConfig->bus_speed == I2C_SPEED_PRESET_100_KHZ)
	{
		sclHz = 100000U;
	}
	else
	{
		sclHz = 400000U;
	}

	// Ceiling-division: add (divisor - 1) before integer division so a non-exact
	// quotient rounds up to the next whole CCR step rather than truncating down,
	// which would otherwise make SCL faster than the requested bus speed.
	if (isFastMode == 0U)
	{
		// Standard mode: CCR = ceil(PCLK1 / (2 * SCL))
		ccrValue = (reg)(((apb1FrequencyHz + (2U * sclHz) - 1U) / (2U * sclHz)));
		if (ccrValue < 4U)
		{
			ccrValue = 4U;
		}
		if (ccrValue > 4095U)
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
		triseValue = freqMHz + 1U;
	}
	else
	{
		if (pConfig->fast_mode_duty == I2C_FAST_DUTY_2_1)
		{
			// CCR = ceil(PCLK1 / (3 * SCL))
			ccrValue = (reg)(((apb1FrequencyHz + (3U * sclHz) - 1U) / (3U * sclHz)));
			if (ccrValue < 4U)
			{
				ccrValue = 4U;
			}
		}
		else
		{
			// CCR = ceil(PCLK1 / (25 * SCL))
			ccrValue = (reg)(((apb1FrequencyHz + (25U * sclHz) - 1U) / (25U * sclHz)));
			if (ccrValue < 1U)
			{
				ccrValue = 1U;
			}
		}
		if (ccrValue > 4095U)
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
		triseValue = (reg)(((freqMHz * 300U) / 1000U) + 1U);
	}

	// Stage CR2.FREQ (preserve other bits)
	*pCr2RegImage &= ~I2C_CR2_FREQ;
	*pCr2RegImage |= ((freqMHz & 0x3FU) << I2C_CR2_FREQ_Pos);

	// Stage CCR
	*pCcrRegImage &= ~(I2C_CCR_FS | I2C_CCR_DUTY | I2C_CCR_CCR);
	if (isFastMode != 0U)
	{
		*pCcrRegImage |= I2C_CCR_FS;
		if (pConfig->fast_mode_duty == I2C_FAST_DUTY_16_9)
		{
			*pCcrRegImage |= I2C_CCR_DUTY;
		}
	}
	*pCcrRegImage |= ((ccrValue & 0x0FFFU) << I2C_CCR_CCR_Pos);

	// Stage TRISE
	*pTriseRegImage &= ~I2C_TRISE_TRISE;
	*pTriseRegImage |= ((triseValue & 0x3FU) << I2C_TRISE_TRISE_Pos);

	return DRIVER_STATUS_SUCCESS;
}

// ==================================================================================================== //
// I2C CR1 Baseline Codec
// ==================================================================================================== //

driver_status_t Codec_I2C_StageBaseline(reg* const pCr1RegImage, const uint8_t disableClockStretching)
{
	// Validate Pointer
	if (pCr1RegImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	// Master I2C baseline: SMBUS=0, ACK=1, POS=0
	*pCr1RegImage &= ~I2C_CR1_SMBUS;
	*pCr1RegImage |= I2C_CR1_ACK;
	*pCr1RegImage &= ~I2C_CR1_POS;

	// Clock stretching control
	if (disableClockStretching != 0U)
	{
		*pCr1RegImage |= I2C_CR1_NOSTRETCH;
	}
	else
	{
		*pCr1RegImage &= ~I2C_CR1_NOSTRETCH;
	}

	return DRIVER_STATUS_SUCCESS;
}

// ==================================================================================================== //
// I2C Address Codec
// ==================================================================================================== //

uint8_t Codec_I2C_EncodeAddressByte(const i2c_address_t address, const uint8_t isRead)
{
	return (uint8_t)(((address & 0x7FU) << 1U) | (isRead != 0U ? 0x01U : 0x00U));
}

// ==================================================================================================== //
// I2C CR1 Action Codec
// ==================================================================================================== //

driver_status_t Codec_I2C_StageStart(reg* const pCr1RegImage)
{
	if (pCr1RegImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	*pCr1RegImage |= I2C_CR1_START;
	return DRIVER_STATUS_SUCCESS;
}

driver_status_t Codec_I2C_StageStop(reg* const pCr1RegImage)
{
	if (pCr1RegImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	*pCr1RegImage |= I2C_CR1_STOP;
	return DRIVER_STATUS_SUCCESS;
}

driver_status_t Codec_I2C_StageAck(reg* const pCr1RegImage, const uint8_t ack)
{
	if (pCr1RegImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if (ack != 0U)
	{
		*pCr1RegImage |= I2C_CR1_ACK;
	}
	else
	{
		*pCr1RegImage &= ~I2C_CR1_ACK;
	}
	return DRIVER_STATUS_SUCCESS;
}

driver_status_t Codec_I2C_StagePos(reg* const pCr1RegImage, const uint8_t pos)
{
	if (pCr1RegImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if (pos != 0U)
	{
		*pCr1RegImage |= I2C_CR1_POS;
	}
	else
	{
		*pCr1RegImage &= ~I2C_CR1_POS;
	}
	return DRIVER_STATUS_SUCCESS;
}

driver_status_t Codec_I2C_StageSoftwareReset(reg* const pCr1RegImage, const uint8_t reset)
{
	if (pCr1RegImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if (reset != 0U)
	{
		*pCr1RegImage |= I2C_CR1_SWRST;
	}
	else
	{
		*pCr1RegImage &= ~I2C_CR1_SWRST;
	}
	return DRIVER_STATUS_SUCCESS;
}

// ==================================================================================================== //
// I2C SR1 Error Codec
// ==================================================================================================== //

driver_status_t Codec_I2C_ExtractError(const reg sr1RegImage, i2c_error_t* const pError)
{
	if (pError == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	if ((sr1RegImage & I2C_SR1_BERR) != 0U)
	{
		*pError = I2C_ERROR_BUS_ERROR;
	}
	else if ((sr1RegImage & I2C_SR1_ARLO) != 0U)
	{
		*pError = I2C_ERROR_ARBITRATION_LOST;
	}
	else if ((sr1RegImage & I2C_SR1_AF) != 0U)
	{
		*pError = I2C_ERROR_DATA_NACK;
	}
	else if ((sr1RegImage & I2C_SR1_OVR) != 0U)
	{
		*pError = I2C_ERROR_OVERRUN;
	}
	else if ((sr1RegImage & I2C_SR1_PECERR) != 0U)
	{
		*pError = I2C_ERROR_BUS_ERROR;
	}
	else if ((sr1RegImage & I2C_SR1_TIMEOUT) != 0U)
	{
		*pError = I2C_ERROR_HARDWARE_TIMEOUT;
	}
	else if ((sr1RegImage & I2C_SR1_SMBALERT) != 0U)
	{
		*pError = I2C_ERROR_BUS_ERROR;
	}
	else
	{
		*pError = I2C_ERROR_NONE;
	}

	return DRIVER_STATUS_SUCCESS;
}

driver_status_t Codec_I2C_StageClearErrors(reg* const pWriteImage, const reg sr1Snapshot, const reg errorsToClear)
{
	if (pWriteImage == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	*pWriteImage = sr1Snapshot & ~errorsToClear;

	return DRIVER_STATUS_SUCCESS;
}
