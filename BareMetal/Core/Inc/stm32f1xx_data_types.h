/**
 * @file	stm32f1xx_data_types.h
 * @author	Shrey Shah
 * @brief	Defines shared Core scalar, register, and Driver-status types
 * @version	v1.2
 * @date	23-08-2026
 *
 * @details
 * @section STM32F1XX_DATA_TYPES_H_HIERARCHY Hierarchy
 * - Position: Foundational Core type layer
 * - Included by: Core register models, peripheral Drivers, and applications
 * - Uses: Standard C type and library declarations only
 *
 * @section STM32F1XX_DATA_TYPES_H_RESPONSIBILITY Responsibility
 * This header is the repository gateway for fixed-width integer types,
 * memory-mapped register qualifiers, register-image vocabulary, shared
 * physical quantities, and Driver statuses.
 *
 * @section STM32F1XX_DATA_TYPES_H_DEPENDENCIES Standard C Dependencies
 * `<stdint.h>` supplies fixed-width integers, `<stdbool.h>` supplies the C
 * Boolean vocabulary, `<stdlib.h>` supplies common library declarations, and
 * `<string.h>` supplies byte/string operation declarations used by existing
 * repository modules through this shared Core gateway.
 *
 * @section STM32F1XX_DATA_TYPES_H_BOUNDARY Dependency Boundary
 * This header includes no project, peripheral, register-map, Driver, Codec, or
 * LL header. It defines vocabulary only and performs no hardware access,
 * validation, allocation, string operation, or peripheral policy.
 */

