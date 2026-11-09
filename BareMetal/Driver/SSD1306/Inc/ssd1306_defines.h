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
#define SSD1306_I2C_ADDRESS							((ssd1306_addr_t) 0x3CU)
// #define SSD1306_I2C_ADDRESS							((ssd1306_addr_t) 0x3DU) //!NOTE: This I2C Address depends upon SA0 pin connection

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
