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
// I2C Codec Local Macros
// ==================================================================================================== //

/** @brief 7-bit address mask used when encoding an address byte @def I2C_CODEC_ADDRESS_MASK_7BIT */
#define I2C_CODEC_ADDRESS_MASK_7BIT				((uint8_t) 0x7FU)
/** @brief Address-byte left-shift count @def I2C_CODEC_ADDRESS_SHIFT */
#define I2C_CODEC_ADDRESS_SHIFT					((uint8_t) 1U)
/** @brief Read/write bit mask in an address byte @def I2C_CODEC_ADDRESS_RW_BIT */
#define I2C_CODEC_ADDRESS_RW_BIT				((uint8_t) 0x01U)
/** @brief CR2.FREQ field mask @def I2C_CODEC_CR2_FREQ_MASK */
#define I2C_CODEC_CR2_FREQ_MASK					((reg) 0x3FU)
/** @brief CCR value field mask @def I2C_CODEC_CCR_VALUE_MASK */
#define I2C_CODEC_CCR_VALUE_MASK				((reg) 0x0FFFU)
/** @brief TRISE value field mask @def I2C_CODEC_TRISE_VALUE_MASK */
#define I2C_CODEC_TRISE_VALUE_MASK				((reg) 0x3FU)
/** @brief Hertz per megahertz @def I2C_CODEC_HZ_PER_MHZ */
#define I2C_CODEC_HZ_PER_MHZ					((frequency_t) 1000000U)
/** @brief Standard-mode minimum APB1 frequency in MHz @def I2C_CODEC_STD_MODE_MIN_FREQ_MHZ */
#define I2C_CODEC_STD_MODE_MIN_FREQ_MHZ			((reg) 2U)
/** @brief Fast-mode minimum APB1 frequency in MHz @def I2C_CODEC_FAST_MODE_MIN_FREQ_MHZ */
#define I2C_CODEC_FAST_MODE_MIN_FREQ_MHZ		((reg) 4U)
/** @brief Maximum APB1 frequency in MHz @def I2C_CODEC_MAX_FREQ_MHZ */
#define I2C_CODEC_MAX_FREQ_MHZ					((reg) 36U)
/** @brief Standard-mode minimum CCR value @def I2C_CODEC_STD_MODE_CCR_MIN */
#define I2C_CODEC_STD_MODE_CCR_MIN				((reg) 4U)
/** @brief Fast 2:1 mode minimum CCR value @def I2C_CODEC_FAST_2_1_CCR_MIN */
#define I2C_CODEC_FAST_2_1_CCR_MIN				((reg) 4U)
/** @brief Fast 16:9 mode minimum CCR value @def I2C_CODEC_FAST_16_9_CCR_MIN */
#define I2C_CODEC_FAST_16_9_CCR_MIN				((reg) 1U)
/** @brief Maximum CCR value @def I2C_CODEC_CCR_MAX */
#define I2C_CODEC_CCR_MAX						((reg) 4095U)
/** @brief 88 kHz SCL target in hertz @def I2C_CODEC_SCL_88_KHZ */
#define I2C_CODEC_SCL_88_KHZ					((reg) 88000U)
/** @brief 100 kHz SCL target in hertz @def I2C_CODEC_SCL_100_KHZ */
#define I2C_CODEC_SCL_100_KHZ					((reg) 100000U)
/** @brief 400 kHz SCL target in hertz @def I2C_CODEC_SCL_400_KHZ */
#define I2C_CODEC_SCL_400_KHZ					((reg) 400000U)
/** @brief SCL rise time in nanoseconds @def I2C_CODEC_SCL_RISE_TIME_NS */
#define I2C_CODEC_SCL_RISE_TIME_NS				((reg) 300U)
/** @brief Nanoseconds per microsecond @def I2C_CODEC_NS_PER_US */
#define I2C_CODEC_NS_PER_US						((reg) 1000U)
/** @brief Standard-mode CCR divisor @def I2C_CODEC_STD_MODE_CCR_DIVISOR */
#define I2C_CODEC_STD_MODE_CCR_DIVISOR			((reg) 2U)
/** @brief Fast 2:1 mode CCR divisor @def I2C_CODEC_FAST_2_1_CCR_DIVISOR */
#define I2C_CODEC_FAST_2_1_CCR_DIVISOR			((reg) 3U)
/** @brief Fast 16:9 mode CCR divisor @def I2C_CODEC_FAST_16_9_CCR_DIVISOR */
#define I2C_CODEC_FAST_16_9_CCR_DIVISOR			((reg) 25U)
/** @brief Zero constant @def I2C_CODEC_ZERO */
#define I2C_CODEC_ZERO							((reg) 0U)
/** @brief One constant @def I2C_CODEC_ONE */
#define I2C_CODEC_ONE							((reg) 1U)

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
	if ((apb1FrequencyHz % I2C_CODEC_HZ_PER_MHZ) != 0U)
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	freqMHz = (reg)(apb1FrequencyHz / I2C_CODEC_HZ_PER_MHZ);

	// Range check
	if (isFastMode != 0U)
	{
		if ((freqMHz < I2C_CODEC_FAST_MODE_MIN_FREQ_MHZ) || (freqMHz > I2C_CODEC_MAX_FREQ_MHZ))
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
	}
	else
	{
		if ((freqMHz < I2C_CODEC_STD_MODE_MIN_FREQ_MHZ) || (freqMHz > I2C_CODEC_MAX_FREQ_MHZ))
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
	}

	// Determine SCL target
	if (pConfig->bus_speed == I2C_SPEED_PRESET_88_KHZ)
	{
		sclHz = I2C_CODEC_SCL_88_KHZ;
	}
	else if (pConfig->bus_speed == I2C_SPEED_PRESET_100_KHZ)
	{
		sclHz = I2C_CODEC_SCL_100_KHZ;
	}
	else
	{
		sclHz = I2C_CODEC_SCL_400_KHZ;
	}

	// Ceiling-division: add (divisor - 1) before integer division so a non-exact
	// quotient rounds up to the next whole CCR step rather than truncating down,
	// which would otherwise make SCL faster than the requested bus speed.
	if (isFastMode == 0U)
	{
		// Standard mode: CCR = ceil(PCLK1 / (2 * SCL))
		ccrValue = (reg)(((apb1FrequencyHz + (I2C_CODEC_STD_MODE_CCR_DIVISOR * sclHz) - I2C_CODEC_ONE) / (I2C_CODEC_STD_MODE_CCR_DIVISOR * sclHz)));
		if (ccrValue < I2C_CODEC_STD_MODE_CCR_MIN)
		{
			ccrValue = I2C_CODEC_STD_MODE_CCR_MIN;
		}
		if (ccrValue > I2C_CODEC_CCR_MAX)
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
		triseValue = freqMHz + I2C_CODEC_ONE;
	}
	else
	{
		if (pConfig->fast_mode_duty == I2C_FAST_DUTY_2_1)
		{
			// CCR = ceil(PCLK1 / (3 * SCL))
			ccrValue = (reg)(((apb1FrequencyHz + (I2C_CODEC_FAST_2_1_CCR_DIVISOR * sclHz) - I2C_CODEC_ONE) / (I2C_CODEC_FAST_2_1_CCR_DIVISOR * sclHz)));
			if (ccrValue < I2C_CODEC_STD_MODE_CCR_MIN)
			{
				ccrValue = I2C_CODEC_STD_MODE_CCR_MIN;
			}
		}
		else
		{
			// CCR = ceil(PCLK1 / (25 * SCL))
			ccrValue = (reg)(((apb1FrequencyHz + (I2C_CODEC_FAST_16_9_CCR_DIVISOR * sclHz) - I2C_CODEC_ONE) / (I2C_CODEC_FAST_16_9_CCR_DIVISOR * sclHz)));
			if (ccrValue < I2C_CODEC_FAST_16_9_CCR_MIN)
			{
				ccrValue = I2C_CODEC_FAST_16_9_CCR_MIN;
			}
		}
		if (ccrValue > I2C_CODEC_CCR_MAX)
		{
			return DRIVER_STATUS_ERROR_INVALID_ARG;
		}
		triseValue = (reg)(((freqMHz * I2C_CODEC_SCL_RISE_TIME_NS) / I2C_CODEC_NS_PER_US) + I2C_CODEC_ONE);
	}

	// Stage CR2.FREQ (preserve other bits)
	*pCr2RegImage &= ~I2C_CR2_FREQ;
	*pCr2RegImage |= ((freqMHz & I2C_CODEC_CR2_FREQ_MASK) << I2C_CR2_FREQ_Pos);

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
	*pCcrRegImage |= ((ccrValue & I2C_CODEC_CCR_VALUE_MASK) << I2C_CCR_CCR_Pos);

	// Stage TRISE
	*pTriseRegImage &= ~I2C_TRISE_TRISE;
	*pTriseRegImage |= ((triseValue & I2C_CODEC_TRISE_VALUE_MASK) << I2C_TRISE_TRISE_Pos);

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
	return (uint8_t)(((address & I2C_CODEC_ADDRESS_MASK_7BIT) << I2C_CODEC_ADDRESS_SHIFT) | (isRead != 0U ? I2C_CODEC_ADDRESS_RW_BIT : I2C_CODEC_ZERO));
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
