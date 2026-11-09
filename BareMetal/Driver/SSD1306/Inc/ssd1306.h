/**
 * @file ssd1306.h
 * @author Shrey Shah
 * @brief SSD1306 OLED Driver public API and display-state structure
 * @version v1.0
 * @date 09-11-2026
 * @details
 * @section SSD1306_H_HIERARCHY Hierarchy
 * - Position: SSD1306 Driver public-API layer over the scalar-type registry,
 *   project configuration constants, and the command register map
 * - Included by: Application modules that own an initialized I2C peripheral
 *   and drive the SSD1306 OLED display
 * - Uses: stm32f1xx.h I2C register vocabulary, ssd1306_defines.h project
 *   geometry/address constants, and the ssd1306_data_types.h scalar typedefs
 *   pulled transitively through ssd1306_defines.h
 *
 * @section SSD1306_H_RESPONSIBILITY Responsibility
 * This header declares the SSD1306 display-state structure and the public
 * Driver functions that initialize the controller, mutate the local
 * framebuffer, and transmit command and framebuffer streams to the display
 * through an already-initialized I2C peripheral. It owns the complete
 * display-state contract; voltage, GPIO, RCC, and I2C configuration remain
 * application-owned and are prerequisites, not responsibilities.
 *
 * @section SSD1306_H_DEPENDENCIES Dependencies
 * This header includes stm32f1xx.h for the I2C register-map type and
 * ssd1306_defines.h for the project geometry, slave address, and I2C timeout
 * constants; ssd1306_data_types.h arrives transitively. Canonical command
 * and value vocabulary remains in ssd1306_reg_map.h, which only the Driver
 * implementation includes directly.
 *
 * @section SSD1306_H_BOUNDARY Dependency Boundary
 * This header performs no transfer itself: every declared function mutates
 * only the caller-owned display state until @ref SSD1306_Refresh or @ref
 * SSD1306_SendCommand reaches the I2C Driver. No board pin, RCC gate, or
 * NVIC state is touched by this driver.
 */
// Header Guards
#ifndef SSD1306_H_
#define SSD1306_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx.h"
#include "ssd1306_defines.h"
#include "ssd1306_reg_map.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// ==================================================================================================== //
// Data Types
// ==================================================================================================== //

/**
 * @brief The configuration structure which holds the SSD1306 OLED display state and configuration
 * @details
 * This structure holds the state and configuration of the SSD1306 OLED
 * display. It contains the frame buffer, the I2C peripheral instance, and the
 * pixel coordinates. This structure will be input to all the peripheral
 * functions to perform the required operations on the SSD1306 OLED display.
 * @struct ssd1306_t
 */
typedef struct _ssd1306_t
{
	/**
	 * @brief Frame Buffer for SSD1306 OLED Display
	 * @details
	 * The SSD1306 GDDRAM is a linear array of bytes, each of which encodes
	 * eight vertical pixels in one display column. Each byte is addressed by
	 * its page and column coordinates, so the first index selects the page
	 * and the second index selects the column within that page:
	 * `frameBuffer[page][column]`.
	 * @memberof ssd1306_t
	 */
	ssd1306_data_t frameBuffer[SSD1306_PAGE_COUNT][SSD1306_COLUMN_COUNT];

	/**
	 * @brief Pointer to already initialized I2C Peripheral Instance
	 * @details
	 * The driver transmits every command and framebuffer stream through this
	 * instance using the public I2C Driver; it never configures the
	 * peripheral itself.
	 * @memberof ssd1306_t
	 * @note The I2C peripheral must be initialized and enabled before its
	 * pointer is passed to the SSD1306 driver.
	 */
	I2C_TypeDef* const pI2C;

	/**
	 * @brief The pixel coordinates of the SSD1306 OLED Display
	 * @details
	 * Scratch coordinates the application may use between framebuffer
	 * operations; the driver reads them only where a public function's
	 * contract states it does.
	 * @memberof ssd1306_t
	 */
	struct _ssd1306_pixel_t
	{
		/**
		 * @brief Pixel X-coordinate on the panel, expressed as a column address
		 * @details
		 * The horizontal axis maps one-to-one onto GDDRAM columns, so X uses
		 * the column scalar domain.
		 * @note Valid X-coordinates span @ref `SSD1306_COLUMN_MIN` to
		 * @ref `SSD1306_COLUMN_MAX` for this project geometry.
		 */
		ssd1306_column_t X;
		/**
		 * @brief Pixel Y-coordinate on the panel, expressed as a vertical pixel row
		 * @details
		 * The vertical axis is a GDDRAM row index, not a page address:
		 * `page = Y >> 3` and `row-within-page = Y & 0x07` derive from it.
		 * @note Valid Y-coordinates span 0 to `SSD1306_VERTICAL_PIXELS - 1`
		 * for this project geometry.
		 */
		ssd1306_row_t Y;
	} pixel;

} ssd1306_t;

