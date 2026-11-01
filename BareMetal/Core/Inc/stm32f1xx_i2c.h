/**
 * @file stm32f1xx_i2c.h
 * @author Shrey Shah
 * @brief STM32F1 I2C Register-Layer Definitions
 * @version v1.1
 * @date 23-08-2026
 * @details
 * This file is the canonical Core-layer owner of the STM32F103C8T6 I2C
 * register map, peripheral spacing constant, and raw register-field
 * positions and masks. It depends only on `stm32f1xx_utils.h`; protocol
 * policy, timing calculations, transaction sequencing, and timeout handling
 * remain responsibilities of the I2C Driver stack.
 */

// Header Guard
#ifndef STM32F1XX_I2C_H_
#define STM32F1XX_I2C_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx_utils.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup I2C Inter-Integrated Circuit (I2C)
 * @brief STM32F1 two-wire master/slave, SMBus, interrupt, DMA, and error-detection infrastructure
 * @details
 * RM0008 describes the I2C1/I2C2 block as the hardware engine that:
 * - operates as an I2C master or slave in standard mode up to 100 kHz and fast
 *   mode up to 400 kHz,
 * - supports 7-bit and 10-bit addressing, dual slave addresses, general call,
 *   and multi-master arbitration,
 * - generates START, repeated START, acknowledge, and STOP protocol actions,
 * - transfers byte payloads through `I2Cx_DR`,
 * - exposes protocol events and errors through `I2Cx_SR1` and
 *   `I2Cx_SR2`,
 * - gates event, buffer, and error interrupts through `I2Cx_CR2`,
 * - requests DMA transfers through `I2Cx_CR2.DMAEN`,
 * - supports SMBus, packet error checking, alert, and timeout behavior.
 *
 * @section I2C_RegisterLayoutBrief Register Layout Brief
 * <table>
 * <tr><th>Register</th><th>Offset</th><th>Primary Practical Role</th></tr>
 * <tr><td><code>I2Cx_CR1</code></td><td><code>0x00</code></td><td>Peripheral/protocol enable state and START, STOP, PEC, alert, and reset actions</td></tr>
 * <tr><td><code>I2Cx_CR2</code></td><td><code>0x04</code></td><td>Peripheral-clock frequency, interrupt sources, and DMA request control</td></tr>
 * <tr><td><code>I2Cx_OAR1</code></td><td><code>0x08</code></td><td>Primary 7-bit or 10-bit slave address</td></tr>
 * <tr><td><code>I2Cx_OAR2</code></td><td><code>0x0C</code></td><td>Dual-address enable and secondary 7-bit slave address</td></tr>
 * <tr><td><code>I2Cx_DR</code></td><td><code>0x10</code></td><td>Receive/transmit byte data port</td></tr>
 * <tr><td><code>I2Cx_SR1</code></td><td><code>0x14</code></td><td>Protocol event state and software-clearable error state</td></tr>
 * <tr><td><code>I2Cx_SR2</code></td><td><code>0x18</code></td><td>Bus role, direction, address-match source, and PEC observation</td></tr>
 * <tr><td><code>I2Cx_CCR</code></td><td><code>0x1C</code></td><td>SCL divider, speed mode, and fast-mode duty cycle</td></tr>
 * <tr><td><code>I2Cx_TRISE</code></td><td><code>0x20</code></td><td>Maximum SCL rise-time count</td></tr>
 * </table>
 *
 * @section I2C_TimingBridge Timing Bridge
 * `I2Cx_CR2.FREQ` records the APB1 input clock in megahertz. In master mode,
 * `I2Cx_CCR` derives the SCL low/high periods and `I2Cx_TRISE` constrains
 * the permitted rise interval. Timing calculations belong above this raw
 * register layer because standard mode, fast-mode duty selection, input-clock
 * frequency, and requested bus rate must be validated together.
 *
 * This codebase documents I2C in three software layers:
 * - Register layer: @ref `stm32f1xx_i2c.h`
 * - Low-level control layer: @ref `i2c_ll.h`
 * - Driver/orchestration layer: @ref `i2c.h`
 */

/**
 * @defgroup I2C_01_RegisterLayer I2C Register Layer
 * @ingroup I2C
 * @brief Raw STM32F1 I2C register structure, field definitions, and device constants
 * @details
 * This layer mirrors hardware-visible state and action ports. Driver-facing
 * selectors, transaction policy, event handling, and timeout behavior belong
 * above this Core register layer.
 */

/**
 * @addtogroup I2C_01_RegisterLayer
 * @{
 */

// ==================================================================================================== //
// I2C Device Constants
// ==================================================================================================== //

/**
 * @brief Address-window spacing between consecutive I2C peripheral instances
 * @def I2C_PERIPHERAL_SIZE
 * @details
 * I2C1 and I2C2 each occupy a 1 KiB APB1 address window even though
 * @ref I2C_TypeDef maps only the implemented `0x24`-byte register block.
 * Use this constant for address-derived instance indexing; do not substitute
 * `sizeof(I2C_TypeDef)`.
 */
#define I2C_PERIPHERAL_SIZE			((uint32_t) 0x400UL)

// ==================================================================================================== //
// I2C Register Defines
// ==================================================================================================== //

/**
 * @defgroup I2C_01_RegisterLayer_00_RegisterDefines I2C Register Bit Positions and Masks
 * @ingroup I2C_01_RegisterLayer
 * @brief Raw I2C register positions, widths, masks, and field-bit values
 * @details
 * Single-bit fields expose a position, a mask built with @ref REG_BIT_MASK,
 * and an unsuffixed mask alias. Multi-bit fields additionally expose a width
 * and use @ref REG_FIELD_MASK. Raw field-bit values use
 * @ref REG_FIELD_VALUE so every positioned value has one shared construction
 * path.
 * @{
 */

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_CR1
// ---------------------------------------------------------------------------------------------------- //

/** @brief Peripheral enable (`I2Cx_CR1.PE`) bit position @def I2C_CR1_PE_Pos */
#define I2C_CR1_PE_Pos				((reg_bit_pos_t) 0U)
/** @brief Peripheral enable (`I2Cx_CR1.PE`) bit mask @def I2C_CR1_PE_Msk */
#define I2C_CR1_PE_Msk				REG_BIT_MASK(I2C_CR1_PE_Pos)
/** @brief Peripheral enable (`I2Cx_CR1.PE`) bit mask alias @def I2C_CR1_PE */
#define I2C_CR1_PE					I2C_CR1_PE_Msk

/** @brief SMBus mode selection (`I2Cx_CR1.SMBUS`) bit position @def I2C_CR1_SMBUS_Pos */
#define I2C_CR1_SMBUS_Pos			((reg_bit_pos_t) 1U)
/** @brief SMBus mode selection (`I2Cx_CR1.SMBUS`) bit mask @def I2C_CR1_SMBUS_Msk */
#define I2C_CR1_SMBUS_Msk			REG_BIT_MASK(I2C_CR1_SMBUS_Pos)
/** @brief SMBus mode selection (`I2Cx_CR1.SMBUS`) bit mask alias @def I2C_CR1_SMBUS */
#define I2C_CR1_SMBUS				I2C_CR1_SMBUS_Msk

/** @brief SMBus device-type selection (`I2Cx_CR1.SMBTYPE`) bit position @def I2C_CR1_SMBTYPE_Pos */
#define I2C_CR1_SMBTYPE_Pos			((reg_bit_pos_t) 3U)
/** @brief SMBus device-type selection (`I2Cx_CR1.SMBTYPE`) bit mask @def I2C_CR1_SMBTYPE_Msk */
#define I2C_CR1_SMBTYPE_Msk			REG_BIT_MASK(I2C_CR1_SMBTYPE_Pos)
/** @brief SMBus device-type selection (`I2Cx_CR1.SMBTYPE`) bit mask alias @def I2C_CR1_SMBTYPE */
#define I2C_CR1_SMBTYPE				I2C_CR1_SMBTYPE_Msk

/** @brief ARP protocol enable (`I2Cx_CR1.ENARP`) bit position @def I2C_CR1_ENARP_Pos */
#define I2C_CR1_ENARP_Pos			((reg_bit_pos_t) 4U)
/** @brief ARP protocol enable (`I2Cx_CR1.ENARP`) bit mask @def I2C_CR1_ENARP_Msk */
#define I2C_CR1_ENARP_Msk			REG_BIT_MASK(I2C_CR1_ENARP_Pos)
/** @brief ARP protocol enable (`I2Cx_CR1.ENARP`) bit mask alias @def I2C_CR1_ENARP */
#define I2C_CR1_ENARP				I2C_CR1_ENARP_Msk

