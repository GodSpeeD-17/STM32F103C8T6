/**
 * @file stm32f1xx_utils.h
 * @author Shrey Shah
 * @brief Shared bit manipulation and register operation utilities
 * @version v1.2
 * @date 01-03-2026
 * @defgroup 01_STM32F1xx_Utilities Basic Utilities
 * @ingroup STM32F1xx
 * @details
 * Provides constant-expression helpers for constructing register fields,
 * direct register-operation macros, and checked inline helpers for staging and
 * accessing 32-bit memory-mapped registers.
 * @note @ref reg is the canonical type for register images, masks, and field
 * values throughout this module.
 * @warning Direct `REGOPS_xxx` macros do not validate register pointers. Use
 * the `RegOps_xxx` inline helpers when null-pointer status reporting is needed.
 */

// Header Guards
#ifndef STM32F1XX_UTILS_H_
#define STM32F1XX_UTILS_H_

// ==================================================================================================== //
//												Includes												//
// ==================================================================================================== //
#include "cmsis_gcc.h"
#include "stm32f1xx_data_types.h"

// --- C++ Safeguards ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// ==================================================================================================== //
//										LL Accessor Preservation										//
// ==================================================================================================== //
/**
 * @brief Forces an LL register accessor to persist as inlined, non-removable code
 * @def __STATIC_FORCEINLINE_LL
 * @details
 * Combines the `always_inline` and `used` GCC attributes with internal static
 * linkage so LL accessors both merge into every caller and are never discarded,
 * even when a call site's result is unused or when LTO prunes unreferenced
 * statics. The caller's optimization level governs the merged body; no separate
 * "-O0 island" exists for a single inlined function, which is why the `volatile`
 * qualification of the register memory remains the load-bearing guarantee: two
 * volatile accesses can never be removed, merged, or reordered relative to each
 * other, so ordered pair-clear sequences (`SR1` then `SR2`) and other
 * side-effecting reads survive at every optimization level. Use this instead of
 * plain @ref `__STATIC_FORCEINLINE` wherever each accessor must be auditable as
 *exactly one access plus a debuggable stepping site regardless of optimization.
 * @warning No attribute can preserve the call boundary *and* the caller's
 * optimization decisions simultaneously: if a discrete non-inlined call ever
 * becomes the requirement, switch the attribute set to
 * `__attribute__((noinline, used))` instead of weakening the access semantics.
 */
#define __STATIC_FORCEINLINE_LL	static __attribute__((always_inline, used)) inline

// ==================================================================================================== //
//											Bit Manipulation Utilities									//
// ==================================================================================================== //
/**
 * @defgroup 01_STM32F1xx_Utilities_01_Bit Bit Manipulation Utilities
 * @ingroup 01_STM32F1xx_Utilities
 * @{
 */

/**
 * @brief Shifts a raw value into a register field position
 * @def REG_FIELD_VALUE
 * @param[in] _FieldPosition Zero-based field position in the range `0U..31U`
 * @param[in] _FieldValue Right-aligned register field value
 * @returns Positioned register field value as @ref reg
 * @note @p _FieldPosition must be in the range `0U..31U`.
 * @note @p _FieldValue must already be masked to the intended field width.
 * @warning Bits shifted beyond bit `31U` are discarded.
 */
#define REG_FIELD_VALUE(_FieldPosition, _FieldValue)				\
	((reg) (((reg) (_FieldValue)) << ((reg_bit_pos_t) (_FieldPosition))))

/**
 * @brief Creates a contiguous field mask at a register bit position
 * @def REG_FIELD_MASK
 * @param[in] _Pos First field bit position in the range `0U..31U`
 * @param[in] _FieldWidth Number of field bits in the range `0U..31U`
 * @returns Positioned field mask as @ref reg
 * @warning Mask bits shifted beyond bit `31U` are discarded; the result always remains 32-bit.
 * @note Use `0xFFFFFFFFUL` when all 32 register bits are required.
 * @note For example, `REG_FIELD_MASK(4U, 3U)` produces `0x00000070UL`.
 */
