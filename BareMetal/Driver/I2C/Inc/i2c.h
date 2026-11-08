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
#include "i2c_defines.h"

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
// I2C Root Configuration APIs
// ==================================================================================================== //

/**
 * @brief Configures the supported controller timing baseline for one I2C instance
 * @details
 * Validates and stages the complete timing domain from @ref i2c_config_t using
 * the live APB1 frequency, then commits CR2, CCR, TRISE, and CR1 while the
 * peripheral remains disabled. The driver neither changes RCC gating, pulses
 * the RCC reset, nor touches GPIO/AFIO routing, and leaves `I2Cx_CR1.PE`
 * cleared on success.
 *
 * @param[in] I2Cx I2C peripheral instance
 * Accepted values:
 * - `I2C1`
 * - `I2C2`
 * @param[in] pConfig Complete supported I2C timing configuration
 * Expected values:
 * - Non-`NULL`: Timing configuration is read from @p `pConfig`
 * - `i2c_config_t::busMode`: An admitted I2C bus-speed preset
 * - `i2c_config_t::fastModeDuty`: An admitted Fast-mode duty selector
 * @returns @ref driver_status_t "I2C root-configuration operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Supported I2C timing configuration was applied
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` or @p `pConfig` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx` was unsupported or a configuration selector was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_NOT_READY`: The matching APB1 clock gate is disabled
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: PCLK1 is unsupported, or I2C is not disabled and idle
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The I2C bus is busy
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre `I2Cx_CR1.PE` is cleared and no START or STOP action is pending
 * @note Deferred IRQ, DMA, GPIO/AFIO, and NVIC domains are not configured by this API
 */
driver_status_t I2C_Init(I2C_TypeDef* const I2Cx, const i2c_config_t* const pConfig);

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
 * @retval - @ref `DRIVER_STATUS_ERROR_NOT_READY`: The matching APB1 clock gate is disabled
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: The matching APB1 clock-gate mask is unresolved for the validated instance
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The I2C bus is busy
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre No transfer is in progress, so `I2Cx_SR2.BUSY` is clear
 * @note The APB1 clock gate, GPIO/AFIO route, and NVIC delivery retain their application-owned states
 */
driver_status_t I2C_DeInit(I2C_TypeDef* const I2Cx);

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
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The bus is busy or a START/STOP action is pending
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre The application owns the complete I2C transaction
 */
driver_status_t I2C_SetOperationState(I2C_TypeDef* const I2Cx, const driver_status_t operationState);

// ==================================================================================================== //
// I2C Controller Transfer APIs
// ==================================================================================================== //

/**
 * @brief Transmits one complete bounded message to a 7-bit I2C target
 * @details
 * Generates START, writes the encoded target address and all requested data,
 * waits for final byte transfer completion, then generates STOP. @p `timeout_ms`
 * is transaction-wide: START, the address phase, every payload byte, the final
 * byte-complete tail, the STOP request, and the closing bus-idle wait all draw
 * from one poll budget, so the call cannot exceed the single timeout it was
 * given. The result reports only the primary failure that stopped the transfer.
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
 * @returns @ref driver_status_t "I2C controller transmit operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: All requested bytes were transmitted and STOP was generated
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `I2Cx` or @p `pData` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `I2Cx`, @p `slaveAddress`, @p `size`, or @p `timeout_ms` was invalid
 * @retval - @ref `DRIVER_STATUS_ERROR_NOT_READY`: The APB1 clock gate is disabled
 * @retval - @ref `DRIVER_STATUS_ERROR_STATE`: I2C is not enabled, a START/STOP request is still pending, or a stale `SR1` error flag is latched
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: The bus is busy or another owner holds the I2C instance
 * @retval - @ref `DRIVER_STATUS_ERROR_TIMEOUT`: The transaction-wide timeout expired
 * @retval - @ref `DRIVER_STATUS_ERROR_NO_ACK`: The target did not acknowledge the address or a transferred byte
 * @retval - @ref `DRIVER_STATUS_ERROR_ARBITRATION_LOST`: Arbitration was lost to another master after START
 * @retval - @ref `DRIVER_STATUS_ERROR_OVERRUN`: An `OVR` condition was flagged after START
 * @retval - @ref `DRIVER_STATUS_ERROR_LINE_FAULT`: A line-level error (BERR, PECERR, SMBALERT, or SMBus TIMEOUT) was flagged after START
 * @retval - @ref `DRIVER_STATUS_ERROR_FAIL`: The transfer left master mode without an `SR1` error flag
 * @pre The application enabled the matching APB1 clock gate through the RCC driver
 * @pre I2C is configured, enabled, idle, and exclusively owned by the caller
 * @pre Every `SR1` error flag from an earlier aborted transaction was already acknowledged by the application
 * @note A failure after START enters one best-effort cleanup path: the primary
 * cause is preserved, an abort STOP is requested only while the peripheral still
 * reports master mode and no action is pending, and only the observed `SR1` error
 * flags are cleared. A peripheral that stays busy is left to application-level
 * bus recovery, and `CR1.SWRST` recovery is not performed by this API
 * @note A target that does not acknowledge the address or a data byte is
 * reported as @ref `DRIVER_STATUS_ERROR_NO_ACK`
 *  */
driver_status_t I2C_MasterTransmit
(
	I2C_TypeDef* const			I2Cx,
	const i2c_address_t			slaveAddress,
	const i2c_data_t* const		pData,
	const i2c_data_length_t		size,
	const uint32_t				timeout_ms
);

/** @} */ // I2C_03_Driver

// C++ Compatibility
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* I2C_H_ */
