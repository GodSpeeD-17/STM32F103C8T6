/**
 * @file ssd1306_reg_map.h
 * @author Shrey Shah
 * @brief SSD1306 OLED controller command register map and allowed values
 * @version v1.0
 * @date 09-11-2026
 * @details
 * @section SSD1306_REG_MAP_H_HIERARCHY Hierarchy
 * - Position: SSD1306 register-map and command-opcode definition layer
 * - Included by: Every SSD1306 module that composes SSD1306 command transactions
 * - Uses: ssd1306_data_types.h scalar type vocabulary plus the I2C and
 *   repository-wide fixed-width integer vocabulary it pulls through
 *   i2c_data_types.h
 *
 * @section SSD1306_REG_MAP_H_RESPONSIBILITY Responsibility
 * This header defines the canonical SSD1306 command/register opcode map, I2C
 * control-byte framing values, slave-address values, and the allowed parameter
 * values required to compose valid SSD1306 command transactions. Only the
 * canonical opcode, parameter-value, base-opcode, and encoder macros are
 * declared; compatibility aliases, redundant pre-encoded page constants, and
 * display-layer pixel-coordinate wrappers deliberately remain outside this
 * file.
 *
 * @section SSD1306_REG_MAP_H_DEPENDENCIES Dependencies
 * This header includes ssd1306_data_types.h, which supplies the SSD1306 scalar
 * domain typedefs used below and the I2C/fixed-width integer vocabulary
 * through i2c_data_types.h. Every byte of the command/data stream uses
 * `ssd1306_cmd_t`; address-domain parameters and values keep their own scalar
 * typedefs; and no raw `uint8_t` cast remains in this file.
 *
 * @section SSD1306_REG_MAP_H_BOUNDARY Dependency Boundary
 * This header defines command and value vocabulary only. It performs no I2C/SPI
 * transfer, framebuffer management, display initialization sequencing, timing
 * delay, or board-specific pin/bus binding.
 */
// Header Guards
#ifndef SSD1306_REG_MAP_H_
#define SSD1306_REG_MAP_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "ssd1306_data_types.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// ==================================================================================================== //
// I2C Control Byte Definitions
// ==================================================================================================== //

/**
 * @brief Control byte that marks following bytes as SSD1306 command bytes
 * @def SSD1306_CTRL_BYTE_COMMAND
 * @details
 * SSD1306 I2C transactions begin with a control byte. A control byte of
 * `0x00` selects Co=0 and D/C#=0, so the following bytes in the same
 * transaction are interpreted as command opcodes and command parameters.
 */
#define SSD1306_CTRL_BYTE_COMMAND								((ssd1306_cmd_t) 0x00U)

/**
 * @brief Control byte that marks following bytes as SSD1306 GDDRAM data bytes
 * @def SSD1306_CTRL_BYTE_DATA
 * @details
 * A control byte of `0x40` selects Co=0 and D/C#=1, so the following bytes in
 * the same transaction are written to Graphic Display Data RAM (GDDRAM) as
 * display data.
 */
#define SSD1306_CTRL_BYTE_DATA									((ssd1306_cmd_t) 0x40U)

// ==================================================================================================== //
// I2C Slave Address Definitions
// ==================================================================================================== //

/**
 * @brief SSD1306 7-bit I2C slave address when SA0/D/C# is driven low
 * @def SSD1306_I2C_ADDR_SA0_LOW
 * Accepted values:
 * - `0x3CU`: 7-bit address `0111100b`
 */
#define SSD1306_I2C_ADDR_SA0_LOW								((i2c_address_t) 0x3CU)

/**
 * @brief SSD1306 7-bit I2C slave address when SA0/D/C# is driven high
 * @def SSD1306_I2C_ADDR_SA0_HIGH
 * Accepted values:
 * - `0x3DU`: 7-bit address `0111101b`
 */
#define SSD1306_I2C_ADDR_SA0_HIGH								((i2c_address_t) 0x3DU)

// ==================================================================================================== //
// Fundamental Command Register Map
// ==================================================================================================== //

