/**
 * @file	gpio.h
 * @author	Shrey Shah
 * @brief	GPIO Driver Public Interface
 * @version	v1.4
 * @date	30-05-2026
 *
 * @details
 * This header exposes the public GPIO driver API. Public code sees GPIO
 * selectors, validation macros, and driver entry points. Low-level register
 * access and codec image staging remain private to `gpio.c`.
 */

#ifndef GPIO_H_
#define GPIO_H_

// ==================================================================================================== //
//												Includes												//
// ==================================================================================================== //
#include "gpio_defines.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup GPIO_03_Driver
 * @{
 */

// ==================================================================================================== //
//										GPIO Driver Configuration APIs									//
// ==================================================================================================== //

/**
 * @brief Extracts the mode and electrical behavior of one GPIO pin
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pin GPIO single-pin mask
 * Accepted values:
 * - One value from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @param[out] pMode Destination for the current driver GPIO mode selector
 * Expected values:
 * - Non-`NULL`: Current mode is written to @p `pMode`
 * @param[out] pConfig Destination for the current driver GPIO configuration selector
 * Expected values:
 * - Non-`NULL`: Current configuration is written to @p `pConfig`
 * @returns @ref driver_status_t "Mode/config extraction status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The current mode/config pair was extracted
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pMode` or @p `pConfig` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx` or @p `pin` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Internal decode failed unexpectedly
 * @note @p `pin` must contain exactly one valid GPIO pin bit.
 */
driver_status_t GPIO_GetPinModeConfig
(
	GPIO_TypeDef* const			GPIOx,
	const gpio_pin_t			pin,
	gpio_pin_mode_t* const		pMode,
	gpio_pin_config_t* const	pConfig
);

/**
 * @brief Configures the mode and electrical behavior of one or more GPIO pins
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @param[in] mode Driver GPIO mode selector
 * Accepted values:
 * - @ref `GPIO_PIN_MODE_INPUT`
 * - @ref `GPIO_PIN_MODE_OUTPUT_10MHZ`
 * - @ref `GPIO_PIN_MODE_OUTPUT_2MHZ`
 * - @ref `GPIO_PIN_MODE_OUTPUT_50MHZ`
 * @param[in] config Driver GPIO configuration selector
 * Accepted values:
 * - @ref `GPIO_PIN_CONFIG_INPUT_ANALOG`
 * - @ref `GPIO_PIN_CONFIG_INPUT_FLOATING`
 * - @ref `GPIO_PIN_CONFIG_INPUT_PULL_DOWN`
 * - @ref `GPIO_PIN_CONFIG_INPUT_PULL_UP`
 * - @ref `GPIO_PIN_CONFIG_OUTPUT_PUSH_PULL`
 * - @ref `GPIO_PIN_CONFIG_OUTPUT_OPEN_DRAIN`
 * - @ref `GPIO_PIN_CONFIG_ALTERNATE_PUSH_PULL`
 * - @ref `GPIO_PIN_CONFIG_ALTERNATE_OPEN_DRAIN`
 * @returns @ref driver_status_t "Mode/config operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The requested mode/config pair was applied
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx`, @p `pinMask`, @p `mode`, or @p `config` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Internal staged-image update failed unexpectedly
 * @note @p `mode` and @p `config` must form a valid STM32F1 GPIO mode/config pair.
 */
driver_status_t GPIO_SetPinModeConfig
(
	GPIO_TypeDef* const			GPIOx,
	const gpio_pin_t			pinMask,
	const gpio_pin_mode_t		mode,
	const gpio_pin_config_t		config
);

/**
 * @brief Returns the current driver-facing mode selector for one GPIO pin
 * @details This is a convenience wrapper around @ref `GPIO_GetPinModeConfig`.
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pin GPIO single-pin mask
 * Accepted values:
 * - One value from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns Current mode as @ref `gpio_pin_mode_t`
 * @retval - @ref `GPIO_PIN_MODE_INPUT`: Pin is in input mode, or validation/decode failed
 * @retval - @ref `GPIO_PIN_MODE_OUTPUT_10MHZ`: Pin is in output mode with max speed 10 MHz
 * @retval - @ref `GPIO_PIN_MODE_OUTPUT_2MHZ`: Pin is in output mode with max speed 2 MHz
 * @retval - @ref `GPIO_PIN_MODE_OUTPUT_50MHZ`: Pin is in output mode with max speed 50 MHz
 * @note Returns @ref `GPIO_PIN_MODE_INPUT` if validation or decode fails.
 */
gpio_pin_mode_t GPIO_GetPinMode(GPIO_TypeDef* const GPIOx, const gpio_pin_t pin);

/**
 * @brief Configures the mode field of one or more GPIO pins
 * @details This is a convenience wrapper around @ref `GPIO_GetPinModeConfig`
 * and @ref `GPIO_SetPinModeConfig`. Existing configuration for every selected
 * pin must remain compatible with @p `mode`.
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @param[in] mode Driver GPIO mode selector
 * Accepted values:
 * - @ref `GPIO_PIN_MODE_INPUT`
 * - @ref `GPIO_PIN_MODE_OUTPUT_10MHZ`
 * - @ref `GPIO_PIN_MODE_OUTPUT_2MHZ`
 * - @ref `GPIO_PIN_MODE_OUTPUT_50MHZ`
 * @returns @ref driver_status_t "Mode field operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The requested mode field was applied
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx`, @p `pinMask`, or @p `mode` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Existing pin state could not be decoded or staged
 * @note Use @ref `GPIO_SetPinModeConfig` when changing mode and config together.
 */
driver_status_t GPIO_SetPinMode
(
	GPIO_TypeDef* const			GPIOx,
	const gpio_pin_t			pinMask,
	const gpio_pin_mode_t		mode
);

/**
 * @brief Returns the current driver-facing configuration selector for one GPIO pin
 * @details This is a convenience wrapper around @ref `GPIO_GetPinModeConfig`.
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pin GPIO single-pin mask
 * Accepted values:
 * - One value from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns Current configuration as @ref `gpio_pin_config_t`
 * @retval - @ref `GPIO_PIN_CONFIG_INPUT_ANALOG`: Pin is analog input, or validation/decode failed
 * @retval - @ref `GPIO_PIN_CONFIG_INPUT_FLOATING`: Pin is floating input
 * @retval - @ref `GPIO_PIN_CONFIG_INPUT_PULL_DOWN`: Pin is input with pull-down
 * @retval - @ref `GPIO_PIN_CONFIG_INPUT_PULL_UP`: Pin is input with pull-up
 * @retval - @ref `GPIO_PIN_CONFIG_OUTPUT_PUSH_PULL`: Pin is general-purpose output push-pull
 * @retval - @ref `GPIO_PIN_CONFIG_OUTPUT_OPEN_DRAIN`: Pin is general-purpose output open-drain
 * @retval - @ref `GPIO_PIN_CONFIG_ALTERNATE_PUSH_PULL`: Pin is alternate-function output push-pull
 * @retval - @ref `GPIO_PIN_CONFIG_ALTERNATE_OPEN_DRAIN`: Pin is alternate-function output open-drain
 * @note Returns @ref `GPIO_PIN_CONFIG_INPUT_ANALOG` if validation or decode fails.
 */
gpio_pin_config_t GPIO_GetPinConfig(GPIO_TypeDef* const GPIOx, const gpio_pin_t pin);

/**
 * @brief Configures the electrical configuration field of one or more GPIO pins
 * @details This is a convenience wrapper around @ref `GPIO_GetPinModeConfig`
 * and @ref `GPIO_SetPinModeConfig`. Existing mode for every selected pin must
 * remain compatible with @p `config`.
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @param[in] config Driver GPIO configuration selector
 * Accepted values:
 * - @ref `GPIO_PIN_CONFIG_INPUT_ANALOG`
 * - @ref `GPIO_PIN_CONFIG_INPUT_FLOATING`
 * - @ref `GPIO_PIN_CONFIG_INPUT_PULL_DOWN`
 * - @ref `GPIO_PIN_CONFIG_INPUT_PULL_UP`
 * - @ref `GPIO_PIN_CONFIG_OUTPUT_PUSH_PULL`
 * - @ref `GPIO_PIN_CONFIG_OUTPUT_OPEN_DRAIN`
 * - @ref `GPIO_PIN_CONFIG_ALTERNATE_PUSH_PULL`
 * - @ref `GPIO_PIN_CONFIG_ALTERNATE_OPEN_DRAIN`
 * @returns @ref driver_status_t "Configuration field operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The requested configuration field was applied
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx`, @p `pinMask`, or @p `config` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Existing pin state could not be decoded or staged
 * @note Use @ref `GPIO_SetPinModeConfig` when changing mode and config together.
 */
driver_status_t GPIO_SetPinConfig
(
	GPIO_TypeDef* const			GPIOx,
	const gpio_pin_t			pinMask,
	const gpio_pin_config_t		config
);

/**
 * @brief Initializes one or more GPIO pins from explicit mode/config selectors
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @param[in] mode Driver GPIO mode selector
 * Accepted values:
 * - @ref `GPIO_PIN_MODE_INPUT`
 * - @ref `GPIO_PIN_MODE_OUTPUT_10MHZ`
 * - @ref `GPIO_PIN_MODE_OUTPUT_2MHZ`
 * - @ref `GPIO_PIN_MODE_OUTPUT_50MHZ`
 * @param[in] config Driver GPIO configuration selector
 * Accepted values:
 * - @ref `GPIO_PIN_CONFIG_INPUT_ANALOG`
 * - @ref `GPIO_PIN_CONFIG_INPUT_FLOATING`
 * - @ref `GPIO_PIN_CONFIG_INPUT_PULL_DOWN`
 * - @ref `GPIO_PIN_CONFIG_INPUT_PULL_UP`
 * - @ref `GPIO_PIN_CONFIG_OUTPUT_PUSH_PULL`
 * - @ref `GPIO_PIN_CONFIG_OUTPUT_OPEN_DRAIN`
 * - @ref `GPIO_PIN_CONFIG_ALTERNATE_PUSH_PULL`
 * - @ref `GPIO_PIN_CONFIG_ALTERNATE_OPEN_DRAIN`
 * @returns @ref driver_status_t "Pin initialization status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Pin configuration completed successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx`, @p `pinMask`, @p `mode`, or @p `config` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The GPIO port clock gate (or, for alternate-function
 * @p `config`, the AFIO clock gate) is disabled, or the internal staged-image update failed unexpectedly
 * @pre The application enabled the matching GPIO port clock gate, and the AFIO clock gate when
 * @p `config` is alternate-function, through the RCC driver before calling this API.
 * @note @p `mode` and @p `config` must form a valid STM32F1 GPIO mode/config pair.
 * @note This API only verifies the required clock gate(s); it never enables them.
 */
driver_status_t GPIO_Init
(
	GPIO_TypeDef* const			GPIOx,
	const gpio_pin_t			pinMask,
	const gpio_pin_mode_t		mode,
	const gpio_pin_config_t		config
);

/**
 * @brief Restores one or more GPIO pins to floating-input configuration
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns @ref driver_status_t "Pin restoration status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The selected pins were restored
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx` or @p `pinMask` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Internal staged-image update failed unexpectedly
 * @note STM32F1 GPIO reset configuration is input floating:
 * `MODE[1:0] = 0b00`, `CNF[1:0] = 0b01`, so each CRL/CRH pin field is `0x04U`
 * and a full CRL/CRH reset image is `0x44444444UL`.
 * @note This API restores selected CRL/CRH fields and resets the selected
 * output latch bits through BRR. AFIO clock is not disabled here; disable it
 * explicitly when no remaining peripheral path requires AFIO.
 */
driver_status_t GPIO_Deinit(GPIO_TypeDef* const GPIOx, const gpio_pin_t pinMask);

// ==================================================================================================== //
//										GPIO Driver Pin State APIs										//
// ==================================================================================================== //

/**
 * @brief Reads the sampled logic level of one GPIO input pin
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pin GPIO single-pin mask
 * Accepted values:
 * - One value from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns @ref driver_status_t "Logic-level read status"
 * @retval - @ref `DRIVER_STATUS_OFF`: The sampled input bit was low
 * @retval - @ref `DRIVER_STATUS_ON`: The sampled input bit was high
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx` or @p `pin` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Internal input image extraction failed
 * @note @p `pin` must contain exactly one valid GPIO pin bit.
 */
driver_status_t GPIO_PinGet(GPIO_TypeDef* const GPIOx, const gpio_pin_t pin);

/**
 * @brief Sets one or more GPIO output latch bits
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns @ref driver_status_t "Output-latch set operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The selected output latch bits were set
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx` or @p `pinMask` was invalid
 * @note This API writes through `GPIOx_BSRR`; it does not read or write `GPIOx_ODR`.
 */
driver_status_t GPIO_PinSet(GPIO_TypeDef* const GPIOx, const gpio_pin_t pinMask);

/**
 * @brief Resets one or more GPIO output latch bits
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns @ref driver_status_t "Output-latch reset operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The selected output latch bits were reset
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx` or @p `pinMask` was invalid
 * @note This API writes through `GPIOx_BRR`; it does not read or write `GPIOx_ODR`.
 */
driver_status_t GPIO_PinReset(GPIO_TypeDef* const GPIOx, const gpio_pin_t pinMask);

/**
 * @brief Toggles one or more GPIO output latch bits
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns @ref driver_status_t "Output-latch toggle operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The selected output latch bits were toggled
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx` or @p `pinMask` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: Internal output image extraction or staging failed
 * @note Toggle uses `GPIOx_ODR` read-modify-write because it must observe the
 * existing latch state before applying the inverted state.
 */
driver_status_t GPIO_PinToggle(GPIO_TypeDef* const GPIOx, const gpio_pin_t pinMask);

/**
 * @brief Initializes one or more GPIO pins as LED-safe outputs
 * @param[in] GPIOx GPIO peripheral instance
 * Accepted values:
 * - @ref `GPIOA`
 * - @ref `GPIOB`
 * - @ref `GPIOC`
 * - @ref `GPIOD`
 * - @ref `GPIOE`
 * - @ref `GPIOF`
 * - @ref `GPIOG`
 * @param[in] pinMask GPIO pin mask
 * Accepted values:
 * - One or more OR-combined values from @ref `GPIO_PIN_0` through @ref `GPIO_PIN_15`
 * @returns @ref driver_status_t "LED pin initialization status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: LED pin configuration completed successfully
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `GPIOx` or @p `pinMask` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The GPIO port clock gate is disabled, or the internal
 * staged-image update failed unexpectedly
 * @pre The application enabled the matching GPIO port clock gate through the RCC driver before
 * calling this API.
 * @note This API always configures selected pins as @ref `GPIO_PIN_MODE_OUTPUT_2MHZ`
 * with @ref `GPIO_PIN_CONFIG_OUTPUT_PUSH_PULL`.
 * @note This API only verifies the required clock gate; it never enables it.
 */
driver_status_t GPIO_LED_Init(GPIO_TypeDef* const GPIOx, const gpio_pin_t pinMask);

/** @} */ // GPIO_03_Driver

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GPIO_H_ */
