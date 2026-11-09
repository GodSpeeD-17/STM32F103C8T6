/**
 * @file ssd1306_data_types.h
 * @author Shrey Shah
 * @brief Scalar data-type registry for the SSD1306 OLED display stack
 * @version v1.0
 * @date 09-11-2026
 * @details
 * @section SSD1306_DATA_TYPES_H_HIERARCHY Hierarchy
 * - Position: SSD1306 Driver scalar data-type definition layer
 * - Included by: ssd1306_defines.h, ssd1306_reg_map.h, and every SSD1306
 *   module that needs an SSD1306 scalar value type
 * - Uses: i2c_data_types.h scalar vocabulary, re-exports the repository-wide
 *   fixed-width integer types through it
 *
 * @section SSD1306_DATA_TYPES_H_RESPONSIBILITY Responsibility
 * This header is the canonical registry of SSD1306 scalar data types. Each
 * typedef names one physical scalar domain consumed by the SSD1306 stack:
 * the command-stream byte, transport addressing, page addressing, column
 * addressing, pixel-row addressing, and the fill-pattern byte. It declares
 * types only. Configuration constants, command opcodes, and geometry values
 * belong to ssd1306_defines.h and ssd1306_reg_map.h and deliberately remain
 * outside this file.
 *
 * @section SSD1306_DATA_TYPES_H_DEPENDENCIES Dependencies
 * This header includes only i2c_data_types.h, which supplies the I2C scalar
 * vocabulary the transport-facing typedefs are built from and the
 * repository-wide fixed-width integer vocabulary every typedef below is
 * built from. It includes no MCU, board, bus-instance, framebuffer, or
 * display-service header. A scalar typedef is introduced here only when a
 * new scalar domain actually earns one; this file is not a dumping ground
 * for aliases of unrelated quantities.
 *
 * @section SSD1306_DATA_TYPES_H_BOUNDARY Dependency Boundary
 * Declaring portable scalar aliases over the repository's fixed-width integer
 * types is the complete dependency of this header. This header performs no bus
 * access, initialization behaviour, or allocation.
 */

// Header Guards
#ifndef SSD1306_DATA_TYPES_H_
#define SSD1306_DATA_TYPES_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "i2c_data_types.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// ==================================================================================================== //
// Data Types
// ==================================================================================================== //
/**
 * @brief One byte of the SSD1306 command/data stream in a framed transaction
 * @details
 * SSD1306 I2C transactions carry a stream of single-byte payloads: the
 * control byte, command opcodes, encoded command values, command parameters,
 * and GDDRAM data bytes. Every byte emitted into that stream uses this type,
 * while address-domain values entering the stream keep their own scalar
 * typedefs.
 * @typedef ssd1306_cmd_t
 */
typedef uint8_t									ssd1306_cmd_t;

/**
 * @brief Page address of one SSD1306 display memory row
 * @note Valid page addresses span `SSD1306_PAGE_MIN` to `SSD1306_PAGE_MAX`
 * after project geometry is applied by ssd1306_config.h
 * @typedef ssd1306_page_t
 */
typedef uint8_t									ssd1306_page_t;

/**
 * @brief Column address of one SSD1306 display memory column
 * @note Valid column addresses span `SSD1306_COLUMN_MIN` to
 * `SSD1306_COLUMN_MAX` after project geometry is applied by ssd1306_config.h
 * @typedef ssd1306_column_t
 */
typedef uint8_t									ssd1306_column_t;

/**
 * @brief Vertical pixel-row address within one display page
 * @note Valid row addresses span `SSD1306_HEIGHT_MIN` to `SSD1306_HEIGHT_MAX`
 * after project geometry is applied by ssd1306_config.h. Frame-scale GDDRAM
 * line indexes, such as the display start line, reuse this type.
 * @typedef ssd1306_row_t
 */
typedef uint8_t									ssd1306_row_t;

/**
 * @brief One byte of the SSD1306 GDDRAM framebuffer
 * @details
 * The SSD1306 framebuffer is a linear array of bytes, each of which encodes
 * eight vertical pixels in one display column. Each byte is addressed by its
 * page and column coordinates, so this type is distinct from the page and
 * column typedefs.
 * @typedef ssd1306_data_t
 */
typedef i2c_data_t								ssd1306_data_t;


// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SSD1306_DATA_TYPES_H_ */