// ==================================================================================================== //
// Public API
// ==================================================================================================== //

/**
 * @brief Transmits one single-byte SSD1306 command over the bound I2C peripheral
 * @details
 * Frames the opcode behind the command control byte and issues one bounded
 * controller transmit. The display state is otherwise untouched.
 *
 * @param[in] pSSD1306 Display state owning an initialized I2C peripheral
 * Expected values:
 * - Non-`NULL`: Command is transmitted through @p `pSSD1306`->`pI2C`
 * @param[in] command One canonical SSD1306 command or encoded command byte
 * Accepted values:
 * - Any `ssd1306_cmd_t` opcode or parameter-less encoded command from
 *   ssd1306_reg_map.h
 * @returns @ref driver_status_t "SSD1306 command-transmit operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Command byte was transmitted and the transaction closed with STOP
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pSSD1306` was `NULL`
 * @note The remaining failure statuses map one-to-one from
 * @ref I2C_MasterTransmit: timeout, no-ack, arbitration loss, overrun, line
 * fault, bus/state failures
 * @pre The bound I2C peripheral is configured, enabled, idle, and exclusively owned by the caller
 * @note Two-byte and three-byte commands (those with parameters) are not
 * expressible through this API; the Driver initializes such state internally
 */
driver_status_t SSD1306_SendCommand
(
	const ssd1306_t* const	pSSD1306,
	const ssd1306_cmd_t		command
);

/**
 * @brief Initializing the SSD1306 Controller configuration for the target display
 * @details
 * Transmits the complete power-on configuration command stream: display off
 * housing-keeping, clock/oscillator, multiplex ratio, display offset, start
 * line, charge pump enable, horizontal memory addressing, segment remap, COM
 * scan direction, COM pin layout, contrast, pre-charge, VCOMH deselect, RAM
 * content resume, normal display, and display on. Also clears the local
 * framebuffer so a later @ref SSD1306_Refresh reproduces exactly what the
 * application has drawn.
 *
 * @param[in,out] pSSD1306 Display state to bring up
 * Expected values:
 * - Non-`NULL` with a bound, initialized, enabled I2C peripheral instance
 * @returns @ref driver_status_t "SSD1306 initialization operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Controller was configured, and the local framebuffer was cleared
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pSSD1306` was `NULL`
 * @note The remaining failure statuses map one-to-one from
 * @ref I2C_MasterTransmit: timeout, no-ack, arbitration loss, overrun, line
 * fault, bus/state failures
 * @pre The application enabled the I2C APB1 clock gate, routed SDA/SCL, configured and enabled the I2C instance
 * @note The framebuffer rows in the local buffer are cleared, but the panel
 * still shows GDDRAM content until @ref SSD1306_Refresh transmits it
 */
driver_status_t SSD1306_Init(ssd1306_t* const pSSD1306);

/**
 * @brief Applies one byte-wide fill pattern to the complete local framebuffer
 * @details
 * Assigns the same byte value to every `frameBuffer[page][column]` element.
 * This is a framebuffer-only operation; nothing reaches the panel until @ref
 * SSD1306_Refresh transmits the framebuffer.
 *
 * @param[in,out] pSSD1306 Display state whose framebuffer is filled
 * Expected values:
 * - Non-`NULL`: Every framebuffer byte is written with @p `pattern`
 * @param[in] pattern Byte-wide fill value applied to the complete framebuffer
 * Accepted values:
 * - `0x00U`: All pixels off (matches the panel's clear state)
 * - `0xFFU`: All pixels on
 * - Any other value: Repeating vertical bit pattern per column byte
 * @returns @ref driver_status_t "SSD1306 framebuffer-fill operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Every framebuffer byte was assigned @p `pattern`
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pSSD1306` was `NULL`
 * @note Pure memory operation with no MMIO or I2C access
 */
