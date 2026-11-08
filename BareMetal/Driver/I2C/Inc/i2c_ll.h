/**
 * @file	i2c_ll.h
 * @author	Shrey Shah
 * @brief	I2C Low-Level Control Interface
 * @version	v2.0
 * @date	31-10-2026
 *
 * @details
 * This header defines the I2C low-level module that sits directly above the
 * register-layer definitions from @ref `stm32f1xx_i2c.h`.
 *
 * LL authority is intentionally narrow. It owns dumb I2C register access:
 * - static inline read/write accessors for I2C register images
 * - the minimal register-address macro needed to select a C struct member
 * - enabling or disabling I2C-related APB1 clock gates through RCC LL
 *
 * This module does not own:
 * - I2C selector vocabulary definitions
 * - public selector to raw field translation
 * - CCR/TRISE bitfield placement, packing, or decoding
 * - protocol configuration (speed mode, duty cycle, ACK, clock stretching)
 * - public driver policy validation
 * - staged batching policy
 * - board-specific behavior
 *
 * Practical Authority Rule:
 * - Layer 0 (`stm32f1xx_i2c.h`) defines raw register symbols and register map.
 * - LL exposes named full-register accessors and thin clock forwarding only.
 * - Codec APIs own selector translation and CR1/CR2/OAR1/OAR2/DR/SR1/SR2/CCR/TRISE image mutation.
 * - Driver APIs (`i2c.h` / `i2c.c`) own public validation, sequencing, and
 *   user-facing status.
 *
 * @note Functions in this file do not validate @ref `I2C_TypeDef` pointers, do
 * not check clock state, and do not preserve unrelated register bits unless the
 * caller supplies a pre-staged full register image.
 */

// Header Guards
#ifndef LL_I2C_H_
#define LL_I2C_H_