/**
 * @brief Sets the contrast control register data byte in a two-byte command transaction
 * @def SSD1306_CMD_SET_CONTRAST
 * @details
 * Send this opcode followed by one contrast data byte to select one of 256
 * display contrast steps.
 * Accepted command parameter values:
 * - `0x00U..0xFFU`: Contrast step, where larger values increase contrast
 * @note SSD1306 reset contrast value is `0x7FU`
 */
#define SSD1306_CMD_SET_CONTRAST								((ssd1306_cmd_t) 0x81U)

/**
 * @brief Requests display output to follow GDDRAM content
 * @def SSD1306_CMD_ENTIRE_DISPLAY_FOLLOW_RAM
 * @details
 * The display output resumes normal pixel sourcing from GDDRAM, so RAM bit
 * values determine whether pixels are on or off according to the normal/inverse
 * display setting.
 */
#define SSD1306_CMD_ENTIRE_DISPLAY_FOLLOW_RAM					((ssd1306_cmd_t) 0xA4U)

/**
 * @brief Forces all display pixels on regardless of GDDRAM content
 * @def SSD1306_CMD_ENTIRE_DISPLAY_ON
 * @details
 * The controller ignores GDDRAM pixel data and drives every display output on.
 * GDDRAM content is preserved and can be displayed again by issuing
 * @ref `SSD1306_CMD_ENTIRE_DISPLAY_FOLLOW_RAM`.
 */
#define SSD1306_CMD_ENTIRE_DISPLAY_ON							((ssd1306_cmd_t) 0xA5U)

/**
 * @brief Selects normal RAM-to-pixel mapping
 * @def SSD1306_CMD_SET_NORMAL_DISPLAY
 * @details
 * In normal display mode, a GDDRAM bit value of 1 turns the corresponding
 * pixel on and a value of 0 turns it off.
 */
#define SSD1306_CMD_SET_NORMAL_DISPLAY							((ssd1306_cmd_t) 0xA6U)

/**
 * @brief Selects inverse RAM-to-pixel mapping
 * @def SSD1306_CMD_SET_INVERSE_DISPLAY
 * @details
 * In inverse display mode, a GDDRAM bit value of 0 turns the corresponding
 * pixel on and a value of 1 turns it off.
 */
#define SSD1306_CMD_SET_INVERSE_DISPLAY							((ssd1306_cmd_t) 0xA7U)

/**
 * @brief Places the SSD1306 display into sleep mode
 * @def SSD1306_CMD_SET_DISPLAY_OFF
 * @details
 * Display output is disabled and the controller enters the display-off/sleep
 * state. Segment and common outputs follow the device off-state behavior.
 */
#define SSD1306_CMD_SET_DISPLAY_OFF								((ssd1306_cmd_t) 0xAEU)

/**
 * @brief Places the SSD1306 display into normal operating mode
 * @def SSD1306_CMD_SET_DISPLAY_ON
 * @details
 * Display output is enabled so the current GDDRAM mapping and panel
 * configuration are driven to the OLED panel.
 */
#define SSD1306_CMD_SET_DISPLAY_ON								((ssd1306_cmd_t) 0xAFU)

// ==================================================================================================== //
// Addressing Command Register Map
// ==================================================================================================== //

/**
 * @brief Sets the SSD1306 GDDRAM memory addressing mode in a two-byte command transaction
 * @def SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE
 * @details
 * Send this opcode followed by one mode byte selected from the
 * @ref `SSD1306_MEMORY_ADDRESSING_MODE_*` allowed values.
 */
#define SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE					((ssd1306_cmd_t) 0x20U)

/**
 * @brief Selects horizontal GDDRAM addressing mode
 * @def SSD1306_MEMORY_ADDRESSING_MODE_HORIZONTAL
 * Accepted values:
 * - `0x00U`: Column address increments through the configured column range and
 *   wraps to the next page in the configured page range
 */
#define SSD1306_MEMORY_ADDRESSING_MODE_HORIZONTAL				((ssd1306_cmd_t) 0x00U)

/**
 * @brief Selects vertical GDDRAM addressing mode
 * @def SSD1306_MEMORY_ADDRESSING_MODE_VERTICAL
 * Accepted values:
 * - `0x01U`: Page address increments through the configured page range and
 *   wraps to the next column in the configured column range
 */
#define SSD1306_MEMORY_ADDRESSING_MODE_VERTICAL					((ssd1306_cmd_t) 0x01U)