#define REG_FIELD_MASK(_Pos, _FieldWidth)										\
	REG_FIELD_VALUE																	\
	(																				\
		(_Pos),																		\
		((reg) ((0x01UL << ((reg_field_width_t) (_FieldWidth))) - 0x01UL))			\
	)

/**
 * @brief Packs a right-aligned field value into register position
 * @def REG_FIELD_PACK
 * @see REG_FIELD_VALUE
 * @see REG_FIELD_MASK
 * @param[in] _Pos Zero-based field position in the range `0U..31U`
 * @param[in] _Val Right-aligned raw field value
 * @param[in] _FieldWidth Number of field bits in the range `0U..31U`
 * @returns Masked and positioned register field value as @ref reg
 * @warning Bits positioned beyond bit `31U` are discarded.
 */
#define REG_FIELD_PACK(_Pos, _Val, _FieldWidth)					\
	REG_FIELD_VALUE												\
	(															\
		((reg_bit_pos_t) (_Pos)),								\
		(((reg) (_Val)) & REG_FIELD_MASK(0U, (_FieldWidth)))	\
	)

/**
 * @brief Creates a single-bit mask at the requested bit position
 * @def REG_BIT_MASK
 * @see REG_FIELD_VALUE
 * @param[in] _Pos Zero-based bit position in the range `0U..31U`
 * @returns Register mask with only bit @p _Pos set
 */
#define REG_BIT_MASK(_Pos)								REG_FIELD_VALUE(((reg_bit_pos_t) (_Pos)), 0x01UL)

/**
 * @brief Calculates a zero-based peripheral index from address spacing
 * @def BIT_POS
 * @param[in] _Instance Target peripheral address or pointer
 * @param[in] _BaseInstance First peripheral address or pointer in the same family
 * @param[in] _InstanceSpacing Byte spacing between consecutive peripheral instances
 * @returns Zero-based peripheral index relative to @p _BaseInstance
 * @pre @p _Instance and @p _BaseInstance belong to the same equally spaced peripheral family.
 * @pre @p _InstanceSpacing is non-zero and matches the hardware memory-map spacing.
 * @warning Misaligned addresses or incorrect spacing produce an invalid index.
 * @note Constant operands allow the compiler to evaluate the expression at compile time.
 */
#define BIT_POS(_Instance, _BaseInstance, _InstanceSpacing)		\
	((reg_bit_pos_t) ((((uintptr_t) (_Instance)) - ((uintptr_t) (_BaseInstance))) / ((uintptr_t) (_InstanceSpacing))))

/** @} */ // 01_STM32F1xx_Utilities_01_Bit

/**
 * @defgroup 01_STM32F1xx_Utilities_02_RegOpsMacros Generic Register Operation Macros
 * @ingroup 01_STM32F1xx_Utilities
 * @brief Unchecked direct register-access macros
 * @details These macros access compatible 32-bit memory-mapped registers
 * without status handling.
 * @warning Register-pointer arguments must be valid and non-`NULL`.
 * @{
 */

/**
 * @brief Gets a peripheral register's writable `.REG` member address
 * @def REGOPS_REG
 * @param[in] _Peripheral Peripheral instance pointer
 * @param[in] _Register Register member token inside the peripheral register map
 * @returns Pointer to the selected `.REG` member
 * @note This macro exists because C cannot pass a struct member token to a
 * generic static inline function without first forming the member address.
 * @pre @p _Peripheral points to a register map where @p _Register exposes `.REG`.
 */
#define REGOPS_REG(_Peripheral, _Register)				(&((_Peripheral)->_Register.REG))

/**
 * @brief Reads a complete register image
 * @def REGOPS_READ
 * @param[in] _Register Pointer to a readable memory-mapped register
 * @param[out] _RegisterImage Destination register image
 * @returns Assigned register image
 * @note @p _RegisterImage is evaluated as an assignable expression.
 */
