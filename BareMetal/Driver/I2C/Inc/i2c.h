/**
 * @file i2c.h
 * @author Shrey Shah
 * @brief I2C Driver Header File
 * @version v1.0
 * @date 16-11-2025
 * @details
 * This header exposes the contract-frozen public I2C Driver surface defined
 * in I2C_ARCHITECTURE.md. It is limited to controller-only configuration,
 * operation control, and bounded synchronous whole-message transactions.
 * Register-image staging, raw START/STOP control, device-register protocols,
 * IRQ, DMA, and GPIO/AFIO ownership remain outside this public interface.
 */

// Header Guards
#ifndef I2C_H_
#define I2C_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx.h"
#include "i2c_config.h"

// C++ Compatibility
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup I2C_03_Driver I2C Driver APIs
 * @ingroup I2C
 * @brief I2C Driver APIs
 * @details
 * Public Driver contracts follow the Driver -> Codec -> LL ownership model
 * in I2C_ARCHITECTURE.md. The initial scope is controller-only with bounded
 * synchronous transactions; application-owned RCC gating and GPIO/AFIO
 * routing remain explicit prerequisites.
 * @{
 */

// ==================================================================================================== //
// I2C Operation State APIs
// ==================================================================================================== //

/**
 * @brief Gets the I2C peripheral operation state
 * @details
 * Reads the current `I2Cx_CR1.PE` state for one supported I2C instance. This
 * API does not change the application-owned APB1 clock gate or any peripheral
 * configuration register.
 *
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @returns @ref driver_status_t "I2C operation-state query status"
 * @retval - @ref `DRIVER_STATUS_OFF`: I2C peripheral operation is disabled
 * @retval - @ref `DRIVER_STATUS_ON`: I2C peripheral operation is enabled
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx` was unsupported
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The matching APB1 clock gate is disabled
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 */
driver_status_t I2C_GetOperationState(I2C_TypeDef* const I2Cx);

/**
 * @brief Sets the I2C peripheral operation state
 * @details
 * Sets or clears only `I2Cx_CR1.PE` through the Codec and LL layers while
 * preserving supported persistent CR1 configuration. The driver does not
 * enable or disable the application-owned APB1 clock gate.
 *
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @param[in] operationState Requested I2C peripheral operation state
 * Accepted values:
 * - @ref `DRIVER_STATUS_OFF`: Disable I2C peripheral operation
 * - @ref `DRIVER_STATUS_ON`: Enable I2C peripheral operation
 * @returns @ref driver_status_t "I2C operation-state configuration status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: I2C peripheral operation state was updated
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx` was unsupported or @p `operationState` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The matching APB1 clock gate is disabled or the I2C timing image is not configured
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The bus is busy or a START/STOP action is pending
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre The application owns the complete I2C transaction
 */
driver_status_t I2C_SetOperationState(I2C_TypeDef* const I2Cx, const driver_status_t operationState);

// ==================================================================================================== //
// I2C Root Configuration APIs
// ==================================================================================================== //

/**
 * @brief Restores one supported I2C register bank to its hardware reset state
 * @details
 * Requires the matching APB1 clock gate to already be enabled, then pulses the
 * matching RCC peripheral-reset bit. This resets the complete I2C register
 * bank without changing application-owned RCC clock-gate, GPIO/AFIO, or NVIC
 * state.
 *
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @returns @ref driver_status_t "I2C root-deconfiguration operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: I2C register state was reset
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx` was unsupported
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The matching APB1 clock gate is disabled
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @note The APB1 clock gate, GPIO/AFIO route, and NVIC delivery retain their application-owned states
 */
driver_status_t I2C_DeConfig(I2C_TypeDef* const I2Cx);

/**
 * @brief Configures the supported controller timing baseline for one I2C instance
 * @details
 * Validates and stages the complete timing domain from @ref i2c_config_t using
 * the live APB1 frequency, then commits CR2, CCR, TRISE, and CR1 while the
 * peripheral remains disabled. The driver neither changes RCC gating nor
 * GPIO/AFIO routing, and leaves `I2Cx_CR1.PE` cleared on success.
 *
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @param[in] pConfig Complete supported I2C timing configuration
 * Expected values:
 * - Non-`NULL`: Timing configuration is read from @p `pConfig`
 * - `i2c_config_t::bus_speed`: An admitted I2C bus-speed preset
 * - `i2c_config_t::fast_mode_duty`: An admitted Fast-mode duty selector
 * @returns @ref driver_status_t "I2C root-configuration operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Supported I2C timing configuration was applied
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` or @p `pConfig` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx` was unsupported or a configuration selector was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The APB1 clock gate is disabled, PCLK1 is unsupported, or I2C is not disabled and idle
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The I2C bus is busy
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre `I2Cx_CR1.PE` is cleared and no START or STOP action is pending
 * @note Deferred IRQ, DMA, GPIO/AFIO, and NVIC domains are not configured by this API
 */
driver_status_t I2C_Config(I2C_TypeDef* const I2Cx, const i2c_config_t* const pConfig);

// ==================================================================================================== //
// I2C Controller Transfer APIs
// ==================================================================================================== //

/**
 * @brief Transmits one complete bounded message to a 7-bit I2C target
 * @details
 * Generates START, writes the encoded target address and all requested data,
 * waits for final byte transfer completion, then generates STOP. The timeout
 * is transaction-wide and the result records exact transmitted progress and
 * the primary typed hardware or software failure after START is requested.
 *
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @param[in] slaveAddress Unshifted 7-bit I2C target address
 * Accepted values:
 * - `0x08U` through `0x77U`: Non-reserved 7-bit target address
 * @param[in] pData Source data buffer
 * Expected values:
 * - Non-`NULL`: Source of @p `size` bytes; not modified by the Driver
 * @param[in] size Number of bytes to transmit
 * Accepted values:
 * - Nonzero: Complete message length in bytes
 * @param[in] timeout_ms Transaction-wide timeout in milliseconds
 * Accepted values:
 * - Nonzero: Total polling budget for the complete transaction
 * @param[out] pResult Transfer result destination
 * Expected values:
 * - Non-`NULL`: Exact progress and primary error are published after START
 * @returns @ref driver_status_t "I2C controller transmit operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: All requested bytes were transmitted and STOP was generated
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx`, @p `pData`, or @p `pResult` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx`, @p `address`, @p `size`, or @p `timeout_ms` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The APB1 clock gate is disabled or I2C is not configured and enabled
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The bus is busy or another owner holds the I2C instance
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: The transaction-wide timeout expired
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: The transfer failed; @p `pResult` identifies the primary cause after START
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre I2C is configured, enabled, idle, and exclusively owned by the caller
 * @note @p `pResult` is unchanged when preflight fails; after START it is published on every return
 */
driver_status_t I2C_MasterTransmit
(
	I2C_TypeDef* const			I2Cx,
	const i2c_address_t			slaveAddress,
	const i2c_data_t* const		pData,
	const i2c_data_length_t		size,
	const uint32_t				timeout_ms
);

/**
 * @brief Receives one complete bounded message from a 7-bit I2C target
 * @details
 * Generates START, addresses the target for read, receives the requested byte
 * count using the hardware-required one-, two-, or many-byte receive tail,
 * then generates STOP. The result records exact received progress and the
 * primary typed failure after START is requested.
 *
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @param[in] slaveAddress Unshifted 7-bit I2C target address
 * Accepted values:
 * - `0x08U` through `0x77U`: Non-reserved 7-bit target address
 * @param[in] startRegAddress Register address to start reading from
 * @param[out] pData Destination data buffer
 * Expected values:
 * - Non-`NULL`: Destination for @p `size` bytes
 * @param[in] size Number of bytes to receive
 * Accepted values:
 * - Nonzero: Complete message length in bytes
 * @param[in] timeout_ms Transaction-wide timeout in milliseconds
 * Accepted values:
 * - Nonzero: Total polling budget for the complete transaction
 * @param[out] pResult Transfer result destination
 * Expected values:
 * - Non-`NULL`: Exact progress and primary error are published after START
 * @returns @ref driver_status_t "I2C controller receive operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: All requested bytes were received and STOP was generated
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx`, @p `pData`, or @p `pResult` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx`, @p `address`, @p `size`, or @p `timeout_ms` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The APB1 clock gate is disabled or I2C is not configured and enabled
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The bus is busy or another owner holds the I2C instance
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: The transaction-wide timeout expired
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: The transfer failed; @p `pResult` identifies the primary cause after START
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre I2C is configured, enabled, idle, and exclusively owned by the caller
 * @note Bytes before `pResult->rxBytesTransferred` may be valid after a transfer failure; later destination bytes remain unchanged
 */
driver_status_t I2C_MasterReceive
(
	I2C_TypeDef* const			I2Cx,
	const i2c_address_t			slaveAddress,
	const i2c_reg_addr_t		startRegAddress,
	i2c_data_t* const			pData,
	const i2c_data_length_t		size,
	const uint32_t				timeout_ms
);

/** @} */ // I2C_03_Driver

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_H_ */