/** @brief Packet error checking enable (`I2Cx_CR1.ENPEC`) bit position @def I2C_CR1_ENPEC_Pos */
#define I2C_CR1_ENPEC_Pos			((reg_bit_pos_t) 5U)
/** @brief Packet error checking enable (`I2Cx_CR1.ENPEC`) bit mask @def I2C_CR1_ENPEC_Msk */
#define I2C_CR1_ENPEC_Msk			REG_BIT_MASK(I2C_CR1_ENPEC_Pos)
/** @brief Packet error checking enable (`I2Cx_CR1.ENPEC`) bit mask alias @def I2C_CR1_ENPEC */
#define I2C_CR1_ENPEC				I2C_CR1_ENPEC_Msk

/** @brief General-call recognition enable (`I2Cx_CR1.ENGC`) bit position @def I2C_CR1_ENGC_Pos */
#define I2C_CR1_ENGC_Pos			((reg_bit_pos_t) 6U)
/** @brief General-call recognition enable (`I2Cx_CR1.ENGC`) bit mask @def I2C_CR1_ENGC_Msk */
#define I2C_CR1_ENGC_Msk			REG_BIT_MASK(I2C_CR1_ENGC_Pos)
/** @brief General-call recognition enable (`I2Cx_CR1.ENGC`) bit mask alias @def I2C_CR1_ENGC */
#define I2C_CR1_ENGC				I2C_CR1_ENGC_Msk

/** @brief Clock stretching disable in slave mode (`I2Cx_CR1.NOSTRETCH`) bit position @def I2C_CR1_NOSTRETCH_Pos */
#define I2C_CR1_NOSTRETCH_Pos		((reg_bit_pos_t) 7U)
/** @brief Clock stretching disable in slave mode (`I2Cx_CR1.NOSTRETCH`) bit mask @def I2C_CR1_NOSTRETCH_Msk */
#define I2C_CR1_NOSTRETCH_Msk		REG_BIT_MASK(I2C_CR1_NOSTRETCH_Pos)
/** @brief Clock stretching disable in slave mode (`I2Cx_CR1.NOSTRETCH`) bit mask alias @def I2C_CR1_NOSTRETCH */
#define I2C_CR1_NOSTRETCH			I2C_CR1_NOSTRETCH_Msk

/** @brief START condition generation request (`I2Cx_CR1.START`) bit position @def I2C_CR1_START_Pos */
#define I2C_CR1_START_Pos			((reg_bit_pos_t) 8U)
/** @brief START condition generation request (`I2Cx_CR1.START`) bit mask @def I2C_CR1_START_Msk */
#define I2C_CR1_START_Msk			REG_BIT_MASK(I2C_CR1_START_Pos)
/** @brief START condition generation request (`I2Cx_CR1.START`) bit mask alias @def I2C_CR1_START */
#define I2C_CR1_START				I2C_CR1_START_Msk

/** @brief STOP condition generation request (`I2Cx_CR1.STOP`) bit position @def I2C_CR1_STOP_Pos */
#define I2C_CR1_STOP_Pos			((reg_bit_pos_t) 9U)
/** @brief STOP condition generation request (`I2Cx_CR1.STOP`) bit mask @def I2C_CR1_STOP_Msk */
#define I2C_CR1_STOP_Msk			REG_BIT_MASK(I2C_CR1_STOP_Pos)
/** @brief STOP condition generation request (`I2Cx_CR1.STOP`) bit mask alias @def I2C_CR1_STOP */
#define I2C_CR1_STOP				I2C_CR1_STOP_Msk

/** @brief Acknowledge generation enable (`I2Cx_CR1.ACK`) bit position @def I2C_CR1_ACK_Pos */
#define I2C_CR1_ACK_Pos				((reg_bit_pos_t) 10U)
/** @brief Acknowledge generation enable (`I2Cx_CR1.ACK`) bit mask @def I2C_CR1_ACK_Msk */
#define I2C_CR1_ACK_Msk				REG_BIT_MASK(I2C_CR1_ACK_Pos)
/** @brief Acknowledge generation enable (`I2Cx_CR1.ACK`) bit mask alias @def I2C_CR1_ACK */
#define I2C_CR1_ACK					I2C_CR1_ACK_Msk

/** @brief Acknowledge or PEC position selection (`I2Cx_CR1.POS`) bit position @def I2C_CR1_POS_Pos */
#define I2C_CR1_POS_Pos				((reg_bit_pos_t) 11U)
/** @brief Acknowledge or PEC position selection (`I2Cx_CR1.POS`) bit mask @def I2C_CR1_POS_Msk */
#define I2C_CR1_POS_Msk				REG_BIT_MASK(I2C_CR1_POS_Pos)
/** @brief Acknowledge or PEC position selection (`I2Cx_CR1.POS`) bit mask alias @def I2C_CR1_POS */
#define I2C_CR1_POS					I2C_CR1_POS_Msk

/** @brief Packet error checking transfer request (`I2Cx_CR1.PEC`) bit position @def I2C_CR1_PEC_Pos */
#define I2C_CR1_PEC_Pos				((reg_bit_pos_t) 12U)
/** @brief Packet error checking transfer request (`I2Cx_CR1.PEC`) bit mask @def I2C_CR1_PEC_Msk */
#define I2C_CR1_PEC_Msk				REG_BIT_MASK(I2C_CR1_PEC_Pos)
/** @brief Packet error checking transfer request (`I2Cx_CR1.PEC`) bit mask alias @def I2C_CR1_PEC */
#define I2C_CR1_PEC					I2C_CR1_PEC_Msk

/** @brief SMBus alert request (`I2Cx_CR1.ALERT`) bit position @def I2C_CR1_ALERT_Pos */
#define I2C_CR1_ALERT_Pos			((reg_bit_pos_t) 13U)
/** @brief SMBus alert request (`I2Cx_CR1.ALERT`) bit mask @def I2C_CR1_ALERT_Msk */
#define I2C_CR1_ALERT_Msk			REG_BIT_MASK(I2C_CR1_ALERT_Pos)
/** @brief SMBus alert request (`I2Cx_CR1.ALERT`) bit mask alias @def I2C_CR1_ALERT */
#define I2C_CR1_ALERT				I2C_CR1_ALERT_Msk

/** @brief Software reset request (`I2Cx_CR1.SWRST`) bit position @def I2C_CR1_SWRST_Pos */
#define I2C_CR1_SWRST_Pos			((reg_bit_pos_t) 15U)
/** @brief Software reset request (`I2Cx_CR1.SWRST`) bit mask @def I2C_CR1_SWRST_Msk */
#define I2C_CR1_SWRST_Msk			REG_BIT_MASK(I2C_CR1_SWRST_Pos)
/** @brief Software reset request (`I2Cx_CR1.SWRST`) bit mask alias @def I2C_CR1_SWRST */
#define I2C_CR1_SWRST				I2C_CR1_SWRST_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_CR2
// ---------------------------------------------------------------------------------------------------- //

/** @brief Peripheral input-clock frequency in megahertz (`I2Cx_CR2.FREQ[5:0]`) field position @def I2C_CR2_FREQ_Pos */
#define I2C_CR2_FREQ_Pos			((reg_bit_pos_t) 0U)
/** @brief Peripheral input-clock frequency in megahertz (`I2Cx_CR2.FREQ[5:0]`) field width @def I2C_CR2_FREQ_Width */
#define I2C_CR2_FREQ_Width			((reg_field_width_t) 6U)
/** @brief Peripheral input-clock frequency in megahertz (`I2Cx_CR2.FREQ[5:0]`) field mask @def I2C_CR2_FREQ_Msk */
#define I2C_CR2_FREQ_Msk			REG_FIELD_MASK(I2C_CR2_FREQ_Pos, I2C_CR2_FREQ_Width)
/** @brief Peripheral input-clock frequency in megahertz (`I2Cx_CR2.FREQ[5:0]`) field mask alias @def I2C_CR2_FREQ */
#define I2C_CR2_FREQ				I2C_CR2_FREQ_Msk

/** @brief Peripheral input-clock frequency (`I2Cx_CR2.FREQ[0]`) raw field bit 0 mask @def I2C_CR2_FREQ_0 */
#define I2C_CR2_FREQ_0				REG_FIELD_VALUE(I2C_CR2_FREQ_Pos, 0x01UL)