#define REGOPS_READ(_Register, _RegisterImage)						((_RegisterImage) = *(_Register))

/**
 * @brief Writes a complete register image
 * @def REGOPS_WRITE
 * @param[in,out] _Register Pointer to a writable memory-mapped register
 * @param[in] _RegisterImage Complete register image to write
 * @returns Assigned register image
 */
#define REGOPS_WRITE(_Register, _RegisterImage)					(*(_Register) = (_RegisterImage))

/**
 * @brief Sets selected register bits
 * @def REGOPS_SET
 * @param[in,out] _Register Pointer to a writable memory-mapped register
 * @param[in] _BitMask Register bit mask to set
 * @returns Updated register image
 * @warning This macro performs a non-atomic read-modify-write operation.
 */
#define REGOPS_SET(_Register, _BitMask)						(*(_Register) |= (_BitMask))

/**
 * @brief Clears selected register bits
 * @def REGOPS_CLEAR
 * @param[in,out] _Register Pointer to a writable memory-mapped register
 * @param[in] _BitMask Register bit mask to clear
 * @returns Updated register image
 * @warning This macro performs a non-atomic read-modify-write operation.
 */
#define REGOPS_CLEAR(_Register, _BitMask)					(*(_Register) &= ~(_BitMask))

/**
 * @brief Toggles selected register bits
 * @def REGOPS_TOGGLE
 * @param[in,out] _Register Pointer to a writable memory-mapped register
 * @param[in] _BitMask Register bit mask to toggle
 * @returns Updated register image
 * @warning This macro performs a non-atomic read-modify-write operation.
 */
#define REGOPS_TOGGLE(_Register, _BitMask)					(*(_Register) ^= (_BitMask))

/**
 * @brief Replaces selected bits in a register
 * @def REGOPS_MODIFY
 * @param[in,out] _Register Pointer to a writable memory-mapped register
 * @param[in] _PositionedFieldMask Positioned register field mask
 * @param[in] _PositionedFieldValue Positioned register field value
 * @returns Updated register image
 * @note Bits in @p _PositionedFieldValue outside @p _PositionedFieldMask are ignored.
 * @warning This macro performs a non-atomic read-modify-write operation.
 */
#define REGOPS_MODIFY(_Register, _PositionedFieldMask, _PositionedFieldValue)			\
	(*(_Register) = (*(_Register) & ~(_PositionedFieldMask)) |							\
		((_PositionedFieldValue) & (_PositionedFieldMask)))

/** @} */ // 01_STM32F1xx_Utilities_02_RegOpsMacros

// ==================================================================================================== //
//											Register Operations Utilities								//
// ==================================================================================================== //

/**
 * @defgroup 01_STM32F1xx_Utilities_04_RegisterOps Register Operations Utilities
 * @ingroup 01_STM32F1xx_Utilities
 * @brief Checked helpers for staging and accessing 32-bit registers
 * @{
 */

/**
 * @brief Replaces a positioned field in a register image
 * @param[in] registerImage Register image before field replacement
 * @param[in] positionedFieldMask Register-positioned field mask
 * @param[in] positionedFieldValue Register-positioned replacement value
 * @returns Updated register image as @ref reg
 * @note Bits in @p positionedFieldValue outside @p positionedFieldMask are ignored.
 */
__STATIC_FORCEINLINE reg RegOps_StageField
(
	const reg	registerImage,
	const reg	positionedFieldMask,
	const reg	positionedFieldValue
)
{
	// Local Variable
	reg updatedRegisterImage = registerImage;

	//! Clear the selected field while preserving every unrelated register bit.
	updatedRegisterImage &= ~positionedFieldMask;
	//! Apply only replacement bits covered by the positioned field mask.
	updatedRegisterImage |= (positionedFieldValue & positionedFieldMask);

	return updatedRegisterImage;
}

