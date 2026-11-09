/**
 * @file ssd1306_defines.h
 * @author Shrey Shah
 * @brief SSD1306 project configuration constants: geometry and selected transport address
 * @version v1.0
 * @date 09-11-2026
 * @details
 * @section SSD1306_DEFINES_H_HIERARCHY Hierarchy
 * - Position: SSD1306 project configuration-constant definition layer
 * - Included by: SSD1306 driver implementation modules that need the project
 *   geometry or the selected slave address
 * - Uses: ssd1306_data_types.h scalar type vocabulary plus the I2C scalar
 *   vocabulary it pulls through i2c_data_types.h
 *
 * @section SSD1306_DEFINES_H_RESPONSIBILITY Responsibility
 * This header selects the fixed SSD1306 panel geometry and the concrete I2C
 * slave address used by this project. It names physical dimensions, the
 * derived panel memory-domain limits (pages and columns), and one selected
 * transport address; it declares no command vocabulary, opcode, encoder, or
 * behaviour of its own.
 *
 * @section SSD1306_DEFINES_H_DEPENDENCIES Dependencies
 * This header includes ssd1306_data_types.h, which supplies the SSD1306
 * scalar typedefs and the I2C address typedef through i2c_data_types.h.
 * The canonical @ref `SSD1306_I2C_ADDR_SA0_LOW` / @ref
 * `SSD1306_I2C_ADDR_SA0_HIGH` address vocabulary lives in ssd1306_reg_map.h;
 * this header selects between them without depending on that file directly.
 *
 * @section SSD1306_DEFINES_H_BOUNDARY Dependency Boundary
 * This header defines project selection constants only. It performs no I2C
 * transfer, framebuffer management, display initialization sequencing, or
 * board-specific pin/bus binding, and it introduces no new scalar type.
 */
// Header Guards
#ifndef SSD1306_DEFINES_H_
#define SSD1306_DEFINES_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "ssd1306_data_types.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Selected SSD1306 7-bit I2C slave address for this project
 * @def SSD1306_I2C_ADDRESS
 * @details
 * The concrete transport address wired to this project's display. It selects
 * one of the SA0-dependent address options named by ssd1306_reg_map.h with
 * SA0/D/C# driven low.
 * Accepted values:
 * - One SA0-dependent option: @ref `SSD1306_I2C_ADDR_SA0_LOW` (0x3C) or
 *   @ref `SSD1306_I2C_ADDR_SA0_HIGH` (0x3D)
 * @note Changing the wired SA0 level requires selecting the matching
 * option here; the constant does not auto-detect the module state.
 */
#define SSD1306_I2C_ADDRESS							((i2c_address_t) 0x3CU)

/**
 * @brief Transaction-wide I2C poll budget for one SSD1306 I2C transaction
 * @def SSD1306_I2C_TIMEOUT_MS
 * @details
 * Millisecond timeout handed to every @ref I2C_MasterTransmit call issued by
 * the SSD1306 driver. One full-page data transaction moves
 * `SSD1306_COLUMN_COUNT + 1` bytes, so the value must cover one such message
 * plus address/START/STOP overhead at the configured I2C bus speed; the
 * default covers a 100 kHz bus with margin.
 * @note Raising the I2C bus speed does not require changing this constant;
 * slowing the bus or shrinking it below the full-page time may cause
 * spurious @ref `DRIVER_STATUS_ERROR_TIMEOUT` results.
 */
#define SSD1306_I2C_TIMEOUT_MS						((uint32_t) 100U)

// ==================================================================================================== //
// Selected Configuration Command Parameters
// ==================================================================================================== //

/**
 * @brief Display clock divide ratio and oscillator frequency selected for this project
 * @def SSD1306_CONFIG_DISPLAY_CLOCK
 * @details
 * The parameter byte handed to the two-byte
 * @ref `SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO` command by
 * @ref SSD1306_Init. Encoded value `0xF0` selects divide ratio 1
 * (`[3:0] = 0` so ratio = 0 + 1) with oscillator at its maximum frequency
 * (`[7:4] = 15`). The fastest frame refresh keeps duty cycle per COM row
 * short but the panel emission rate high, which this project prioritizes
 * for maximum brightness.
 * Accepted values:
 * - One encoded byte: bits `[3:0]` divide ratio, bits `[7:4]` oscillator
 *   frequency, as documented by the opcode
 */