/** @brief Peripheral input-clock frequency (`I2Cx_CR2.FREQ[1]`) raw field bit 1 mask @def I2C_CR2_FREQ_1 */
#define I2C_CR2_FREQ_1				REG_FIELD_VALUE(I2C_CR2_FREQ_Pos, 0x02UL)

/** @brief Peripheral input-clock frequency (`I2Cx_CR2.FREQ[2]`) raw field bit 2 mask @def I2C_CR2_FREQ_2 */
#define I2C_CR2_FREQ_2				REG_FIELD_VALUE(I2C_CR2_FREQ_Pos, 0x04UL)

/** @brief Peripheral input-clock frequency (`I2Cx_CR2.FREQ[3]`) raw field bit 3 mask @def I2C_CR2_FREQ_3 */
#define I2C_CR2_FREQ_3				REG_FIELD_VALUE(I2C_CR2_FREQ_Pos, 0x08UL)

/** @brief Peripheral input-clock frequency (`I2Cx_CR2.FREQ[4]`) raw field bit 4 mask @def I2C_CR2_FREQ_4 */
#define I2C_CR2_FREQ_4				REG_FIELD_VALUE(I2C_CR2_FREQ_Pos, 0x10UL)

/** @brief Peripheral input-clock frequency (`I2Cx_CR2.FREQ[5]`) raw field bit 5 mask @def I2C_CR2_FREQ_5 */
#define I2C_CR2_FREQ_5				REG_FIELD_VALUE(I2C_CR2_FREQ_Pos, 0x20UL)

/** @brief Error interrupt enable (`I2Cx_CR2.ITERREN`) bit position @def I2C_CR2_ITERREN_Pos */
#define I2C_CR2_ITERREN_Pos			((reg_bit_pos_t) 8U)
/** @brief Error interrupt enable (`I2Cx_CR2.ITERREN`) bit mask @def I2C_CR2_ITERREN_Msk */
#define I2C_CR2_ITERREN_Msk			REG_BIT_MASK(I2C_CR2_ITERREN_Pos)
/** @brief Error interrupt enable (`I2Cx_CR2.ITERREN`) bit mask alias @def I2C_CR2_ITERREN */
#define I2C_CR2_ITERREN				I2C_CR2_ITERREN_Msk

/** @brief Event interrupt enable (`I2Cx_CR2.ITEVTEN`) bit position @def I2C_CR2_ITEVTEN_Pos */
#define I2C_CR2_ITEVTEN_Pos			((reg_bit_pos_t) 9U)
/** @brief Event interrupt enable (`I2Cx_CR2.ITEVTEN`) bit mask @def I2C_CR2_ITEVTEN_Msk */
#define I2C_CR2_ITEVTEN_Msk			REG_BIT_MASK(I2C_CR2_ITEVTEN_Pos)
/** @brief Event interrupt enable (`I2Cx_CR2.ITEVTEN`) bit mask alias @def I2C_CR2_ITEVTEN */
#define I2C_CR2_ITEVTEN				I2C_CR2_ITEVTEN_Msk

/** @brief Buffer interrupt enable (`I2Cx_CR2.ITBUFEN`) bit position @def I2C_CR2_ITBUFEN_Pos */
#define I2C_CR2_ITBUFEN_Pos			((reg_bit_pos_t) 10U)
/** @brief Buffer interrupt enable (`I2Cx_CR2.ITBUFEN`) bit mask @def I2C_CR2_ITBUFEN_Msk */
#define I2C_CR2_ITBUFEN_Msk			REG_BIT_MASK(I2C_CR2_ITBUFEN_Pos)
/** @brief Buffer interrupt enable (`I2Cx_CR2.ITBUFEN`) bit mask alias @def I2C_CR2_ITBUFEN */
#define I2C_CR2_ITBUFEN				I2C_CR2_ITBUFEN_Msk

/** @brief DMA request enable (`I2Cx_CR2.DMAEN`) bit position @def I2C_CR2_DMAEN_Pos */
#define I2C_CR2_DMAEN_Pos			((reg_bit_pos_t) 11U)
/** @brief DMA request enable (`I2Cx_CR2.DMAEN`) bit mask @def I2C_CR2_DMAEN_Msk */
#define I2C_CR2_DMAEN_Msk			REG_BIT_MASK(I2C_CR2_DMAEN_Pos)
/** @brief DMA request enable (`I2Cx_CR2.DMAEN`) bit mask alias @def I2C_CR2_DMAEN */
#define I2C_CR2_DMAEN				I2C_CR2_DMAEN_Msk

/** @brief DMA last-transfer selection (`I2Cx_CR2.LAST`) bit position @def I2C_CR2_LAST_Pos */
#define I2C_CR2_LAST_Pos			((reg_bit_pos_t) 12U)
/** @brief DMA last-transfer selection (`I2Cx_CR2.LAST`) bit mask @def I2C_CR2_LAST_Msk */
#define I2C_CR2_LAST_Msk			REG_BIT_MASK(I2C_CR2_LAST_Pos)
/** @brief DMA last-transfer selection (`I2Cx_CR2.LAST`) bit mask alias @def I2C_CR2_LAST */
#define I2C_CR2_LAST				I2C_CR2_LAST_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_OAR1
// ---------------------------------------------------------------------------------------------------- //

/** @brief Own-address bit 0 (`I2Cx_OAR1.ADD0`) bit position @def I2C_OAR1_ADD0_Pos */
#define I2C_OAR1_ADD0_Pos			((reg_bit_pos_t) 0U)
/** @brief Own-address bit 0 (`I2Cx_OAR1.ADD0`) bit mask @def I2C_OAR1_ADD0_Msk */
#define I2C_OAR1_ADD0_Msk			REG_BIT_MASK(I2C_OAR1_ADD0_Pos)
/** @brief Own-address bit 0 (`I2Cx_OAR1.ADD0`) bit mask alias @def I2C_OAR1_ADD0 */
#define I2C_OAR1_ADD0				I2C_OAR1_ADD0_Msk

/** @brief Seven-bit own-address field (`I2Cx_OAR1.ADD[7:1]`) field position @def I2C_OAR1_ADD1_7_Pos */
#define I2C_OAR1_ADD1_7_Pos			((reg_bit_pos_t) 1U)
/** @brief Seven-bit own-address field (`I2Cx_OAR1.ADD[7:1]`) field width @def I2C_OAR1_ADD1_7_Width */
#define I2C_OAR1_ADD1_7_Width		((reg_field_width_t) 7U)
/** @brief Seven-bit own-address field (`I2Cx_OAR1.ADD[7:1]`) field mask @def I2C_OAR1_ADD1_7_Msk */
#define I2C_OAR1_ADD1_7_Msk			REG_FIELD_MASK(I2C_OAR1_ADD1_7_Pos, I2C_OAR1_ADD1_7_Width)
/** @brief Seven-bit own-address field (`I2Cx_OAR1.ADD[7:1]`) field mask alias @def I2C_OAR1_ADD1_7 */
#define I2C_OAR1_ADD1_7				I2C_OAR1_ADD1_7_Msk

/** @brief Own-address bit 1 (`I2Cx_OAR1.ADD1`) bit position @def I2C_OAR1_ADD1_Pos */
#define I2C_OAR1_ADD1_Pos			((reg_bit_pos_t) 1U)
/** @brief Own-address bit 1 (`I2Cx_OAR1.ADD1`) bit mask @def I2C_OAR1_ADD1_Msk */
#define I2C_OAR1_ADD1_Msk			REG_BIT_MASK(I2C_OAR1_ADD1_Pos)
/** @brief Own-address bit 1 (`I2Cx_OAR1.ADD1`) bit mask alias @def I2C_OAR1_ADD1 */
#define I2C_OAR1_ADD1				I2C_OAR1_ADD1_Msk

/** @brief Own-address bit 2 (`I2Cx_OAR1.ADD2`) bit position @def I2C_OAR1_ADD2_Pos */
#define I2C_OAR1_ADD2_Pos			((reg_bit_pos_t) 2U)
/** @brief Own-address bit 2 (`I2Cx_OAR1.ADD2`) bit mask @def I2C_OAR1_ADD2_Msk */
#define I2C_OAR1_ADD2_Msk			REG_BIT_MASK(I2C_OAR1_ADD2_Pos)
/** @brief Own-address bit 2 (`I2Cx_OAR1.ADD2`) bit mask alias @def I2C_OAR1_ADD2 */
#define I2C_OAR1_ADD2				I2C_OAR1_ADD2_Msk

