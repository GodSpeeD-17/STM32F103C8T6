/**
 * @file ssd1306.h
 * @author Shrey Shah
 * @brief SSD1306 OLED driver stack aggregate header
 * @version v1.0
 * @date 09-11-2026
 * @details
 * @section SSD1306_H_HIERARCHY Hierarchy
 * - Position: SSD1306 Driver stack aggregate (top-level convenience) header
 * - Included by: Application and higher layers that want the complete SSD1306
 *   vocabulary in one include
 * - Uses: ssd1306_data_types.h, ssd1306_defines.h, and ssd1306_reg_map.h
 *
 * @section SSD1306_H_RESPONSIBILITY Responsibility
 * This header aggregates the canonical SSD1306 headers so one include exposes
 * the scalar type registry, the project configuration constants, and the
 * command register map. It declares nothing of its own.
 *
 * @section SSD1306_H_DEPENDENCIES Dependencies
 * This header includes the three canonical SSD1306 headers only. Each of them
 * is individually self-contained, so consumers may include them directly to
 * keep their dependency surface minimal.
 *
 * @section SSD1306_H_BOUNDARY Dependency Boundary
 * This header adds no vocabulary and binds no bus instance, board pin, or
 * framebuffer; it is a pure include composition surface.
 */

// Header Guards
#ifndef SSD1306_H_
#define SSD1306_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx.h"
#include "ssd1306_defines.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief The configuration structure which holds the SSD1306 OLED display state and configuration
 * @details
 * This structure holds the state and configuration of the SSD1306 OLED display.
 * It contains the frame buffer, the I2C peripheral instance, and the pixel coordinates.
 * This structure will be input to all the peripheral functions to perform the required operations on the SSD1306 OLED display.
 * @struct ssd1306_t
 */
typedef struct _ssd1306_t
{
	/**
	 * @brief Frame Buffer for SSD1306 OLED Display
	 * @details
	 * The SSD1306 GDDRAM is a linear array of bytes, each of which encodes eight vertical pixels in one display column.
	 * Each byte is addressed by its page and column coordinates
	 * @memberof ssd1306_t
	 */
	ssd1306_data_t frameBuffer[SSD1306_PAGE_COUNT][SSD1306_COLUMN_COUNT];

	/**
	 * @brief Pointer to already initialized I2C Peripheral Instance 
	 * @memberof ssd1306_t
	 * @note - The I2C peripheral must be initialized before passing its pointer to the SSD1306 driver.
	 * @note - The driver will use this pointer to communicate with the SSD1306 OLED display over
	 */
	I2C_TypeDef* const pI2C;

	/**
	 * @brief The pixel coordinates of the SSD1306 OLED Display
	 * @memberof ssd1306_t
	 */
	struct _ssd1306_pixel_t
	{
		/**
		 * @brief Pixel X-coordinate in the SSD1306 OLED Display
		 * @memberof pixel
		 * @note Valid X-coordinates span `SSD1306_WIDTH_MIN` to `SSD1306_WIDTH_MAX` after project geometry is applied by ssd1306_config.h
		 */
		ssd1306_row_t X;
		/**
		 * @brief Pixel Y-coordinate in the SSD1306 OLED Display
		 * @memberof pixel
		 * @note Valid Y-coordinates span `SSD1306_HEIGHT_MIN` to `SSD1306_HEIGHT_MAX` after project geometry is applied by ssd1306_config.h
		 */
		ssd1306_column_t Y;
	} pixel;

} ssd1306_t;



// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SSD1306_H_ */