/**
 * @brief Extracts a right-aligned field from a register image
 * @param[in] registerImage Register image to inspect
 * @param[in] positionedFieldMask Register-positioned field mask
 * @param[in] fieldPosition Zero-based least-significant field bit position
 * @returns Right-aligned field value as @ref reg
 * @pre @p fieldPosition matches the least-significant set bit in
 * @p positionedFieldMask.
 */
__STATIC_FORCEINLINE reg RegOps_ExtractFieldValue
(
	const reg				registerImage,
	const reg				positionedFieldMask,
	const reg_bit_pos_t		fieldPosition
)
{
	//! Mask first, then shift down so callers receive a right-aligned raw field value.
	return ((registerImage & positionedFieldMask) >> fieldPosition);
}

/**
 * @brief Stages a right-aligned field value into a 32-bit register image
 * @see REG_FIELD_MASK
 * @see REG_FIELD_PACK
 * @see RegOps_StageField
 * @param[in] registerImage Register image before field replacement
 * @param[in] fieldPosition Zero-based least-significant field bit position
 * @param[in] fieldValue Right-aligned field value to stage
 * @param[in] fieldWidth Number of bits occupied by the field
 * @returns Updated register image as @ref reg
 * @note @p fieldWidth must be in the range `0U..31U`.
 * @note @p fieldPosition must be in the range `0U..31U`.
 * @warning Field bits positioned beyond bit `31U` are discarded.
 */
__STATIC_FORCEINLINE reg RegOps_StageFieldValue
(
	const reg				registerImage,
	const reg_bit_pos_t		fieldPosition,
	const reg				fieldValue,
	const reg_field_width_t	fieldWidth
)
{
	// Local Variables
	const reg positionedFieldMask = REG_FIELD_MASK(fieldPosition, fieldWidth);
	const reg positionedFieldValue = REG_FIELD_PACK(fieldPosition, fieldValue, fieldWidth);

	//! Reuse the mask-based staging primitive after positioning both field operands.
	return RegOps_StageField(registerImage, positionedFieldMask, positionedFieldValue);
}

/**
 * @brief Reads a complete 32-bit memory-mapped register image
 * @param[in] pRegister Pointer to a readable memory-mapped register
 * @param[out] pRegisterImage Destination for the captured register image
 * @returns Driver operation status
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Register image was captured.
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: An input pointer is `NULL`.
 * @note The caller owns address-range and peripheral-instance validation.
 */
__STATIC_FORCEINLINE driver_status_t RegOps_Read(const _IO* const pRegister, reg* const pRegisterImage)
{
	//! Validate both addresses before touching memory-mapped I/O or caller storage.
	if ((pRegister == NULL) || (pRegisterImage == NULL))
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Capture the volatile register once so the caller receives one coherent image.
	*pRegisterImage = *pRegister;

	return DRIVER_STATUS_SUCCESS;
}

/**
 * @brief Writes a complete 32-bit memory-mapped register image
 * @param[in,out] pRegister Pointer to a writable memory-mapped register
 * @param[in] registerImage Complete register image to write
 * @returns Driver operation status
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Register image was written.
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRegister is `NULL`.
 * @note The caller owns address-range, peripheral-instance, and write-policy validation.
 */
