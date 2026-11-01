/**
 * @file ssd1306_config.h
 * @author Shrey Shah
 * @brief Configuration Structure & Functions for SSD1306 OLED Display
 * @version 1.0
 * @date 03-08-2025
 */

// Header Guards
#ifndef __SSD1306_CONFIG_H__
#define __SSD1306_CONFIG_H__

// ------------------------------------------------------------- INCLUDES START ------------------------------------------------------------- //
#include "ssd1306_reg_map.h"
// ------------------------------------------------------------- INCLUDES END ------------------------------------------------------------- //

// ------------------------------------------------------------- MACROS START ------------------------------------------------------------- //
// Size of SSD1306 Goto XY Buffer Size
#define SSD1306_BUFFER_GOTO_XY_SIZE					(3)

/**
 * @brief Initializes the buffer with X, Y command sequence
 * @param X X-coordinate: 0 - `SSD1306_WIDTH_MAX`
 * @param Y Y-coordinate: 0 - `SSD1306_HEIGHT_MAX`
 */
#define SSD1306_BUFFER_GOTO_XY(X, Y){				 \
	/* Send X Lower Nibble Command */				 \
	SSD1306_CMD_PAGE_MODE_SET_COL_LOWER_NIBBLE((X)), \
	/* Send X Upper Nibble Command */				 \
	SSD1306_CMD_PAGE_MODE_SET_COL_UPPER_NIBBLE((X)), \
	/* Send Y Command */							 \
	SSD1306_CMD_PAGE_MODE_SET_PAGE(((Y) >> 3)) 		 \
}
// ------------------------------------------------------------- MACROS END ------------------------------------------------------------- //

// ------------------------------------------------------------- STRUCT START ------------------------------------------------------------- //
// SSD1306 Display Pixel Structure
typedef struct {
	// 2D Array of 128x64 pixels for display
	// - Each pixel is represented by 1 byte
	// - Total 1024 bytes for the entire display
	// - Each byte represents 8 vertical pixels
	uint8_t (*buffer)[SSD1306_WIDTH];
	// Page Tracker
	uint16_t page: 3;
	// Column Tracker
	uint16_t col: 7;
} ssd1306_buff_t;

// SSD1306 Display Pixel Co-ordinates Structure
typedef struct {
	// X co-ordinate
	uint8_t X: 6;
	// Y co-ordinate
	uint8_t Y: 7;
} SSD1306_pix_t;

// SSD1306 Configuration Structure
typedef struct {
	// I2C Configuration Structure
	i2c_config_t i2c_config;
	// SSD1306 I2C Address: 
	// - `0x3C` (Default)
	// - `0x3D`
	uint8_t address;
	// I2C Ring Buffer
	ring_buffer_t i2c_rb;
	// Display Buffer
	ssd1306_buff_t display;
	// Cursor position
	SSD1306_pix_t cursor;
} ssd1306_config_t;
// ------------------------------------------------------------- STRUCT END ------------------------------------------------------------- //

// ------------------------------------------------------------- CONFIGURATION FUNCTION START ------------------------------------------------------------- //
/**
 * @brief Configures the I2C Ring Buffer for the SSD1306 OLED Display
 * @param ssd1306 Pointer to the SSD1306 configuration structure
 * @param i2c_buffer Pointer to the I2C buffer
 * @param i2c_buff_size Pointer to I2C buffer size 
 * @note - The I2C buffer size must be a power of 2
 * @note - If not, it will be rounded up to the next power of 2
 * @note - The updated Buffer Size will be used for the ring buffer
 */
__STATIC_FORCEINLINE void SSD1306_Config_RB(ssd1306_config_t* ssd1306, uint8_t* i2c_buffer, uint16_t i2c_buff_size){
	// Configure the I2C Ring Buffer
	Ring_Buffer_Config(&ssd1306->i2c_rb, i2c_buffer, i2c_buff_size);
}

/**
 * @brief Configures the address for Display Buffer used to track SSD1306 OLED Display
 * @param ssd1306 Pointer to the SSD1306 configuration structure
 * @param buffer Pointer to the display buffer
 * @note - The display buffer must be 2D array
 * @note - The buffer will have `SSD1306_PAGE` rows
 * @note - The buffer will have `SSD1306_WIDTH` columns
 */
__STATIC_FORCEINLINE void SSD1306_Config_Disp(ssd1306_config_t* ssd1306, uint8_t (*buffer)[SSD1306_WIDTH]){
	// Set the Display Buffer
	ssd1306->display.buffer = buffer;
	// Initialize the Cursor Position
	ssd1306->cursor.X = 0;
	ssd1306->cursor.Y = 0;
	// Initialize the Page and Column Tracker
	ssd1306->display.page = 0;
	ssd1306->display.col = 0;
}
// ------------------------------------------------------------- CONFIGURATION FUNCTION END ------------------------------------------------------------- //

#endif /* __SSD1306_CONFIG_H__ */