/** @brief Own-address bit 3 (`I2Cx_OAR1.ADD3`) bit position @def I2C_OAR1_ADD3_Pos */
#define I2C_OAR1_ADD3_Pos			((reg_bit_pos_t) 3U)
/** @brief Own-address bit 3 (`I2Cx_OAR1.ADD3`) bit mask @def I2C_OAR1_ADD3_Msk */
#define I2C_OAR1_ADD3_Msk			REG_BIT_MASK(I2C_OAR1_ADD3_Pos)
/** @brief Own-address bit 3 (`I2Cx_OAR1.ADD3`) bit mask alias @def I2C_OAR1_ADD3 */
#define I2C_OAR1_ADD3				I2C_OAR1_ADD3_Msk

/** @brief Own-address bit 4 (`I2Cx_OAR1.ADD4`) bit position @def I2C_OAR1_ADD4_Pos */
#define I2C_OAR1_ADD4_Pos			((reg_bit_pos_t) 4U)
/** @brief Own-address bit 4 (`I2Cx_OAR1.ADD4`) bit mask @def I2C_OAR1_ADD4_Msk */
#define I2C_OAR1_ADD4_Msk			REG_BIT_MASK(I2C_OAR1_ADD4_Pos)
/** @brief Own-address bit 4 (`I2Cx_OAR1.ADD4`) bit mask alias @def I2C_OAR1_ADD4 */
#define I2C_OAR1_ADD4				I2C_OAR1_ADD4_Msk

/** @brief Own-address bit 5 (`I2Cx_OAR1.ADD5`) bit position @def I2C_OAR1_ADD5_Pos */
#define I2C_OAR1_ADD5_Pos			((reg_bit_pos_t) 5U)
/** @brief Own-address bit 5 (`I2Cx_OAR1.ADD5`) bit mask @def I2C_OAR1_ADD5_Msk */
#define I2C_OAR1_ADD5_Msk			REG_BIT_MASK(I2C_OAR1_ADD5_Pos)
/** @brief Own-address bit 5 (`I2Cx_OAR1.ADD5`) bit mask alias @def I2C_OAR1_ADD5 */
#define I2C_OAR1_ADD5				I2C_OAR1_ADD5_Msk

/** @brief Own-address bit 6 (`I2Cx_OAR1.ADD6`) bit position @def I2C_OAR1_ADD6_Pos */
#define I2C_OAR1_ADD6_Pos			((reg_bit_pos_t) 6U)
/** @brief Own-address bit 6 (`I2Cx_OAR1.ADD6`) bit mask @def I2C_OAR1_ADD6_Msk */
#define I2C_OAR1_ADD6_Msk			REG_BIT_MASK(I2C_OAR1_ADD6_Pos)
/** @brief Own-address bit 6 (`I2Cx_OAR1.ADD6`) bit mask alias @def I2C_OAR1_ADD6 */
#define I2C_OAR1_ADD6				I2C_OAR1_ADD6_Msk

/** @brief Own-address bit 7 (`I2Cx_OAR1.ADD7`) bit position @def I2C_OAR1_ADD7_Pos */
#define I2C_OAR1_ADD7_Pos			((reg_bit_pos_t) 7U)
/** @brief Own-address bit 7 (`I2Cx_OAR1.ADD7`) bit mask @def I2C_OAR1_ADD7_Msk */
#define I2C_OAR1_ADD7_Msk			REG_BIT_MASK(I2C_OAR1_ADD7_Pos)
/** @brief Own-address bit 7 (`I2Cx_OAR1.ADD7`) bit mask alias @def I2C_OAR1_ADD7 */
#define I2C_OAR1_ADD7				I2C_OAR1_ADD7_Msk

/** @brief Ten-bit own-address upper field (`I2Cx_OAR1.ADD[9:8]`) field position @def I2C_OAR1_ADD8_9_Pos */
#define I2C_OAR1_ADD8_9_Pos			((reg_bit_pos_t) 8U)
/** @brief Ten-bit own-address upper field (`I2Cx_OAR1.ADD[9:8]`) field width @def I2C_OAR1_ADD8_9_Width */
#define I2C_OAR1_ADD8_9_Width		((reg_field_width_t) 2U)
/** @brief Ten-bit own-address upper field (`I2Cx_OAR1.ADD[9:8]`) field mask @def I2C_OAR1_ADD8_9_Msk */
#define I2C_OAR1_ADD8_9_Msk			REG_FIELD_MASK(I2C_OAR1_ADD8_9_Pos, I2C_OAR1_ADD8_9_Width)
/** @brief Ten-bit own-address upper field (`I2Cx_OAR1.ADD[9:8]`) field mask alias @def I2C_OAR1_ADD8_9 */
#define I2C_OAR1_ADD8_9				I2C_OAR1_ADD8_9_Msk

/** @brief Own-address bit 8 (`I2Cx_OAR1.ADD8`) bit position @def I2C_OAR1_ADD8_Pos */
#define I2C_OAR1_ADD8_Pos			((reg_bit_pos_t) 8U)
/** @brief Own-address bit 8 (`I2Cx_OAR1.ADD8`) bit mask @def I2C_OAR1_ADD8_Msk */
#define I2C_OAR1_ADD8_Msk			REG_BIT_MASK(I2C_OAR1_ADD8_Pos)
/** @brief Own-address bit 8 (`I2Cx_OAR1.ADD8`) bit mask alias @def I2C_OAR1_ADD8 */
#define I2C_OAR1_ADD8				I2C_OAR1_ADD8_Msk

/** @brief Own-address bit 9 (`I2Cx_OAR1.ADD9`) bit position @def I2C_OAR1_ADD9_Pos */
#define I2C_OAR1_ADD9_Pos			((reg_bit_pos_t) 9U)
/** @brief Own-address bit 9 (`I2Cx_OAR1.ADD9`) bit mask @def I2C_OAR1_ADD9_Msk */
#define I2C_OAR1_ADD9_Msk			REG_BIT_MASK(I2C_OAR1_ADD9_Pos)
/** @brief Own-address bit 9 (`I2Cx_OAR1.ADD9`) bit mask alias @def I2C_OAR1_ADD9 */
#define I2C_OAR1_ADD9				I2C_OAR1_ADD9_Msk

/** @brief Own-address mode selection (`I2Cx_OAR1.ADDMODE`) bit position @def I2C_OAR1_ADDMODE_Pos */
#define I2C_OAR1_ADDMODE_Pos		((reg_bit_pos_t) 15U)
/** @brief Own-address mode selection (`I2Cx_OAR1.ADDMODE`) bit mask @def I2C_OAR1_ADDMODE_Msk */
#define I2C_OAR1_ADDMODE_Msk		REG_BIT_MASK(I2C_OAR1_ADDMODE_Pos)
/** @brief Own-address mode selection (`I2Cx_OAR1.ADDMODE`) bit mask alias @def I2C_OAR1_ADDMODE */
#define I2C_OAR1_ADDMODE			I2C_OAR1_ADDMODE_Msk

/** @brief Mandatory reserved bit 14 that configuration must keep at one (`I2Cx_OAR1`) bit position @def I2C_OAR1_BIT14_MUST_SET_Pos */
#define I2C_OAR1_BIT14_MUST_SET_Pos	((reg_bit_pos_t) 14U)
/** @brief Mandatory reserved bit 14 that configuration must keep at one (`I2Cx_OAR1`) bit mask @def I2C_OAR1_BIT14_MUST_SET_Msk */
#define I2C_OAR1_BIT14_MUST_SET_Msk	REG_BIT_MASK(I2C_OAR1_BIT14_MUST_SET_Pos)
/** @brief Mandatory reserved bit 14 that configuration must keep at one (`I2Cx_OAR1`) bit mask alias @def I2C_OAR1_BIT14_MUST_SET */
#define I2C_OAR1_BIT14_MUST_SET		I2C_OAR1_BIT14_MUST_SET_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_OAR2
// ---------------------------------------------------------------------------------------------------- //