/**
 * @brief Selects page GDDRAM addressing mode
 * @def SSD1306_MEMORY_ADDRESSING_MODE_PAGE
 * Accepted values:
 * - `0x02U`: Column address increments within the current page; page address
 *   must be updated explicitly by page-addressing commands
 */
#define SSD1306_MEMORY_ADDRESSING_MODE_PAGE						((ssd1306_cmd_t) 0x02U)

/**
 * @brief Sets the column start and end addresses in horizontal or vertical addressing mode
 * @def SSD1306_CMD_SET_COLUMN_ADDRESS
 * @details
 * Send this opcode followed by a column start byte and a column end byte.
 * Accepted command parameter values:
 * - Start address: `0x00U..0x7FU`
 * - End address: `0x00U..0x7FU`
 * @note This command is valid only for horizontal or vertical addressing mode
 */
#define SSD1306_CMD_SET_COLUMN_ADDRESS							((ssd1306_cmd_t) 0x21U)

/**
 * @brief Sets the page start and end addresses in horizontal or vertical addressing mode
 * @def SSD1306_CMD_SET_PAGE_ADDRESS
 * @details
 * Send this opcode followed by a page start byte and a page end byte.
 * Accepted command parameter values:
 * - Start page: `0x00U..0x07U`
 * - End page: `0x00U..0x07U`
 * @note This command is valid only for horizontal or vertical addressing mode
 */
#define SSD1306_CMD_SET_PAGE_ADDRESS							((ssd1306_cmd_t) 0x22U)

/**
 * @brief Base opcode for setting the lower column start nibble in page addressing mode
 * @def SSD1306_CMD_SET_LOWER_COLUMN_START_BASE
 * @details
 * The command byte encodes the lower four bits of the column start address as
 * `0x00U | (columnStart & 0x0FU)`.
 * Accepted encoded command range:
 * - `0x00U..0x0FU`
 * @note This command is valid only for page addressing mode
 */
#define SSD1306_CMD_SET_LOWER_COLUMN_START_BASE					((ssd1306_cmd_t) 0x00U)

/**
 * @brief Base opcode for setting the higher column start nibble in page addressing mode
 * @def SSD1306_CMD_SET_HIGHER_COLUMN_START_BASE
 * @details
 * The command byte encodes the upper four bits of the column start address as
 * `0x10U | ((columnStart >> 4U) & 0x0FU)`.
 * Accepted encoded command range:
 * - `0x10U..0x1FU`
 * @note This command is valid only for page addressing mode
 */
#define SSD1306_CMD_SET_HIGHER_COLUMN_START_BASE				((ssd1306_cmd_t) 0x10U)

/**
 * @brief Base opcode for setting the GDDRAM page start address in page addressing mode
 * @def SSD1306_CMD_SET_PAGE_START_ADDRESS_BASE
 * @details
 * The command byte encodes the page start address as
 * `0xB0U | (page & 0x07U)`.
 * Accepted encoded command range:
 * - `0xB0U..0xB7U`
 * @note This command is valid only for page addressing mode
 */
#define SSD1306_CMD_SET_PAGE_START_ADDRESS_BASE					((ssd1306_cmd_t) 0xB0U)

/**
 * @brief Encodes the lower column start nibble command for page addressing mode
 * @def SSD1306_CMD_SET_LOWER_COLUMN_START
 * @param columnStart Column start address
 * Accepted values:
 * - `0x00U..0x7FU`: Column start address; only the lower four bits are encoded
 */
#define SSD1306_CMD_SET_LOWER_COLUMN_START(columnStart)			((ssd1306_cmd_t) (SSD1306_CMD_SET_LOWER_COLUMN_START_BASE | ((ssd1306_column_t) (columnStart) & 0x0FU)))

/**
 * @brief Encodes the higher column start nibble command for page addressing mode
 * @def SSD1306_CMD_SET_HIGHER_COLUMN_START
 * @param columnStart Column start address
 * Accepted values:
 * - `0x00U..0x7FU`: Column start address; only the upper nibble is encoded
 */
#define SSD1306_CMD_SET_HIGHER_COLUMN_START(columnStart)		((ssd1306_cmd_t) (SSD1306_CMD_SET_HIGHER_COLUMN_START_BASE | (((ssd1306_column_t) (columnStart) >> 4U) & 0x0FU)))