// Header Guard
#ifndef STM32F1XX_DATA_TYPES_H_
#define STM32F1XX_DATA_TYPES_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// C++ Compatibility
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Shared STM32F1 Core type vocabulary
 * @defgroup STM32F1xx_DataTypes STM32F1 Shared Data Types
 * @ingroup STM32F1xx
 * @{
 */

// ==================================================================================================== //
// Register Access and Image Types
// ==================================================================================================== //

/**
 * @brief Declares a volatile read/write 32-bit hardware register word
 * @typedef _IO
 * @details
 * The `volatile` qualifier forces every C read and write expression to perform
 * an observable access to the represented memory-mapped register.
 * @warning `volatile` does not provide atomicity, ordering between agents, or
 * register-specific read/modify/write safety
 */
typedef volatile uint32_t						_IO;

/**
 * @brief Declares a volatile read-only 32-bit hardware register word
 * @typedef _I
 * @details
 * The `const volatile` qualifiers require a fresh hardware read while
 * preventing writes through the qualified C lvalue.
 * @note Hardware may update the represented register asynchronously
 */
typedef volatile const uint32_t					_I;

/**
 * @brief Declares a volatile write-only 32-bit hardware register word
 * @typedef _O
 * @details
 * The C type remains readable because C has no write-only qualifier; the
 * register-map contract prohibits reads through this semantic alias.
 * @warning Reading a hardware register declared with this alias may return an
 * undefined value or trigger a device-specific side effect
 */
typedef volatile uint32_t						_O;

/**
 * @brief Defines a non-volatile 32-bit register image
 * @typedef reg
 * @details
 * Use this type for local register snapshots, staged register images, masks,
 * and complete register values. Use `_IO`, `_I`, or `_O` only in hardware
 * register-map structures.
 */
typedef uint32_t								reg;

/**
 * @brief Defines a right-aligned raw register-field value
 * @typedef reg_field_t
 */
typedef uint8_t									reg_field_t;

/**
 * @brief Defines a zero-based register bit position
 * @typedef reg_bit_pos_t
 * @note Valid positions for a 32-bit register are `0U..31U`
 */
typedef uint8_t									reg_bit_pos_t;

/**
 * @brief Defines the width of a register field in bits
 * @typedef reg_field_width_t
 * @note Individual register helpers define whether width `0U` is admitted
 */
typedef uint8_t									reg_field_width_t;

// ==================================================================================================== //
// Shared Physical Quantity Types
// ==================================================================================================== //

/**
 * @brief Defines a frequency value in hertz shared by every peripheral stack
 * @typedef frequency_t
 */
typedef uint32_t								frequency_t;

/** @brief Zero-valued frequency used for inactive or unavailable clocks @def FREQ_ZERO */
#define FREQ_ZERO								((frequency_t) 0UL)
/** @brief 1kHz frequency scaling constant @def FREQ_1kHz */
#define FREQ_1kHz								((frequency_t) 1000UL)
/** @brief 10kHz frequency scaling constant @def FREQ_10kHz */
#define FREQ_10kHz								((frequency_t) 10000UL)
/** @brief 100kHz frequency scaling constant @def FREQ_100kHz */
#define FREQ_100kHz								((frequency_t) 100000UL)
/** @brief One-megahertz frequency scaling constant @def FREQ_1MHz */
#define FREQ_1MHz								((frequency_t) 1000000UL)

// ==================================================================================================== //
// Driver Operation Status Type
// ==================================================================================================== //

/**
 * @brief Driver operation status vocabulary and propagation utilities
 * @defgroup 01_STM32F1xx_Utilities_03_DriverStatus Driver Status Definitions
 * @ingroup 01_STM32F1xx_Utilities
 * @{
 */

/**
 * @brief Defines common Driver operation results and observable states
 * @typedef driver_status_t
 * @details
 * Numeric ranges preserve three result classes:
 * - Values through @ref `DRIVER_STATUS_ERROR` indicate errors
 * - @ref `DRIVER_STATUS_SUCCESS` indicates successful completion
 * - Values from @ref `DRIVER_STATUS_OFF` indicate observable non-error states
 */
typedef enum _driver_status_t
{
	/** @brief Transport fault on one line, bit, or flag condition. @enum DRIVER_STATUS_ERROR_LINE_FAULT */
	DRIVER_STATUS_ERROR_LINE_FAULT			=	-10,
	/** @brief Data arrived faster than it was consumed and was discarded. @enum DRIVER_STATUS_ERROR_OVERRUN */
	DRIVER_STATUS_ERROR_OVERRUN				=	-9,
	/** @brief Bus contention resolved in favor of another master. @enum DRIVER_STATUS_ERROR_ARBITRATION_LOST */
	DRIVER_STATUS_ERROR_ARBITRATION_LOST	=	-8,
	/** @brief A remote device failed to acknowledge the addressed request. @enum DRIVER_STATUS_ERROR_NO_ACK */
	DRIVER_STATUS_ERROR_NO_ACK				=	-7,
	/** @brief Peripheral or driver is currently executing an operation. @enum DRIVER_STATUS_ERROR_BUSY */
	DRIVER_STATUS_ERROR_BUSY				=	-6,
	/** @brief Operation requested in an invalid driver or peripheral state. @enum DRIVER_STATUS_ERROR_STATE */
	DRIVER_STATUS_ERROR_STATE				=	-5,
	/** @brief Operation timed out waiting for hardware or condition. @enum DRIVER_STATUS_ERROR_TIMEOUT */
	DRIVER_STATUS_ERROR_TIMEOUT				=	-4,
	/** @brief Operation failed due to a non-specific error. @enum  */
	DRIVER_STATUS_ERROR_FAIL				=	-3,
	/** @brief An input parameter was invalid or out of range. @enum DRIVER_STATUS_ERROR_INVALID_ARG */
	DRIVER_STATUS_ERROR_INVALID_ARG			=	-2,
	/** @brief Operation failed due to a null pointer. @enum DRIVER_STATUS_ERROR_NULL_PTR */
	DRIVER_STATUS_ERROR_NULL_PTR			=	-1,
	/** @brief Generic error (baseline error code). @enum DRIVER_STATUS_ERROR */
	DRIVER_STATUS_ERROR						=	0,
	/** @brief Operation completed successfully. @enum DRIVER_STATUS_SUCCESS */
	DRIVER_STATUS_SUCCESS					=	1,
	/** @brief Target state: Disabled or turned OFF. @enum DRIVER_STATUS_OFF */
	DRIVER_STATUS_OFF						=	2,
	/** @brief Target state: Enabled or turned ON. @enum DRIVER_STATUS_ON */
	DRIVER_STATUS_ON						=	3,
	/** @brief Peripheral or driver is configured and ready for an operation. @enum DRIVER_STATUS_READY */
	DRIVER_STATUS_READY						=	4,
	/** @brief The peripheral clock gate required by the operation is disabled. @enum DRIVER_STATUS_ERROR_CLOCK_NOT_ENABLED */
	DRIVER_STATUS_ERROR_CLOCK_NOT_ENABLED	=	5,
	/** @brief A requested action is accepted but not yet complete. @enum DRIVER_STATUS_IN_PROGRESS */
	DRIVER_STATUS_IN_PROGRESS				=	6
} driver_status_t;

/**
 * @brief Propagates an unsuccessful Driver status from the enclosing function
 * @def ASSERT_DRIVER_STATUS
 * @details
 * Evaluates @p expr exactly once. Execution continues when it produces
 * @ref `DRIVER_STATUS_SUCCESS`; every other status is returned unchanged from
 * the enclosing function.
 * @param[in] expr Expression that produces @ref `driver_status_t`
 * Expected values:
 * - Any value representable by @ref `driver_status_t`
 * @warning Use this macro only inside a function whose return type admits every
 * value in @ref `driver_status_t`
 */
#define ASSERT_DRIVER_STATUS(expr)				\
do												\
{												\
	driver_status_t _st = (expr);				\
	if (_st != DRIVER_STATUS_SUCCESS)			\
	{											\
		return _st;								\
	}											\
} while (0)

/**
 * @brief Calculates the compile-time element count of an array object
 * @def ARRAY_SIZE
 * @param[in] arr Complete array object
 * Accepted values:
 * - Any complete array object visible at the expansion site
 * @returns Number of elements in @p arr
 * @warning A pointer argument produces the pointer-size ratio, not the number
 * of elements in the pointed-to storage
 */
#define ARRAY_SIZE(arr)				(sizeof(arr) / sizeof((arr)[0]))

/** @} */ // 01_STM32F1xx_Utilities_03_DriverStatus

/** @} */ // STM32F1xx_DataTypes

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32F1XX_DATA_TYPES_H_ */