__STATIC_FORCEINLINE driver_status_t RegOps_Write(_IO* const pRegister, const reg registerImage)
{
	//! Reject an invalid destination before performing the volatile register write.
	if (pRegister == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Replace the complete register image exactly as supplied by the caller.
	*pRegister = registerImage;

	return DRIVER_STATUS_SUCCESS;
}

/**
 * @brief Writes a register only when its staged image differs from hardware
 * @param[in,out] pRegister Pointer to the writable memory-mapped register
 * @param[in] currentRegisterImage Previously captured image of @p pRegister
 * @param[in] stagedRegisterImage Complete register image requested by the caller
 * @returns Driver operation status
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Write was skipped or completed.
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRegister is `NULL`.
 * @pre @p currentRegisterImage represents the current image of @p pRegister.
 * @note Supplying the current image lets callers reuse an existing snapshot
 * without forcing another volatile register read.
 */
__STATIC_FORCEINLINE driver_status_t RegOps_WriteIfChanged
(
	_IO* const	pRegister,
	const reg	currentRegisterImage,
	const reg	stagedRegisterImage
)
{
	//! Validate the register even when equal images would otherwise skip the write.
	if (pRegister == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Avoid register side effects and bus traffic when staging produced no change.
	if (currentRegisterImage != stagedRegisterImage)
	{
		return RegOps_Write(pRegister, stagedRegisterImage);
	}

	return DRIVER_STATUS_SUCCESS;
}

/**
 * @brief Sets selected bits in a memory-mapped register
 * @param[in,out] pRegister Pointer to a writable memory-mapped register
 * @param[in] bitMask Register bit mask to set
 * @returns Driver operation status
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Selected bits were set.
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRegister is `NULL`.
 * @warning This helper performs a non-atomic read-modify-write operation.
 */
__STATIC_FORCEINLINE driver_status_t RegOps_Set(_IO* const pRegister, const reg bitMask)
{
	//! Reject an invalid destination before starting the read-modify-write sequence.
	if (pRegister == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Preserve unrelated register bits while setting the requested mask.
	*pRegister |= bitMask;

	return DRIVER_STATUS_SUCCESS;
}

/**
 * @brief Clears selected bits in a memory-mapped register
 * @param[in,out] pRegister Pointer to a writable memory-mapped register
 * @param[in] bitMask Register bit mask to clear
 * @returns Driver operation status
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Selected bits were cleared.
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRegister is `NULL`.
 * @warning This helper performs a non-atomic read-modify-write operation.
 */
__STATIC_FORCEINLINE driver_status_t RegOps_Clear(_IO* const pRegister, const reg bitMask)
{
	//! Reject an invalid destination before starting the read-modify-write sequence.
	if (pRegister == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Preserve unrelated register bits while clearing the requested mask.
	*pRegister &= ~bitMask;

	return DRIVER_STATUS_SUCCESS;
}

/**
 * @brief Toggles selected bits in a memory-mapped register
 * @param[in,out] pRegister Pointer to a writable memory-mapped register
 * @param[in] bitMask Register bit mask to toggle
 * @returns Driver operation status
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Selected bits were toggled.
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRegister is `NULL`.
 * @warning This helper performs a non-atomic read-modify-write operation.
 */
__STATIC_FORCEINLINE driver_status_t RegOps_Toggle(_IO* const pRegister, const reg bitMask)
{
	//! Reject an invalid destination before starting the read-modify-write sequence.
	if (pRegister == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Preserve unrelated register bits while toggling the requested mask.
	*pRegister ^= bitMask;

	return DRIVER_STATUS_SUCCESS;
}

/**
 * @brief Replaces selected bits in a memory-mapped register
 * @param[in,out] pRegister Pointer to a writable memory-mapped register
 * @param[in] positionedFieldMask Register-positioned field mask
 * @param[in] positionedFieldValue Register-positioned replacement value
 * @returns Driver operation status
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Selected register bits were replaced.
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p pRegister is `NULL`.
 * @note Bits in @p positionedFieldValue outside @p positionedFieldMask are ignored.
 * @warning This helper performs a non-atomic read-modify-write operation.
 */
__STATIC_FORCEINLINE driver_status_t RegOps_WriteMasked
(
	_IO* const	pRegister,
	const reg	positionedFieldMask,
	const reg	positionedFieldValue
)
{
	// Local Variable
	reg updatedRegisterImage;

	//! Reject an invalid destination before starting the read-modify-write sequence.
	if (pRegister == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Stage the field from one hardware snapshot so unrelated bits remain unchanged.
	updatedRegisterImage = RegOps_StageField(*pRegister, positionedFieldMask, positionedFieldValue);
	*pRegister = updatedRegisterImage;

	return DRIVER_STATUS_SUCCESS;
}

/** @} */ // 01_STM32F1xx_Utilities_04_RegisterOps

// --- C++ Safeguards ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32F1XX_UTILS_H_ */