#define SSD1306_CONFIG_DISPLAY_CLOCK				((ssd1306_cmd_t) 0xF0U)

/**
 * @brief Multiplex ratio selected for this project
 * @def SSD1306_CONFIG_MULTIPLEX_RATIO
 * @details
 * The parameter byte handed to the two-byte
 * @ref `SSD1306_CMD_SET_MULTIPLEX_RATIO` command by @ref SSD1306_Init.
 * Encoded value `0x3F` selects an effective multiplex ratio of 64
 * (`N + 1 = 0x3F + 1`), matching the full 64-row panel height.
 * Accepted values:
 * - `0x0FU..0x3FU`: Multiplex ratio encoding, effective ratio is `N + 1`;
 *   the maximum matches @ref `SSD1306_VERTICAL_PIXELS` for this geometry
 */
#define SSD1306_CONFIG_MULTIPLEX_RATIO				((ssd1306_cmd_t) 0x3FU)

/**
 * @brief Display offset selected for this project
 * @def SSD1306_CONFIG_DISPLAY_OFFSET
 * @details
 * The parameter byte handed to the two-byte
 * @ref `SSD1306_CMD_SET_DISPLAY_OFFSET` command by @ref SSD1306_Init.
 * Encoded value `0x00` selects no vertical COM shift, so panel row Y renders
 * at COM row Y for this geometry.
 * Accepted values:
 * - `0x00U..0x3FU`: Vertical shift in COM rows; zero keeps the additive
 *   panel-to-COM mapping exact
 */
#define SSD1306_CONFIG_DISPLAY_OFFSET				((ssd1306_cmd_t) 0x00U)

/**
 * @brief Display start line selected for this project
 * @def SSD1306_CONFIG_DISPLAY_START_LINE
 * @details
 * The row argument handed to the @ref `SSD1306_CMD_SET_DISPLAY_START_LINE`
 * encoder by @ref SSD1306_Init. Row `0` composes the reset encoding
 * `0x40`, so panel row Y maps to display line Y with no vertical pan.
 * Accepted values:
 * - `0x00U..0x3FU`: Display start line as a vertical GDDRAM row index;
 *   zero keeps the additive panel-to-line mapping exact
 */
#define SSD1306_CONFIG_DISPLAY_START_LINE			((ssd1306_row_t) 0U)

/**
 * @brief Contrast step selected for this project
 * @def SSD1306_CONFIG_CONTRAST
 * @details
 * The parameter byte handed to the two-byte
 * @ref `SSD1306_CMD_SET_CONTRAST` command by @ref SSD1306_Init.
 * Encoded value `0xFF` selects the maximum contrast step (segment output
 * current maximized) so the panel renders at full driver brightness.
 * Accepted values:
 * - `0x00U..0xFFU`: Contrast step; larger values increase contrast
 */
#define SSD1306_CONFIG_CONTRAST						((ssd1306_cmd_t) 0xFFU)

/**
 * @brief Pre-charge period selected for this project
 * @def SSD1306_CONFIG_PRECHARGE_PERIOD
 * @details
 * The parameter byte handed to the two-byte
 * @ref `SSD1306_CMD_SET_PRECHARGE_PERIOD` command by @ref SSD1306_Init.
 * Encoded value `0xF1` selects phase 1 = 2 DCLKs (`[3:0] = 1`, period = 2)
 * and phase 2 = 16 DCLKs (`[7:4] = 15`, period = 16), the pairing the module
 * datasheet recommends with the internal charge pump enabled.
 * Accepted values:
 * - `0x11U..0xFFU` by phase-field encoding: each phase uses `1..15` DCLKs;
 *   a phase field of `0` is invalid
 */
#define SSD1306_CONFIG_PRECHARGE_PERIOD				((ssd1306_cmd_t) 0xFFU)