/** @brief Dual-addressing enable (`I2Cx_OAR2.ENDUAL`) bit position @def I2C_OAR2_ENDUAL_Pos */
#define I2C_OAR2_ENDUAL_Pos			((reg_bit_pos_t) 0U)
/** @brief Dual-addressing enable (`I2Cx_OAR2.ENDUAL`) bit mask @def I2C_OAR2_ENDUAL_Msk */
#define I2C_OAR2_ENDUAL_Msk			REG_BIT_MASK(I2C_OAR2_ENDUAL_Pos)
/** @brief Dual-addressing enable (`I2Cx_OAR2.ENDUAL`) bit mask alias @def I2C_OAR2_ENDUAL */
#define I2C_OAR2_ENDUAL				I2C_OAR2_ENDUAL_Msk

/** @brief Second seven-bit own address (`I2Cx_OAR2.ADD2[7:1]`) field position @def I2C_OAR2_ADD2_Pos */
#define I2C_OAR2_ADD2_Pos			((reg_bit_pos_t) 1U)
/** @brief Second seven-bit own address (`I2Cx_OAR2.ADD2[7:1]`) field width @def I2C_OAR2_ADD2_Width */
#define I2C_OAR2_ADD2_Width			((reg_field_width_t) 7U)
/** @brief Second seven-bit own address (`I2Cx_OAR2.ADD2[7:1]`) field mask @def I2C_OAR2_ADD2_Msk */
#define I2C_OAR2_ADD2_Msk			REG_FIELD_MASK(I2C_OAR2_ADD2_Pos, I2C_OAR2_ADD2_Width)
/** @brief Second seven-bit own address (`I2Cx_OAR2.ADD2[7:1]`) field mask alias @def I2C_OAR2_ADD2 */
#define I2C_OAR2_ADD2				I2C_OAR2_ADD2_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_DR
// ---------------------------------------------------------------------------------------------------- //

/** @brief Transmit or receive data payload (`I2Cx_DR.DR[7:0]`) field position @def I2C_DR_DR_Pos */
#define I2C_DR_DR_Pos				((reg_bit_pos_t) 0U)
/** @brief Transmit or receive data payload (`I2Cx_DR.DR[7:0]`) field width @def I2C_DR_DR_Width */
#define I2C_DR_DR_Width				((reg_field_width_t) 8U)
/** @brief Transmit or receive data payload (`I2Cx_DR.DR[7:0]`) field mask @def I2C_DR_DR_Msk */
#define I2C_DR_DR_Msk				REG_FIELD_MASK(I2C_DR_DR_Pos, I2C_DR_DR_Width)
/** @brief Transmit or receive data payload (`I2Cx_DR.DR[7:0]`) field mask alias @def I2C_DR_DR */
#define I2C_DR_DR					I2C_DR_DR_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_SR1
// ---------------------------------------------------------------------------------------------------- //

/** @brief START condition generated event (`I2Cx_SR1.SB`) bit position @def I2C_SR1_SB_Pos */
#define I2C_SR1_SB_Pos				((reg_bit_pos_t) 0U)
/** @brief START condition generated event (`I2Cx_SR1.SB`) bit mask @def I2C_SR1_SB_Msk */
#define I2C_SR1_SB_Msk				REG_BIT_MASK(I2C_SR1_SB_Pos)
/** @brief START condition generated event (`I2Cx_SR1.SB`) bit mask alias @def I2C_SR1_SB */
#define I2C_SR1_SB					I2C_SR1_SB_Msk

/** @brief Address sent or matched event (`I2Cx_SR1.ADDR`) bit position @def I2C_SR1_ADDR_Pos */
#define I2C_SR1_ADDR_Pos			((reg_bit_pos_t) 1U)
/** @brief Address sent or matched event (`I2Cx_SR1.ADDR`) bit mask @def I2C_SR1_ADDR_Msk */
#define I2C_SR1_ADDR_Msk			REG_BIT_MASK(I2C_SR1_ADDR_Pos)
/** @brief Address sent or matched event (`I2Cx_SR1.ADDR`) bit mask alias @def I2C_SR1_ADDR */
#define I2C_SR1_ADDR				I2C_SR1_ADDR_Msk

/** @brief Byte transfer finished event (`I2Cx_SR1.BTF`) bit position @def I2C_SR1_BTF_Pos */
#define I2C_SR1_BTF_Pos				((reg_bit_pos_t) 2U)
/** @brief Byte transfer finished event (`I2Cx_SR1.BTF`) bit mask @def I2C_SR1_BTF_Msk */
#define I2C_SR1_BTF_Msk				REG_BIT_MASK(I2C_SR1_BTF_Pos)
/** @brief Byte transfer finished event (`I2Cx_SR1.BTF`) bit mask alias @def I2C_SR1_BTF */
#define I2C_SR1_BTF					I2C_SR1_BTF_Msk

/** @brief Ten-bit header sent event (`I2Cx_SR1.ADD10`) bit position @def I2C_SR1_ADD10_Pos */
#define I2C_SR1_ADD10_Pos			((reg_bit_pos_t) 3U)
/** @brief Ten-bit header sent event (`I2Cx_SR1.ADD10`) bit mask @def I2C_SR1_ADD10_Msk */
#define I2C_SR1_ADD10_Msk			REG_BIT_MASK(I2C_SR1_ADD10_Pos)
/** @brief Ten-bit header sent event (`I2Cx_SR1.ADD10`) bit mask alias @def I2C_SR1_ADD10 */
#define I2C_SR1_ADD10				I2C_SR1_ADD10_Msk

/** @brief STOP detection event (`I2Cx_SR1.STOPF`) bit position @def I2C_SR1_STOPF_Pos */
#define I2C_SR1_STOPF_Pos			((reg_bit_pos_t) 4U)
/** @brief STOP detection event (`I2Cx_SR1.STOPF`) bit mask @def I2C_SR1_STOPF_Msk */
#define I2C_SR1_STOPF_Msk			REG_BIT_MASK(I2C_SR1_STOPF_Pos)
/** @brief STOP detection event (`I2Cx_SR1.STOPF`) bit mask alias @def I2C_SR1_STOPF */
#define I2C_SR1_STOPF				I2C_SR1_STOPF_Msk

/** @brief Receive data register not empty state (`I2Cx_SR1.RXNE`) bit position @def I2C_SR1_RXNE_Pos */
#define I2C_SR1_RXNE_Pos			((reg_bit_pos_t) 6U)
/** @brief Receive data register not empty state (`I2Cx_SR1.RXNE`) bit mask @def I2C_SR1_RXNE_Msk */
#define I2C_SR1_RXNE_Msk			REG_BIT_MASK(I2C_SR1_RXNE_Pos)
/** @brief Receive data register not empty state (`I2Cx_SR1.RXNE`) bit mask alias @def I2C_SR1_RXNE */
#define I2C_SR1_RXNE				I2C_SR1_RXNE_Msk

/** @brief Transmit data register empty state (`I2Cx_SR1.TXE`) bit position @def I2C_SR1_TXE_Pos */
#define I2C_SR1_TXE_Pos				((reg_bit_pos_t) 7U)
/** @brief Transmit data register empty state (`I2Cx_SR1.TXE`) bit mask @def I2C_SR1_TXE_Msk */
#define I2C_SR1_TXE_Msk				REG_BIT_MASK(I2C_SR1_TXE_Pos)
/** @brief Transmit data register empty state (`I2Cx_SR1.TXE`) bit mask alias @def I2C_SR1_TXE */
#define I2C_SR1_TXE					I2C_SR1_TXE_Msk

/** @brief Bus error event (`I2Cx_SR1.BERR`) bit position @def I2C_SR1_BERR_Pos */
#define I2C_SR1_BERR_Pos			((reg_bit_pos_t) 8U)
/** @brief Bus error event (`I2Cx_SR1.BERR`) bit mask @def I2C_SR1_BERR_Msk */
#define I2C_SR1_BERR_Msk			REG_BIT_MASK(I2C_SR1_BERR_Pos)
/** @brief Bus error event (`I2Cx_SR1.BERR`) bit mask alias @def I2C_SR1_BERR */
#define I2C_SR1_BERR				I2C_SR1_BERR_Msk

/** @brief Arbitration lost event (`I2Cx_SR1.ARLO`) bit position @def I2C_SR1_ARLO_Pos */
#define I2C_SR1_ARLO_Pos			((reg_bit_pos_t) 9U)
/** @brief Arbitration lost event (`I2Cx_SR1.ARLO`) bit mask @def I2C_SR1_ARLO_Msk */
#define I2C_SR1_ARLO_Msk			REG_BIT_MASK(I2C_SR1_ARLO_Pos)
/** @brief Arbitration lost event (`I2Cx_SR1.ARLO`) bit mask alias @def I2C_SR1_ARLO */
#define I2C_SR1_ARLO				I2C_SR1_ARLO_Msk

