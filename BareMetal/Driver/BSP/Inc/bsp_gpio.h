/**
 * @file	bsp_gpio.h
 * @author	Shrey Shah
 * @brief	Declares the Blue Pill board GPIO capability
 * @version	v1.0
 * @date	26-08-2026
 *
 * @details
 * This self-contained capability header binds the STM32F103C8T6 Blue Pill
 * active-low on-board LED to PC13. It includes only GPIO because its public
 * mappings and operations use GPIO symbols; RCC remains private to the
 * complete BSP_InitOBLED() initialization transaction.
 */

// Header Guard
#ifndef BSP_GPIO_H_
#define BSP_GPIO_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "gpio.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Board support package APIs and board-level constants
 * @defgroup BSP_01_BoardSupport BSP Board Support
 * @details
 * These APIs bind reusable peripheral Drivers to resources fixed by the
 * STM32F103C8T6 Blue Pill board.
 * @{
 */

#ifdef STM32F103C8T6__

// ==================================================================================================== //
// On-Board LED Mapping
// ==================================================================================================== //

/**
 * @brief GPIO peripheral instance connected to the Blue Pill on-board LED
 * @def BSP_GPIO_OB_LED_PORT
 * @details
 * The active-low on-board LED is fixed to GPIOC. This public board mapping
 * does not enable the port clock or configure the pin by itself.
 * Accepted values:
 * - @ref `GPIOC`: Port containing the Blue Pill on-board LED
 */
#define BSP_GPIO_OB_LED_PORT							GPIOC

/**
 * @brief GPIO pin connected to the Blue Pill on-board LED
 * @def BSP_GPIO_OB_LED_PIN
 * @details
 * PC13 is active-low: resetting its latch turns the LED on and setting its
 * latch turns the LED off. This mapping does not configure the pin by itself.
 * Accepted values:
 * - @ref `GPIO_PIN_13`: Blue Pill on-board LED pin
 */
#define BSP_GPIO_OB_LED_PIN								GPIO_PIN_13

/** @brief I2C1 default SCL GPIO port @def BSP_GPIO_I2C1_DEFAULT_SCL_PORT */
#define BSP_GPIO_I2C1_DEFAULT_SCL_PORT					GPIOB
/** @brief I2C1 default SCL GPIO pin @def BSP_GPIO_I2C1_DEFAULT_SCL_PIN */
#define BSP_GPIO_I2C1_DEFAULT_SCL_PIN					GPIO_PIN_6

/** @brief I2C1 default SDA GPIO port @def BSP_GPIO_I2C1_DEFAULT_SDA_PORT */
#define BSP_GPIO_I2C1_DEFAULT_SDA_PORT					GPIOB
/** @brief I2C1 default SDA GPIO pin @def BSP_GPIO_I2C1_DEFAULT_SDA_PIN */
#define BSP_GPIO_I2C1_DEFAULT_SDA_PIN					GPIO_PIN_7

// ==================================================================================================== //
// On-Board LED APIs
// ==================================================================================================== //

/**
 * @brief Initializes the complete Blue Pill on-board LED capability
 * @details
 * Enables the GPIOC peripheral clock, configures PC13 as a 2 MHz push-pull
 * output, and sets its latch so the active-low LED starts deterministically
 * off. BSP owns this complete RCC/GPIO initialization transaction.
 * @returns @ref driver_status_t "On-board LED initialization status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: GPIOC and PC13 were initialized and the LED was forced off
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: The fixed board GPIO mapping was rejected
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: GPIO clock validation or GPIO staging failed
 */
driver_status_t BSP_InitOBLED(void);

/**
 * @brief Sets the active-low board on-board LED
 * @details
 * Resets PC13's output latch through GPIO_PinReset(), turning the LED on.
 * @pre BSP_InitOBLED() completed successfully
 */
void BSP_OB_LED_Set(void);

/**
 * @brief Resets the active-low board on-board LED
 * @details
 * Sets PC13's output latch through GPIO_PinSet(), turning the LED off.
 * @pre BSP_InitOBLED() completed successfully
 */
void BSP_OB_LED_Reset(void);

/**
 * @brief Toggles the active-low board on-board LED
 * @details
 * Delegates to GPIO_PinToggle() for PC13; active-low wiring converts the latch
 * transition into the opposite visible LED state.
 * @pre BSP_InitOBLED() completed successfully
 */
void BSP_OB_LED_Toggle(void);

#endif /* STM32F103C8T6__ */

/** @} */ // BSP_01_BoardSupport

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_GPIO_H_ */