/**
 * @brief Encodes the page start address command for page addressing mode
 * @def SSD1306_CMD_SET_PAGE_START_ADDRESS
 * @param page Page start address
 * Accepted values:
 * - `0x00U..0x07U`: GDDRAM page start address
 */
#define SSD1306_CMD_SET_PAGE_START_ADDRESS(page)				((ssd1306_cmd_t) (SSD1306_CMD_SET_PAGE_START_ADDRESS_BASE | ((ssd1306_page_t) (page) & 0x07U)))

// ==================================================================================================== //
// Hardware Configuration Command Register Map
// ==================================================================================================== //

/**
 * @brief Base opcode for setting the display start line
 * @def SSD1306_CMD_SET_DISPLAY_START_LINE_BASE
 * @details
 * The command byte encodes the display start line as
 * `0x40U | (startLine & 0x3FU)`.
 * Accepted encoded command range:
 * - `0x40U..0x7FU`
 */
#define SSD1306_CMD_SET_DISPLAY_START_LINE_BASE					((ssd1306_cmd_t) 0x40U)

/**
 * @brief Encodes the display start line command
 * @def SSD1306_CMD_SET_DISPLAY_START_LINE
 * @param startLine GDDRAM display start line
 * Accepted values:
 * - `0x00U..0x3FU`: Display start line expressed as a vertical GDDRAM row index
 */
#define SSD1306_CMD_SET_DISPLAY_START_LINE(startLine)			((ssd1306_cmd_t) (SSD1306_CMD_SET_DISPLAY_START_LINE_BASE | ((ssd1306_row_t) (startLine) & 0x3FU)))

/**
 * @brief Selects segment remap with column address 0 mapped to SEG0
 * @def SSD1306_CMD_SET_SEGMENT_REMAP_COL0_SEG0
 */
#define SSD1306_CMD_SET_SEGMENT_REMAP_COL0_SEG0					((ssd1306_cmd_t) 0xA0U)

/**
 * @brief Selects segment remap with column address 127 mapped to SEG0
 * @def SSD1306_CMD_SET_SEGMENT_REMAP_COL127_SEG0
 */
#define SSD1306_CMD_SET_SEGMENT_REMAP_COL127_SEG0				((ssd1306_cmd_t) 0xA1U)

/**
 * @brief Sets the multiplex ratio in a two-byte command transaction
 * @def SSD1306_CMD_SET_MULTIPLEX_RATIO
 * @details
 * Send this opcode followed by one multiplex-ratio data byte where the
 * effective multiplex ratio is `N + 1`.
 * Accepted command parameter values:
 * - `0x0FU..0x3FU`: Multiplex ratios from 16 to 64
 * @note SSD1306 reset value is `0x3FU` (64 multiplex)
 */
#define SSD1306_CMD_SET_MULTIPLEX_RATIO							((ssd1306_cmd_t) 0xA8U)

/**
 * @brief Selects normal COM output scan direction from COM0 to COM[N-1]
 * @def SSD1306_CMD_SET_COM_SCAN_NORMAL
 */
#define SSD1306_CMD_SET_COM_SCAN_NORMAL							((ssd1306_cmd_t) 0xC0U)

/**
 * @brief Selects remapped COM output scan direction from COM[N-1] to COM0
 * @def SSD1306_CMD_SET_COM_SCAN_REMAPPED
 */
#define SSD1306_CMD_SET_COM_SCAN_REMAPPED						((ssd1306_cmd_t) 0xC8U)

/**
 * @brief Sets the display offset in a two-byte command transaction
 * @def SSD1306_CMD_SET_DISPLAY_OFFSET
 * @details
 * Send this opcode followed by one offset data byte to set the vertical COM
 * shift mapping.
 * Accepted command parameter values:
 * - `0x00U..0x3FU`: Vertical shift in COM rows
 * @note SSD1306 reset value is `0x00U`
 */
#define SSD1306_CMD_SET_DISPLAY_OFFSET							((ssd1306_cmd_t) 0xD3U)