/** @brief Acknowledge failure event (`I2Cx_SR1.AF`) bit position @def I2C_SR1_AF_Pos */
#define I2C_SR1_AF_Pos				((reg_bit_pos_t) 10U)
/** @brief Acknowledge failure event (`I2Cx_SR1.AF`) bit mask @def I2C_SR1_AF_Msk */
#define I2C_SR1_AF_Msk				REG_BIT_MASK(I2C_SR1_AF_Pos)
/** @brief Acknowledge failure event (`I2Cx_SR1.AF`) bit mask alias @def I2C_SR1_AF */
#define I2C_SR1_AF					I2C_SR1_AF_Msk

/** @brief Overrun or underrun event (`I2Cx_SR1.OVR`) bit position @def I2C_SR1_OVR_Pos */
#define I2C_SR1_OVR_Pos				((reg_bit_pos_t) 11U)
/** @brief Overrun or underrun event (`I2Cx_SR1.OVR`) bit mask @def I2C_SR1_OVR_Msk */
#define I2C_SR1_OVR_Msk				REG_BIT_MASK(I2C_SR1_OVR_Pos)
/** @brief Overrun or underrun event (`I2Cx_SR1.OVR`) bit mask alias @def I2C_SR1_OVR */
#define I2C_SR1_OVR					I2C_SR1_OVR_Msk

/** @brief Packet error checking mismatch event (`I2Cx_SR1.PECERR`) bit position @def I2C_SR1_PECERR_Pos */
#define I2C_SR1_PECERR_Pos			((reg_bit_pos_t) 12U)
/** @brief Packet error checking mismatch event (`I2Cx_SR1.PECERR`) bit mask @def I2C_SR1_PECERR_Msk */
#define I2C_SR1_PECERR_Msk			REG_BIT_MASK(I2C_SR1_PECERR_Pos)
/** @brief Packet error checking mismatch event (`I2Cx_SR1.PECERR`) bit mask alias @def I2C_SR1_PECERR */
#define I2C_SR1_PECERR				I2C_SR1_PECERR_Msk

/** @brief Timeout or Tlow error event (`I2Cx_SR1.TIMEOUT`) bit position @def I2C_SR1_TIMEOUT_Pos */
#define I2C_SR1_TIMEOUT_Pos			((reg_bit_pos_t) 14U)
/** @brief Timeout or Tlow error event (`I2Cx_SR1.TIMEOUT`) bit mask @def I2C_SR1_TIMEOUT_Msk */
#define I2C_SR1_TIMEOUT_Msk			REG_BIT_MASK(I2C_SR1_TIMEOUT_Pos)
/** @brief Timeout or Tlow error event (`I2Cx_SR1.TIMEOUT`) bit mask alias @def I2C_SR1_TIMEOUT */
#define I2C_SR1_TIMEOUT				I2C_SR1_TIMEOUT_Msk

/** @brief SMBus alert event (`I2Cx_SR1.SMBALERT`) bit position @def I2C_SR1_SMBALERT_Pos */
#define I2C_SR1_SMBALERT_Pos		((reg_bit_pos_t) 15U)
/** @brief SMBus alert event (`I2Cx_SR1.SMBALERT`) bit mask @def I2C_SR1_SMBALERT_Msk */
#define I2C_SR1_SMBALERT_Msk		REG_BIT_MASK(I2C_SR1_SMBALERT_Pos)
/** @brief SMBus alert event (`I2Cx_SR1.SMBALERT`) bit mask alias @def I2C_SR1_SMBALERT */
#define I2C_SR1_SMBALERT			I2C_SR1_SMBALERT_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_SR2
// ---------------------------------------------------------------------------------------------------- //

/** @brief Master or slave mode state (`I2Cx_SR2.MSL`) bit position @def I2C_SR2_MSL_Pos */
#define I2C_SR2_MSL_Pos				((reg_bit_pos_t) 0U)
/** @brief Master or slave mode state (`I2Cx_SR2.MSL`) bit mask @def I2C_SR2_MSL_Msk */
#define I2C_SR2_MSL_Msk				REG_BIT_MASK(I2C_SR2_MSL_Pos)
/** @brief Master or slave mode state (`I2Cx_SR2.MSL`) bit mask alias @def I2C_SR2_MSL */
#define I2C_SR2_MSL					I2C_SR2_MSL_Msk

/** @brief Bus busy state (`I2Cx_SR2.BUSY`) bit position @def I2C_SR2_BUSY_Pos */
#define I2C_SR2_BUSY_Pos			((reg_bit_pos_t) 1U)
/** @brief Bus busy state (`I2Cx_SR2.BUSY`) bit mask @def I2C_SR2_BUSY_Msk */
#define I2C_SR2_BUSY_Msk			REG_BIT_MASK(I2C_SR2_BUSY_Pos)
/** @brief Bus busy state (`I2Cx_SR2.BUSY`) bit mask alias @def I2C_SR2_BUSY */
#define I2C_SR2_BUSY				I2C_SR2_BUSY_Msk

/** @brief Transmitter or receiver direction state (`I2Cx_SR2.TRA`) bit position @def I2C_SR2_TRA_Pos */
#define I2C_SR2_TRA_Pos				((reg_bit_pos_t) 2U)
/** @brief Transmitter or receiver direction state (`I2Cx_SR2.TRA`) bit mask @def I2C_SR2_TRA_Msk */
#define I2C_SR2_TRA_Msk				REG_BIT_MASK(I2C_SR2_TRA_Pos)
/** @brief Transmitter or receiver direction state (`I2Cx_SR2.TRA`) bit mask alias @def I2C_SR2_TRA */
#define I2C_SR2_TRA					I2C_SR2_TRA_Msk

/** @brief General-call address match state (`I2Cx_SR2.GENCALL`) bit position @def I2C_SR2_GENCALL_Pos */
#define I2C_SR2_GENCALL_Pos			((reg_bit_pos_t) 4U)
/** @brief General-call address match state (`I2Cx_SR2.GENCALL`) bit mask @def I2C_SR2_GENCALL_Msk */
#define I2C_SR2_GENCALL_Msk			REG_BIT_MASK(I2C_SR2_GENCALL_Pos)
/** @brief General-call address match state (`I2Cx_SR2.GENCALL`) bit mask alias @def I2C_SR2_GENCALL */
#define I2C_SR2_GENCALL				I2C_SR2_GENCALL_Msk

/** @brief SMBus device-default address match state (`I2Cx_SR2.SMBDEFAULT`) bit position @def I2C_SR2_SMBDEFAULT_Pos */
#define I2C_SR2_SMBDEFAULT_Pos		((reg_bit_pos_t) 5U)
/** @brief SMBus device-default address match state (`I2Cx_SR2.SMBDEFAULT`) bit mask @def I2C_SR2_SMBDEFAULT_Msk */
#define I2C_SR2_SMBDEFAULT_Msk		REG_BIT_MASK(I2C_SR2_SMBDEFAULT_Pos)
/** @brief SMBus device-default address match state (`I2Cx_SR2.SMBDEFAULT`) bit mask alias @def I2C_SR2_SMBDEFAULT */
#define I2C_SR2_SMBDEFAULT			I2C_SR2_SMBDEFAULT_Msk

/** @brief SMBus host-header match state (`I2Cx_SR2.SMBHOST`) bit position @def I2C_SR2_SMBHOST_Pos */
#define I2C_SR2_SMBHOST_Pos			((reg_bit_pos_t) 6U)
/** @brief SMBus host-header match state (`I2Cx_SR2.SMBHOST`) bit mask @def I2C_SR2_SMBHOST_Msk */
#define I2C_SR2_SMBHOST_Msk			REG_BIT_MASK(I2C_SR2_SMBHOST_Pos)
/** @brief SMBus host-header match state (`I2Cx_SR2.SMBHOST`) bit mask alias @def I2C_SR2_SMBHOST */
#define I2C_SR2_SMBHOST				I2C_SR2_SMBHOST_Msk