/**
 * @brief VCOMH deselect level selected for this project
 * @def SSD1306_CONFIG_VCOMH_DESELECT
 * @details
 * The parameter byte handed to the two-byte
 * @ref `SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL` command by @ref SSD1306_Init.
 * The VCOMH deselect level is a contrast-control input alongside the
 * contrast step: it sets the common-row voltage while a row is deselected.
 * A higher level also raises the voltage across every unlit pixel, so the
 * off-state background glows faintly and blacks wash out toward gray; a
 * lower level keeps unlit pixels dark but also reduces the common-row
 * drive headroom that lit pixels render with. Encoded value `0x20` selects
 * approximately 0.77 times VCC, the controller reset default: the balance
 * point that keeps the background dark while preserving lit-pixel
 * brightness.
 * Accepted values:
 * - `0x00U`: Approximately 0.65 times VCC (darkest background, dimmest lit pixels)
 * - `0x20U`: Approximately 0.77 times VCC (controller reset default; balance point)
 * - `0x30U`: Approximately 0.83 times VCC (brightest background, washed-out blacks)
 */
#define SSD1306_CONFIG_VCOMH_DESELECT				((ssd1306_cmd_t) 0x5FU)

// ==================================================================================================== //
// Pixel Color Definitions
// ==================================================================================================== //

/**
 * @brief Byte-wide framebuffer pattern that renders every encoded pixel dark
 * @def SSD1306_FILL_PATTERN_BLACK
 * @details
 * One GDDRAM byte stores eight vertical pixels with bit value 1 lighting the
 * pixel, so the all-zero byte turns the complete byte-column dark. Usable as
 * an @ref SSD1306_FillFrameBuffer pattern and as the driver's cleared/fill
 * baseline value. Distinct from the single-pixel @ref `SSD1306_COLOR_BLACK`.
 * Accepted values:
 * - `0x00U`: All eight vertically stacked pixels of the column dark
 */
#define SSD1306_FILL_PATTERN_BLACK					((ssd1306_data_t) 0x00U)

/**
 * @brief Byte-wide framebuffer pattern that renders every encoded pixel lit
 * @def SSD1306_FILL_PATTERN_WHITE
 * @details
 * One GDDRAM byte stores eight vertical pixels with bit value 1 lighting the
 * pixel, so the all-ones byte lights the complete byte-column lit. Usable as
 * an @ref SSD1306_FillFrameBuffer pattern for a fully lit panel. Distinct
 * from the single-pixel @ref `SSD1306_COLOR_WHITE`.
 * Accepted values:
 * - `0xFFU`: All eight vertically stacked pixels of the column lit
 */
#define SSD1306_FILL_PATTERN_WHITE					((ssd1306_data_t) 0xFFU)

/**
 * @brief The single-pixel display state that renders one addressed pixel dark
 * @def SSD1306_COLOR_BLACK
 * @details
 * Per-pixel color vocabulary consumed by @ref SSD1306_WritePixel.
 * One GDDRAM bit stores one pixel state with bit value 1 lighting the pixel,
 * so the value `0` maps onto a cleared framebuffer bit. Distinct from the
 * byte-wide @ref `SSD1306_FILL_PATTERN_BLACK`, which holds eight such states
 * in one framebuffer column byte.
 * Accepted values:
 * - `0x00U`: The vertical pixel dark
 */
#define SSD1306_COLOR_BLACK							((ssd1306_data_t) 0x00U)

/**
 * @brief The single-pixel display state that renders one addressed pixel lit
 * @def SSD1306_COLOR_WHITE
 * @details
 * Per-pixel color vocabulary consumed by @ref SSD1306_WritePixel.
 * One GDDRAM bit stores one pixel state with bit value 1 lighting the pixel,
 * so the value `1` maps onto a set framebuffer bit. Distinct from the
 * byte-wide @ref `SSD1306_FILL_PATTERN_WHITE`, which holds eight such states
 * in one framebuffer column byte.
 * Accepted values:
 * - `0x01U`: The vertical pixel lit
 */
#define SSD1306_COLOR_WHITE							((ssd1306_data_t) 0x01U)


/**
 * @brief SSD1306 panel width in horizontal pixels (X axis)
 * @def SSD1306_HORIZONTAL_PIXELS
 * @details
 * Columns 0 through `SSD1306_HORIZONTAL_PIXELS - 1` form the full display
 * width that geometry validation and framebuffer addressing use. The physical
 * value 128 equals the SSD1306 GDDRAM column field width, so-derived helpers
 * may rely on `COL = X & 0x7F` for this geometry.
 */