/**
 * @brief Sets COM pin hardware configuration in a two-byte command transaction
 * @def SSD1306_CMD_SET_COM_PINS_HARDWARE_CONFIG
 * @details
 * Send this opcode followed by one configuration byte that encodes COM pin
 * layout and left/right remap.
 * Accepted command parameter values:
 * - `0x02U`: Sequential COM pin configuration, left/right remap disabled
 * - `0x12U`: Alternative COM pin configuration, left/right remap disabled
 * - `0x22U`: Sequential COM pin configuration, left/right remap enabled
 * - `0x32U`: Alternative COM pin configuration, left/right remap enabled
 * @note SSD1306 reset value is `0x12U`
 */
#define SSD1306_CMD_SET_COM_PINS_HARDWARE_CONFIG				((ssd1306_cmd_t) 0xDAU)

/**
 * @brief Selects sequential COM pin hardware configuration
 * @def SSD1306_COM_PINS_SEQUENTIAL
 */
#define SSD1306_COM_PINS_SEQUENTIAL								((ssd1306_cmd_t) 0x02U)

/**
 * @brief Selects alternative COM pin hardware configuration
 * @def SSD1306_COM_PINS_ALTERNATIVE
 */
#define SSD1306_COM_PINS_ALTERNATIVE							((ssd1306_cmd_t) 0x12U)

/**
 * @brief Enables COM left/right remap in the COM pin hardware configuration byte
 * @def SSD1306_COM_PINS_LEFT_RIGHT_REMAP_ENABLE
 */
#define SSD1306_COM_PINS_LEFT_RIGHT_REMAP_ENABLE				((ssd1306_cmd_t) 0x20U)

// ==================================================================================================== //
// Timing and Driving Scheme Command Register Map
// ==================================================================================================== //

/**
 * @brief Sets the display clock divide ratio and oscillator frequency in a two-byte command transaction
 * @def SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO
 * @details
 * Send this opcode followed by one data byte. Bits `[3:0]` set the divide
 * ratio as `DCLK divide ratio = A[3:0] + 1`; bits `[7:4]` set the oscillator
 * frequency.
 * Accepted command parameter values:
 * - `0x00U..0xFFU`: Encoded divide ratio and oscillator frequency
 * @note SSD1306 reset value is `0x80U`
 */
#define SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO				((ssd1306_cmd_t) 0xD5U)

/**
 * @brief Sets the pre-charge period in a two-byte command transaction
 * @def SSD1306_CMD_SET_PRECHARGE_PERIOD
 * @details
 * Send this opcode followed by one data byte. Bits `[3:0]` set phase 1 in
 * DCLK clocks and bits `[7:4]` set phase 2 in DCLK clocks.
 * Accepted command parameter values:
 * - `0x11U..0xFFU` by phase-field encoding: each phase uses `1..15` DCLKs;
 *   phase value `0` is invalid
 * @note SSD1306 reset value is `0x22U`
 */
#define SSD1306_CMD_SET_PRECHARGE_PERIOD						((ssd1306_cmd_t) 0xD9U)

/**
 * @brief Sets the VCOMH deselect level in a two-byte command transaction
 * @def SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL
 * @details
 * Send this opcode followed by one VCOMH deselect-level data byte.
 * Accepted command parameter values:
 * - @ref `SSD1306_VCOMH_DESELECT_0_65_VCC`
 * - @ref `SSD1306_VCOMH_DESELECT_0_77_VCC`
 * - @ref `SSD1306_VCOMH_DESELECT_0_83_VCC`
 */
#define SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL					((ssd1306_cmd_t) 0xDBU)

/**
 * @brief Selects approximately 0.65 times VCC as the VCOMH deselect level
 * @def SSD1306_VCOMH_DESELECT_0_65_VCC
 */
#define SSD1306_VCOMH_DESELECT_0_65_VCC							((ssd1306_cmd_t) 0x00U)

/**
 * @brief Selects approximately 0.77 times VCC as the VCOMH deselect level
 * @def SSD1306_VCOMH_DESELECT_0_77_VCC
 * @note SSD1306 reset value
 */
#define SSD1306_VCOMH_DESELECT_0_77_VCC							((ssd1306_cmd_t) 0x20U)

/**
 * @brief Selects approximately 0.83 times VCC as the VCOMH deselect level
 * @def SSD1306_VCOMH_DESELECT_0_83_VCC
 */