/** @brief Second own-address match state (`I2Cx_SR2.DUALF`) bit position @def I2C_SR2_DUALF_Pos */
#define I2C_SR2_DUALF_Pos			((reg_bit_pos_t) 7U)
/** @brief Second own-address match state (`I2Cx_SR2.DUALF`) bit mask @def I2C_SR2_DUALF_Msk */
#define I2C_SR2_DUALF_Msk			REG_BIT_MASK(I2C_SR2_DUALF_Pos)
/** @brief Second own-address match state (`I2Cx_SR2.DUALF`) bit mask alias @def I2C_SR2_DUALF */
#define I2C_SR2_DUALF				I2C_SR2_DUALF_Msk

/** @brief Packet error checking value (`I2Cx_SR2.PEC[7:0]`) field position @def I2C_SR2_PEC_Pos */
#define I2C_SR2_PEC_Pos				((reg_bit_pos_t) 8U)
/** @brief Packet error checking value (`I2Cx_SR2.PEC[7:0]`) field width @def I2C_SR2_PEC_Width */
#define I2C_SR2_PEC_Width			((reg_field_width_t) 8U)
/** @brief Packet error checking value (`I2Cx_SR2.PEC[7:0]`) field mask @def I2C_SR2_PEC_Msk */
#define I2C_SR2_PEC_Msk				REG_FIELD_MASK(I2C_SR2_PEC_Pos, I2C_SR2_PEC_Width)
/** @brief Packet error checking value (`I2Cx_SR2.PEC[7:0]`) field mask alias @def I2C_SR2_PEC */
#define I2C_SR2_PEC					I2C_SR2_PEC_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_CCR
// ---------------------------------------------------------------------------------------------------- //

/** @brief SCL clock-control divider (`I2Cx_CCR.CCR[11:0]`) field position @def I2C_CCR_CCR_Pos */
#define I2C_CCR_CCR_Pos				((reg_bit_pos_t) 0U)
/** @brief SCL clock-control divider (`I2Cx_CCR.CCR[11:0]`) field width @def I2C_CCR_CCR_Width */
#define I2C_CCR_CCR_Width			((reg_field_width_t) 12U)
/** @brief SCL clock-control divider (`I2Cx_CCR.CCR[11:0]`) field mask @def I2C_CCR_CCR_Msk */
#define I2C_CCR_CCR_Msk				REG_FIELD_MASK(I2C_CCR_CCR_Pos, I2C_CCR_CCR_Width)
/** @brief SCL clock-control divider (`I2Cx_CCR.CCR[11:0]`) field mask alias @def I2C_CCR_CCR */
#define I2C_CCR_CCR					I2C_CCR_CCR_Msk

/** @brief Fast-mode duty-cycle selection (`I2Cx_CCR.DUTY`) bit position @def I2C_CCR_DUTY_Pos */
#define I2C_CCR_DUTY_Pos			((reg_bit_pos_t) 14U)
/** @brief Fast-mode duty-cycle selection (`I2Cx_CCR.DUTY`) bit mask @def I2C_CCR_DUTY_Msk */
#define I2C_CCR_DUTY_Msk			REG_BIT_MASK(I2C_CCR_DUTY_Pos)
/** @brief Fast-mode duty-cycle selection (`I2Cx_CCR.DUTY`) bit mask alias @def I2C_CCR_DUTY */
#define I2C_CCR_DUTY				I2C_CCR_DUTY_Msk

/** @brief Master speed-mode selection (`I2Cx_CCR.FS`) bit position @def I2C_CCR_FS_Pos */
#define I2C_CCR_FS_Pos				((reg_bit_pos_t) 15U)
/** @brief Master speed-mode selection (`I2Cx_CCR.FS`) bit mask @def I2C_CCR_FS_Msk */
#define I2C_CCR_FS_Msk				REG_BIT_MASK(I2C_CCR_FS_Pos)
/** @brief Master speed-mode selection (`I2Cx_CCR.FS`) bit mask alias @def I2C_CCR_FS */
#define I2C_CCR_FS					I2C_CCR_FS_Msk

// ---------------------------------------------------------------------------------------------------- //
// I2Cx_TRISE
// ---------------------------------------------------------------------------------------------------- //

/** @brief Maximum SCL rise-time count (`I2Cx_TRISE.TRISE[5:0]`) field position @def I2C_TRISE_TRISE_Pos */
#define I2C_TRISE_TRISE_Pos			((reg_bit_pos_t) 0U)
/** @brief Maximum SCL rise-time count (`I2Cx_TRISE.TRISE[5:0]`) field width @def I2C_TRISE_TRISE_Width */
#define I2C_TRISE_TRISE_Width		((reg_field_width_t) 6U)
/** @brief Maximum SCL rise-time count (`I2Cx_TRISE.TRISE[5:0]`) field mask @def I2C_TRISE_TRISE_Msk */
#define I2C_TRISE_TRISE_Msk			REG_FIELD_MASK(I2C_TRISE_TRISE_Pos, I2C_TRISE_TRISE_Width)
/** @brief Maximum SCL rise-time count (`I2Cx_TRISE.TRISE[5:0]`) field mask alias @def I2C_TRISE_TRISE */
#define I2C_TRISE_TRISE				I2C_TRISE_TRISE_Msk

/** @} */ // I2C_01_RegisterLayer_00_RegisterDefines

// ==================================================================================================== //
// I2C Register Map
// ==================================================================================================== //

/**
 * @defgroup I2C_01_RegisterLayer_01_RegisterMap I2C Register Block Map
 * @ingroup I2C_01_RegisterLayer
 * @brief Memory-mapped I2C register block and practical register semantics
 * @details
 * @ref I2C_TypeDef is the raw peripheral view consumed by register-near code.
 * Full-register `.REG` members support register-image operations while
 * `.BIT` members expose named hardware fields for direct inspection.
 * @{
 */

/**
 * @brief I2C peripheral register block structure
 * @details Complete memory-mapped register map shared by I2C1 and I2C2.
 * @see RM0008 Section 26.6 I2C registers
 * @struct __I2C_TypeDef
 */