#define SSD1306_HORIZONTAL_PIXELS					((ssd1306_column_t) 128U)

/**
 * @brief SSD1306 panel height in vertical pixels (Y axis)
 * @def SSD1306_VERTICAL_PIXELS
 * @details
 * Rows 0 through `SSD1306_VERTICAL_PIXELS - 1` form the full display height.
 * The physical value 64 spans eight display pages of eight pixel rows, so
 * page-mode addressing may derive `page = Y >> 3` and `row = Y & 0x07` for
 * this geometry.
 */
#define SSD1306_VERTICAL_PIXELS						((ssd1306_row_t) 64U)

// ==================================================================================================== //
// Display Memory Geometry Limits
// ==================================================================================================== //

/**
 * @brief Lowest valid SSD1306 GDDRAM page address for this geometry
 * @def SSD1306_PAGE_MIN
 * @details
 * Physically the first of the eight GDDRAM pages this panel spans; it is the
 * inclusive lower bound that page-address validation and page iteration accept.
 * It derives from `SSD1306_VERTICAL_PIXELS >> 3` and fixes @ref
 * `ssd1306_page_t`'s documented value range.
 */
#define SSD1306_PAGE_MIN							((ssd1306_page_t) 0U)

/**
 * @brief Highest valid SSD1306 GDDRAM page address for this geometry
 * @def SSD1306_PAGE_MAX
 * @details
 * Physically the last of the eight GDDRAM pages this panel spans; it is the
 * inclusive upper bound that page-address validation and page iteration accept.
 * It equals `SSD1306_PAGE_COUNT - 1` and fixes @ref `ssd1306_page_t`'s
 * documented value range.
 */
#define SSD1306_PAGE_MAX							((ssd1306_page_t) 7U)

/**
 * @brief Number of SSD1306 GDDRAM pages this panel geometry spans
 * @def SSD1306_PAGE_COUNT
 * @details
 * The total page-domain size in pages: it derives from
 * `SSD1306_VERTICAL_PIXELS >> 3` and equals
 * `SSD1306_PAGE_MAX - SSD1306_PAGE_MIN + 1`. Iterators may compare against
 * this count while index arithmetic checks against @ref `SSD1306_PAGE_MAX`.
 */
#define SSD1306_PAGE_COUNT							((ssd1306_page_t) 8U)

/**
 * @brief Lowest valid SSD1306 column address for this geometry
 * @def SSD1306_COLUMN_MIN
 * @details
 * Physically the first of the 128 GDDRAM columns this panel exposes; it is
 * the inclusive lower bound that column-address validation and X-axis
 * iteration accept. It fixes @ref `ssd1306_column_t`'s documented value range.
 */
#define SSD1306_COLUMN_MIN							((ssd1306_column_t) 0U)

/**
 * @brief Highest valid SSD1306 column address for this geometry
 * @def SSD1306_COLUMN_MAX
 * @details
 * Physically the last of the 128 GDDRAM columns this panel exposes; it is the
 * inclusive upper bound that column-address validation and X-axis iteration
 * accept. It equals `SSD1306_COLUMN_COUNT - 1` and
 * `SSD1306_HORIZONTAL_PIXELS - 1`, so `COL = X & 0x7F` mod-mapping stays exact
 * for this geometry.
 */
#define SSD1306_COLUMN_MAX							((ssd1306_column_t) 127U)

/**
 * @brief Number of SSD1306 columns this panel geometry spans
 * @def SSD1306_COLUMN_COUNT
 * @details
 * The total column-domain size in columns: it equals
 * `SSD1306_COLUMN_MAX - SSD1306_COLUMN_MIN + 1` and matches
 * `SSD1306_HORIZONTAL_PIXELS`. Iterators may compare against
 * this count, keeping @ref `SSD1306_COLUMN_MAX` reserved for index arithmetic.
 */
#define SSD1306_COLUMN_COUNT						((ssd1306_column_t) 128U)

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SSD1306_DEFINES_H_ */