#define SSD1306_VCOMH_DESELECT_0_83_VCC							((ssd1306_cmd_t) 0x30U)

/**
 * @brief Performs no SSD1306 operation
 * @def SSD1306_CMD_NOP
 */
#define SSD1306_CMD_NOP											((ssd1306_cmd_t) 0xE3U)

// ==================================================================================================== //
// Charge Pump Command Register Map
// ==================================================================================================== //

/**
 * @brief Sets the charge pump regulator state in a two-byte command transaction
 * @def SSD1306_CMD_SET_CHARGE_PUMP
 * @details
 * Send this opcode followed by one charge-pump setting byte.
 * Accepted command parameter values:
 * - @ref `SSD1306_CHARGE_PUMP_DISABLE`
 * - @ref `SSD1306_CHARGE_PUMP_ENABLE`
 * @note The display should be off while the charge pump setting is changed
 */
#define SSD1306_CMD_SET_CHARGE_PUMP								((ssd1306_cmd_t) 0x8DU)

/**
 * @brief Disables the SSD1306 internal charge pump regulator
 * @def SSD1306_CHARGE_PUMP_DISABLE
 */
#define SSD1306_CHARGE_PUMP_DISABLE								((ssd1306_cmd_t) 0x10U)

/**
 * @brief Enables the SSD1306 internal charge pump regulator
 * @def SSD1306_CHARGE_PUMP_ENABLE
 */
#define SSD1306_CHARGE_PUMP_ENABLE								((ssd1306_cmd_t) 0x14U)

// ==================================================================================================== //
// Scrolling Command Register Map
// ==================================================================================================== //

/**
 * @brief Configures continuous right horizontal scrolling
 * @def SSD1306_CMD_HORIZONTAL_SCROLL_RIGHT
 * @details
 * This opcode starts a horizontal-scroll setup sequence as defined by the
 * SSD1306 scrolling command table. The setup sequence requires the dummy and
 * parameter bytes specified by the datasheet before @ref
 * `SSD1306_CMD_ACTIVATE_SCROLL` is issued.
 */
#define SSD1306_CMD_HORIZONTAL_SCROLL_RIGHT						((ssd1306_cmd_t) 0x26U)

/**
 * @brief Configures continuous left horizontal scrolling
 * @def SSD1306_CMD_HORIZONTAL_SCROLL_LEFT
 * @details
 * This opcode starts a horizontal-scroll setup sequence as defined by the
 * SSD1306 scrolling command table.
 */
#define SSD1306_CMD_HORIZONTAL_SCROLL_LEFT						((ssd1306_cmd_t) 0x27U)

/**
 * @brief Configures continuous vertical and right horizontal scrolling
 * @def SSD1306_CMD_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL
 */
#define SSD1306_CMD_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL		((ssd1306_cmd_t) 0x29U)

/**
 * @brief Configures continuous vertical and left horizontal scrolling
 * @def SSD1306_CMD_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL
 */
#define SSD1306_CMD_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL			((ssd1306_cmd_t) 0x2AU)

/**
 * @brief Deactivates the currently configured scrolling action
 * @def SSD1306_CMD_DEACTIVATE_SCROLL
 * @note After deactivation, GDDRAM data should be rewritten before activating
 * a new scroll configuration that depends on RAM contents
 */
#define SSD1306_CMD_DEACTIVATE_SCROLL							((ssd1306_cmd_t) 0x2EU)

/**
 * @brief Activates the scrolling action configured by the last scroll setup sequence
 * @def SSD1306_CMD_ACTIVATE_SCROLL
 */
#define SSD1306_CMD_ACTIVATE_SCROLL								((ssd1306_cmd_t) 0x2FU)

/**
 * @brief Sets the vertical scroll area in a three-byte command transaction
 * @def SSD1306_CMD_SET_VERTICAL_SCROLL_AREA
 * @details
 * Send this opcode followed by the top fixed-area row count and scroll-area
 * row count bytes as defined by the SSD1306 scrolling command table.
 */
#define SSD1306_CMD_SET_VERTICAL_SCROLL_AREA					((ssd1306_cmd_t) 0xA3U)

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SSD1306_REG_MAP_H_ */