// ==================================================================================================== //
//												Includes												//
// ==================================================================================================== //
#include "stm32f1xx.h"
#include "rcc_ll.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup I2C_02_LL
 * @{
 */

// ==================================================================================================== //
//										I2C LL Register Access APIs									//
// ==================================================================================================== //

/**
 * @brief	I2C LL Register Access APIs
 * @defgroup I2C_02_LL_01_RegisterOps I2C LL Register Access APIs
 * @ingroup	I2C_02_LL
 * @details
 * These functions are intentionally dumb full-register image readers and
 * writers. They do not know protocol selectors, bitfield layout, or
 * public I2C configuration vocabulary.
 *
 * Practical Rule:
 * - Always access I2C registers through the `.REG` member.
 * - Do not use the `.BIT` view in the LL layer.
 * - Use named static inline accessors in driver code.
 * - Read-modify-write policy belongs to the caller, not to this LL layer.
 *
 * @warning These accessors perform no NULL, peripheral-validity, clock, reserved-bit,
 * or write-permission checks.
 * @{
 */

/**
 * @brief Returns pointer to I2C register `.REG` image
 * @def LL_I2C_REG
 * @param[in]	_I2Cx	Target I2C peripheral instance expression
 * @param[in]	_REG	Register member token inside @ref `I2C_TypeDef`
 * @returns Pointer to the selected I2C register `.REG` image.
 * @note Thin I2C LL alias over @ref `REGOPS_REG`.
 */
#define LL_I2C_REG(_I2Cx, _REG)				REGOPS_REG((_I2Cx), _REG)

/**
 * @brief Reads one I2C register image by register pointer
 * @param[in]	pRegister	Pointer to an I2C `.REG` member
 * @returns Full 32-bit register image currently observed at @p pRegister.
 * @note Caller must pass a valid I2C register pointer.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadRegister(const _IO* const pRegister)
{
	// Local Variable
	reg registerImage = 0x00000000UL;
	//! Read the full 32-bit register image into caller-owned storage.
	(void) RegOps_Read(pRegister, &registerImage);
	//! Return the full 32-bit register image to the caller.
	return registerImage;
}

/**
 * @brief Writes one I2C register image by register pointer
 * @param[in]	pRegister		Pointer to a writable I2C `.REG` member
 * @param[in]	registerImage	Full 32-bit register image to write
 * @note Caller must pass a valid writable I2C register pointer and a complete
 * register image appropriate for that register.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteRegister(_IO* const pRegister, const reg registerImage)
{
	//! Forward the complete image without adding register-specific policy.
	(void) RegOps_Write(pRegister, registerImage);
}

/**
 * @brief Reads the I2C `CR1` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_CR1` image.
 * @note `CR1` controls peripheral enable, ACK, clock stretching, and other
 * protocol settings. Field extraction and interpretation are codec responsibilities.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadCR1(I2C_TypeDef* const I2Cx)
{
	//! Address the CR1 union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, CR1));
}

/**
 * @brief Writes the I2C `CR1` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_CR1` image to write
 * @note Caller owns read-modify-write staging and field placement.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteCR1(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged CR1 image without modifying unrelated fields.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, CR1), registerImage);
}

/**
 * @brief Reads the I2C `CR2` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_CR2` image.
 * @note `CR2` controls DMA, interrupts, and slave address. Field extraction
 * and interpretation are codec responsibilities.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadCR2(I2C_TypeDef* const I2Cx)
{
	//! Address the CR2 union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, CR2));
}

/**
 * @brief Writes the I2C `CR2` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_CR2` image to write
 * @note Caller owns read-modify-write staging and field placement.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteCR2(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged CR2 image without modifying unrelated fields.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, CR2), registerImage);
}

/**
 * @brief Reads the I2C `OAR1` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_OAR1` image.
 * @note `OAR1` stores the primary own address. Field extraction and
 * interpretation are codec responsibilities.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadOAR1(I2C_TypeDef* const I2Cx)
{
	//! Address the OAR1 union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, OAR1));
}

/**
 * @brief Writes the I2C `OAR1` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_OAR1` image to write
 * @note Caller owns read-modify-write staging and field placement.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteOAR1(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged OAR1 image without modifying unrelated fields.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, OAR1), registerImage);
}

/**
 * @brief Reads the I2C `OAR2` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_OAR2` image.
 * @note `OAR2` stores the secondary own address. Field extraction and
 * interpretation are codec responsibilities.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadOAR2(I2C_TypeDef* const I2Cx)
{
	//! Address the OAR2 union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, OAR2));
}

/**
 * @brief Writes the I2C `OAR2` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_OAR2` image to write
 * @note Caller owns read-modify-write staging and field placement.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteOAR2(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged OAR2 image without modifying unrelated fields.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, OAR2), registerImage);
}

/**
 * @brief Reads the I2C `DR` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_DR` image.
 * @note `DR` is the data register used for transmit and receive. Data
 * interpretation belongs above LL.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadDR(I2C_TypeDef* const I2Cx)
{
	//! Address the DR union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, DR));
}

/**
 * @brief Writes the I2C `DR` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_DR` image to write
 * @note Caller owns data staging and interpretation.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteDR(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged DR image without modifying unrelated fields.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, DR), registerImage);
}

/**
 * @brief Reads the I2C `SR1` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_SR1` image.
 * @note `SR1` contains status flags. Flag extraction and interpretation are
 * codec responsibilities.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadSR1(I2C_TypeDef* const I2Cx)
{
	//! Address the SR1 union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, SR1));
}

/**
 * @brief Writes the I2C `SR1` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_SR1` image to write
 * @note Some SR1 bits are write-to-clear. Caller owns the correct write sequence.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteSR1(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged SR1 image so the caller's write sequence stays intact.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, SR1), registerImage);
}

/**
 * @brief Reads the I2C `SR2` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_SR2` image.
 * @note `SR2` contains additional status flags. Flag extraction and interpretation
 * are codec responsibilities. SR2 is hardware-populated and read-only; no write accessor exists.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadSR2(I2C_TypeDef* const I2Cx)
{
	//! Address the SR2 union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, SR2));
}

/**
 * @brief Reads the I2C `CCR` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_CCR` image.
 * @note `CCR` controls clock frequency. Field extraction and interpretation are
 * codec responsibilities.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadCCR(I2C_TypeDef* const I2Cx)
{
	//! Address the CCR union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, CCR));
}

/**
 * @brief Writes the I2C `CCR` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_CCR` image to write
 * @note Caller owns read-modify-write staging and field placement.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteCCR(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged CCR image without modifying unrelated fields.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, CCR), registerImage);
}

/**
 * @brief Reads the I2C `TRISE` register image
 * @param[in]	I2Cx	I2C peripheral instance
 * @returns Full `I2Cx_TRISE` image.
 * @note `TRISE` controls rise time. Field extraction and interpretation are
 * codec responsibilities.
 */
__STATIC_FORCEINLINE_LL reg LL_I2C_ReadTRISE(I2C_TypeDef* const I2Cx)
{
	//! Address the TRISE union image through the shared register-pointer accessor.
	return LL_I2C_ReadRegister(LL_I2C_REG(I2Cx, TRISE));
}

/**
 * @brief Writes the I2C `TRISE` register image
 * @param[in]	I2Cx			I2C peripheral instance
 * @param[in]	registerImage	Full `I2Cx_TRISE` image to write
 * @note Caller owns read-modify-write staging and field placement.
 */
__STATIC_FORCEINLINE void LL_I2C_WriteTRISE(I2C_TypeDef* const I2Cx, const reg registerImage)
{
	//! Forward the staged TRISE image without modifying unrelated fields.
	LL_I2C_WriteRegister(LL_I2C_REG(I2Cx, TRISE), registerImage);
}

/** @} */ // I2C_02_LL_01_RegisterOps

// ==================================================================================================== //
//										  I2C LL Clock APIs											//
// ==================================================================================================== //

/**
 * @brief	I2C LL Clock APIs
 * @defgroup I2C_02_LL_02_Clock I2C LL Clock APIs
 * @ingroup	I2C_02_LL
 * @details
 * These APIs resolve I2C-related APB1 clock gates and forward the raw
 * `RCC_APB1ENR` bit masks to RCC LL.
 *
 * Practical Rule:
 * - I2C peripheral clock APIs accept a @ref `I2C_TypeDef` pointer and resolve it to
 *   the matching `RCC_APB1ENR_I2CxEN` bit.
 * - This section only performs the requested clock-gate action. Driver policy
 *   decides when I2C clocks are required.
 * @{
 */

/**
 * @brief Resolves an I2C peripheral instance to the matching APB1 clock-gate mask
 * @param[in]	I2Cx	I2C peripheral instance
 * Accepted values:
 * - @ref `I2C1`: I2C peripheral 1 instance
 * - @ref `I2C2`: I2C peripheral 2 instance
 * @returns I2C peripheral APB1 clock-gate mask resolved from @p `I2Cx`
 * @retval - @ref `RCC_APB1ENR_I2C1EN`: I2C1 APB1 clock-gate mask
 * @retval - @ref `RCC_APB1ENR_I2C2EN`: I2C2 APB1 clock-gate mask
 * @retval - `0x00000000UL`: @p `I2Cx` did not map to a supported I2C peripheral
 */
__STATIC_FORCEINLINE reg LL_I2C_GetPortClockMask(const I2C_TypeDef* const I2Cx)
{
	// Local Variable
	reg clockGateMask = 0x00000000UL;

	//! Match by peripheral base address because instance macros are raw memory-mapped pointers.
	switch ((reg) I2Cx)
	{
		case I2C1_BASE_ADDRESS:
		{
			clockGateMask = RCC_APB1ENR_I2C1EN;
			break;
		}
		case I2C2_BASE_ADDRESS:
		{
			clockGateMask = RCC_APB1ENR_I2C2EN;
			break;
		}
		default:
		{
			clockGateMask = 0x00000000UL;
			break;
		}
	}

	//! Publish the resolved mask, or the zero sentinel for an unsupported peripheral.
	return clockGateMask;
}

/**
 * @brief Enables the APB1 clock gate for one I2C peripheral
 * @details
 * This is a dumb single-point forwarder. It resolves @p `I2Cx` to its
 * `RCC_APB1ENR_I2CxEN` mask through @ref `LL_I2C_GetPortClockMask` and
 * delegates the actual gate transition to RCC LL; it performs no other
 * validation, sequencing, or clock-gate policy of its own.
 * @param[in]	I2Cx	I2C peripheral instance
 * Accepted values:
 * - @ref `I2C1`: I2C peripheral 1 instance
 * - @ref `I2C2`: I2C peripheral 2 instance
 * @returns @ref driver_status_t "I2C peripheral APB1 clock-gate enable status forwarded from RCC LL"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: I2C peripheral APB1 clock gate was enabled
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx` did not resolve to a valid APB1 clock-gate mask
 */
__STATIC_FORCEINLINE driver_status_t LL_I2C_EnableClock(const I2C_TypeDef* const I2Cx)
{
	//! Resolve the peripheral-specific mask so RCC LL can validate and gate the correct APB1 bit.
	const reg clockMask = LL_I2C_GetPortClockMask(I2Cx);
	return LL_RCC_EnableAPB1Clock(clockMask);
}

/**
 * @brief Disables the APB1 clock gate for one I2C peripheral
 * @details
 * This is a dumb single-point forwarder. It resolves @p `I2Cx` to its
 * `RCC_APB1ENR_I2CxEN` mask through @ref `LL_I2C_GetPortClockMask` and
 * delegates the actual gate transition to RCC LL; it performs no other
 * validation, sequencing, or clock-gate policy of its own.
 * @param[in]	I2Cx	I2C peripheral instance
 * Accepted values:
 * - @ref `I2C1`: I2C peripheral 1 instance
 * - @ref `I2C2`: I2C peripheral 2 instance
 * @returns @ref driver_status_t "I2C peripheral APB1 clock-gate disable status forwarded from RCC LL"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: I2C peripheral APB1 clock gate was disabled
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx` did not resolve to a valid APB1 clock-gate mask
 */
__STATIC_FORCEINLINE driver_status_t LL_I2C_DisableClock(const I2C_TypeDef* const I2Cx)
{
	//! Resolve the peripheral-specific mask so RCC LL can validate and gate the correct APB1 bit.
	const reg clockMask = LL_I2C_GetPortClockMask(I2Cx);
	return LL_RCC_DisableAPB1Clock(clockMask);
}

/** @} */ // I2C_02_LL_02_Clock

/** @} */ // I2C_02_LL

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LL_I2C_H_ */