driver_status_t SSD1306_FillFrameBuffer
(
	ssd1306_t* const		pSSD1306,
	const ssd1306_data_t	pattern
);

/**
 * @brief Writes one pixel of the requested color into the local framebuffer
 * @details
 * Locates the GDDRAM byte that stores the pixel —
 * `frameBuffer[Y >> 3][X]` — and overwrites only its `Y & 0x07` bit with the
 * requested color while preserving the other seven pixels the byte encodes:
 * white ors the bit in, black inverts-and-ands it out, so the same single
 * entry point serves both set-pixel and clear-pixel intent. Framebuffer-only
 * operation; call @ref SSD1306_Refresh to copy the framebuffer into GDDRAM.
 *
 * @param[in,out] pSSD1306 Display state whose framebuffer receives the pixel
 * Expected values:
 * - Non-`NULL`: The addressed framebuffer bit receives the requested color
 * @param[in] X Pixel X-coordinate on the panel as a column address
 * Accepted values:
 * - @ref `SSD1306_COLUMN_MIN` to @ref `SSD1306_COLUMN_MAX`: Valid column address
 * @param[in] Y Pixel Y-coordinate on the panel as a vertical pixel row
 * Accepted values:
 * - `0x00U` to `SSD1306_VERTICAL_PIXELS - 1`: Valid vertical pixel row
 * @param[in] color Requested display state of the addressed pixel
 * Accepted values:
 * - @ref `SSD1306_COLOR_BLACK`: The addressed framebuffer bit is cleared
 * - @ref `SSD1306_COLOR_WHITE`: The addressed framebuffer bit is set
 * @returns @ref driver_status_t "SSD1306 pixel-write operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: The addressed framebuffer bit was assigned @p `color`
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pSSD1306` was `NULL`
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `X` or @p `Y` exceeded the project geometry
 * @note Pure memory operation with no MMIO or I2C access
 * @note @p `color` reuses the existing @ref ssd1306_data_t byte vocabulary; it
 * holds exactly one pixel state, not an eight-pixel column byte
 */
driver_status_t SSD1306_WritePixel
(
	ssd1306_t* const		pSSD1306,
	const ssd1306_column_t	X,
	const ssd1306_row_t		Y,
	const ssd1306_data_t	color
);

/**
 * @brief Transmits the local framebuffer into the SSD1306 GDDRAM
 * @details
 * Selects the full-panel address window through @ref
 * `SSD1306_CMD_SET_COLUMN_ADDRESS` and @ref `SSD1306_CMD_SET_PAGE_ADDRESS`,
 * then streams the complete framebuffer row by row behind the data control
 * byte. Horizontal addressing was configured at @ref SSD1306_Init, so
 * sequential full-page writes walk the complete GDDRAM window.
 *
 * @param[in] pSSD1306 Display state whose framebuffer is transmitted
 * Expected values:
 * - Non-`NULL` with a bound, initialized, enabled I2C peripheral instance
 * @returns @ref driver_status_t "SSD1306 framebuffer-refresh operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Complete framebuffer reached GDDRAM
 * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pSSD1306` was `NULL`
 * @note The remaining failure statuses map one-to-one from
 * @ref I2C_MasterTransmit: timeout, no-ack, arbitration loss, overrun, line
 * fault, bus/state failures
 * @pre @ref SSD1306_Init configured the controller for horizontal addressing
 * @pre The bound I2C peripheral is idle and exclusively owned by the caller
 * @note One transaction per display page is issued; each is bounded by
 * @ref `SSD1306_I2C_TIMEOUT_MS` and a later page failure leaves earlier pages
 * already updated in GDDRAM
 */
driver_status_t SSD1306_Refresh(ssd1306_t* const pSSD1306);

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SSD1306_H_ */