typedef volatile struct __I2C_TypeDef
{
	/**
	 * @section I2C_TypeDef_CR1 I2C Control Register 1 (CR1)
	 * @brief [R/W and action] Peripheral, protocol, receive, and software-reset control
	 * @details
	 * Persistent enable/policy bits share this register with hardware-consumed
	 * START, STOP, PEC, ALERT, and SWRST requests. Higher layers must respect
	 * each action's hardware-clear or sequencing behavior.
	 */
	union I2C_CR1
	{
		/** @brief Full 32-bit `I2Cx_CR1` register image */
		_IO REG;
		/** @brief Named `I2Cx_CR1` bitfield view */
		struct
		{
			_IO PE : 1;						/**< Peripheral enable */
			_IO SMBUS : 1;					/**< SMBus mode selection */
			_IO reserved_1 : 1;				/**< Reserved */
			_IO SMBTYPE : 1;				/**< SMBus device-type selection */
			_IO ENARP : 1;					/**< ARP protocol enable */
			_IO ENPEC : 1;					/**< Packet error checking enable */
			_IO ENGC : 1;					/**< General-call recognition enable */
			_IO NOSTRETCH : 1;				/**< Clock stretching disable in slave mode */
			_IO START : 1;					/**< START condition generation request */
			_IO STOP : 1;					/**< STOP condition generation request */
			_IO ACK : 1;					/**< Acknowledge generation enable */
			_IO POS : 1;					/**< Acknowledge or PEC position selection */
			_IO PEC : 1;					/**< Packet error checking transfer request */
			_IO ALERT : 1;					/**< SMBus alert request */
			_IO reserved_2 : 1;				/**< Reserved */
			_IO SWRST : 1;					/**< Software reset request */
			_IO reserved_3 : 16;			/**< Reserved */
		} BIT;
	} CR1;			/**< Control register 1 image and named-field views */

	/**
	 * @section I2C_TypeDef_CR2 I2C Control Register 2 (CR2)
	 * @brief [R/W] Input-clock, interrupt-source, and DMA request configuration
	 * @details
	 * `FREQ` must match the APB1 input clock in megahertz. Interrupt-source
	 * gates and DMA controls are independent from NVIC line configuration and
	 * DMA channel ownership.
	 */
	union I2C_CR2
	{
		/** @brief Full 32-bit `I2Cx_CR2` register image */
		_IO REG;
		/** @brief Named `I2Cx_CR2` bitfield view */
		struct
		{
			_IO FREQ : 6;					/**< Peripheral input-clock frequency in megahertz */
			_IO reserved_1 : 2;				/**< Reserved */
			_IO ITERREN : 1;				/**< Error interrupt enable */
			_IO ITEVTEN : 1;				/**< Event interrupt enable */
			_IO ITBUFEN : 1;				/**< Buffer interrupt enable */
			_IO DMAEN : 1;					/**< DMA request enable */
			_IO LAST : 1;					/**< DMA last-transfer selection */
			_IO reserved_2 : 19;			/**< Reserved */
		} BIT;
	} CR2;			/**< Control register 2 image and named-field views */

	/**
	 * @section I2C_TypeDef_OAR1 I2C Own Address Register 1 (OAR1)
	 * @brief [R/W] Primary 7-bit or 10-bit slave address
	 * @details
	 * `ADDMODE` selects the address width. Bit 14 is hardware-reserved but must
	 * be kept at `1` by configuration code as required by RM0008. Use
	 * @ref I2C_OAR1_BIT14_MUST_SET to preserve this bit when staging OAR1.
	 */
	union I2C_OAR1
	{
		/** @brief Full 32-bit `I2Cx_OAR1` register image */
		_IO REG;
		/** @brief Named `I2Cx_OAR1` bitfield view */
		struct
		{
			_IO ADD0 : 1;					/**< Own-address bit 0 */
			_IO ADD1_7 : 7;					/**< Own-address bits 7 through 1 */
			_IO ADD8_9 : 2;					/**< Own-address bits 9 through 8 */
			_IO reserved_1 : 4;				/**< Reserved */
			_IO reserved_bit14 : 1;			/**< Reserved bit 14: configuration must keep at one via @ref I2C_OAR1_BIT14_MUST_SET */
			_IO ADDMODE : 1;				/**< Own-address mode selection */
			_IO reserved_2 : 16;			/**< Reserved */
		} BIT;
	} OAR1;			/**< Primary own-address register image and named-field views */

	/**
	 * @section I2C_TypeDef_OAR2 I2C Own Address Register 2 (OAR2)
	 * @brief [R/W] Secondary 7-bit slave address and dual-address enable
	 */
	union I2C_OAR2
	{
		/** @brief Full 32-bit `I2Cx_OAR2` register image */
		_IO REG;
		/** @brief Named `I2Cx_OAR2` bitfield view */
		struct
		{
			_IO ENDUAL : 1;					/**< Dual-addressing enable */
			_IO ADD2 : 7;					/**< Second seven-bit own address */
			_IO reserved_1 : 24;			/**< Reserved */
		} BIT;
	} OAR2;			/**< Secondary own-address register image and named-field views */

	/**
	 * @section I2C_TypeDef_DR I2C Data Register (DR)
	 * @brief [R/W data port] Receive and transmit byte payload
	 * @details
	 * Reading consumes received data and advances receive-event state. Writing
	 * supplies the next transmit byte and advances transmit-event state. This is
	 * a data port, not stable configuration storage.
	 */
	union I2C_DR
	{
		/** @brief Full 32-bit `I2Cx_DR` data-port image */
		_IO REG;
		/** @brief Named `I2Cx_DR` bitfield view */
		struct
		{
			_IO DR : 8;						/**< Receive or transmit data payload */
			_IO reserved_1 : 24;			/**< Reserved */
		} BIT;
	} DR;			/**< Receive/transmit data-port image and named-field views */

	/**
	 * @section I2C_TypeDef_SR1 I2C Status Register 1 (SR1)
	 * @brief [R and software-clear] Protocol event and error state
	 * @details
	 * Event flags use hardware-defined clear sequences rather than uniform
	 * writes: for example, `ADDR` clears only after reading SR1 followed by SR2,
	 * while `STOPF` clears after reading SR1 followed by writing CR1. Error
	 * flags are cleared by writing `0` to the selected flag. Generic W1C or
	 * read-modify-write assumptions are therefore unsafe for this register.
	 */
	union I2C_SR1
	{
		/** @brief Full 32-bit `I2Cx_SR1` event/error image */
		_IO REG;
		/** @brief Named `I2Cx_SR1` bitfield view */
		struct
		{
			_I SB : 1;						/**< START condition generated event */
			_I ADDR : 1;					/**< Address sent or matched event */
			_I BTF : 1;						/**< Byte transfer finished event */
			_I ADD10 : 1;					/**< Ten-bit header sent event */
			_I STOPF : 1;					/**< STOP detection event */
			_I reserved_1 : 1;				/**< Reserved */
			_I RXNE : 1;					/**< Receive data register not empty state */
			_I TXE : 1;						/**< Transmit data register empty state */
			_IO BERR : 1;					/**< Bus error; write zero to clear */
			_IO ARLO : 1;					/**< Arbitration lost; write zero to clear */
			_IO AF : 1;						/**< Acknowledge failure; write zero to clear */
			_IO OVR : 1;					/**< Overrun or underrun; write zero to clear */
			_IO PECERR : 1;					/**< Packet error checking mismatch; write zero to clear */
			_I reserved_2 : 1;				/**< Reserved */
			_IO TIMEOUT : 1;				/**< Timeout or Tlow error; write zero to clear */
			_IO SMBALERT : 1;				/**< SMBus alert; write zero to clear */
			_I reserved_3 : 16;				/**< Reserved */
		} BIT;
	} SR1;			/**< Protocol event/error register image and named-field views */

	/**
	 * @section I2C_TypeDef_SR2 I2C Status Register 2 (SR2)
	 * @brief [R] Bus role, activity, direction, address-match source, and PEC state
	 * @details
	 * Reading SR2 after SR1 is part of the hardware sequence that clears
	 * `I2Cx_SR1.ADDR`; an isolated SR2 read can therefore have protocol-visible
	 * consequences when ADDR is set. All fields are hardware-populated and
	 * read-only from software.
	 */
	union I2C_SR2
	{
		/** @brief Full 32-bit `I2Cx_SR2` register image */
		_I REG;
		/** @brief Named `I2Cx_SR2` bitfield view */
		struct
		{
			_I MSL : 1;						/**< Master or slave mode state */
			_I BUSY : 1;						/**< Bus busy state */
			_I TRA : 1;						/**< Transmitter or receiver direction state */
			_I reserved_1 : 1;				/**< Reserved */
			_I GENCALL : 1;					/**< General-call address match state */
			_I SMBDEFAULT : 1;				/**< SMBus device-default address match state */
			_I SMBHOST : 1;					/**< SMBus host-header match state */
			_I DUALF : 1;					/**< Second own-address match state */
			_I PEC : 8;						/**< Packet error checking value */
			_I reserved_2 : 16;				/**< Reserved */
		} BIT;
	} SR2;			/**< Bus-state register image and named-field views */

	/**
	 * @section I2C_TypeDef_CCR I2C Clock Control Register (CCR)
	 * @brief [R/W] Master SCL divider, speed mode, and fast-mode duty cycle
	 * @details
	 * In standard mode, SCL period is derived from `2 x CCR`. In fast mode,
	 * DUTY selects the `2:1` or `16:9` low/high timing relationship.
	 */
	union I2C_CCR
	{
		/** @brief Full 32-bit `I2Cx_CCR` register image */
		_IO REG;
		/** @brief Named `I2Cx_CCR` bitfield view */
		struct
		{
			_IO CCR : 12;					/**< SCL clock-control divider */
			_IO reserved_1 : 2;				/**< Reserved */
			_IO DUTY : 1;					/**< Fast-mode duty-cycle selection */
			_IO FS : 1;						/**< Master speed-mode selection */
			_IO reserved_2 : 16;			/**< Reserved */
		} BIT;
	} CCR;			/**< SCL timing-control register image and named-field views */

	/**
	 * @section I2C_TypeDef_TRISE I2C Rise Time Register (TRISE)
	 * @brief [R/W] Maximum SCL rise-time count
	 * @details
	 * The programmed count is derived from the APB1 clock and the selected
	 * standard/fast timing mode. It must be configured while the peripheral is
	 * disabled.
	 */
	union I2C_TRISE
	{
		/** @brief Full 32-bit `I2Cx_TRISE` register image */
		_IO REG;
		/** @brief Named `I2Cx_TRISE` bitfield view */
		struct
		{
			_IO TRISE : 6;					/**< Maximum SCL rise-time count */
			_IO reserved_1 : 26;			/**< Reserved */
		} BIT;
	} TRISE;		/**< Maximum-rise-time register image and named-field views */
} I2C_TypeDef;

/** @} */ // I2C_01_RegisterLayer_01_RegisterMap

/** @} */ // I2C_01_RegisterLayer

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32F1XX_I2C_H_ */
