/**
 * @file ssd1306.c
 * @author Shrey Shah
 * @brief SSD1306 OLED Driver implementation
 * @version v1.0
 * @date 09-11-2026
 * @details
 * @section SSD1306_C_HIERARCHY Hierarchy
 * - Position: SSD1306 Driver implementation layer consuming the public API
 *   declared in ssd1306.h
 * - Used by: Application modules through @ref SSD1306_Init, @ref
 *   SSD1306_SendCommand, @ref SSD1306_FillFrameBuffer, @ref SSD1306_WritePixel,
 *   and @ref SSD1306_Refresh
 * - Calls: @ref I2C_MasterTransmit from the I2C Driver for every transport
 *
 * @section SSD1306_C_RESPONSIBILITY Responsibility
 * This file composes the canonical SSD1306 command vocabulary into transmit
 * streams, owns the file-scope initialization command stream, and implements
 * the framebuffer semantics (fill, pixel write, full-panel refresh). Validation
 * completes before any local mutation, and every transport result preserves
 * the exact I2C Driver status one-to-one.
 *
 * @section SSD1306_C_DEPENDENCIES Dependencies
 * Direct dependencies: ssd1306.h for the public API and state structure and
 * i2c.h for the bounded controller transmit service. The canonical payloads
 * come from ssd1306_reg_map.h through ssd1306.h.
 *
 * @section SSD1306_C_BOUNDARY Dependency Boundary
 * This driver never configures I2C, RCC, GPIO, or NVIC state and never
 * decodes received bus data; everything upstream of a transmit remains
 * application-owned.
 */
// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "ssd1306.h"
#include "i2c.h"

// ==================================================================================================== //
// Defines
// ==================================================================================================== //

/**
 * @brief Data bytes carried by one full-page framebuffer transaction
 * @def SSD1306_PAGE_STREAM_DATA_LENGTH
 * @details
 * Vertical (page) addressing stores eight pixels per column byte, so one
 * whole display page spans exactly @ref `SSD1306_COLUMN_COUNT` data bytes;
 * plus one for the framing control byte.
 */
#define SSD1306_PAGE_STREAM_DATA_LENGTH				((uint16_t) 1U + (uint16_t) SSD1306_COLUMN_COUNT)

/**
 * @brief Refresh window command-stream length including the control byte
 * @def SSD1306_WINDOW_STREAM_LENGTH
 * @details
 * Column-address, page-address, and their two parameter pairs per window
 * command transaction, plus the control byte.
 */
#define SSD1306_WINDOW_STREAM_LENGTH				((uint16_t) 7U)

/**
 * @brief Total bytes of one single-command transaction including the control byte
 * @def SSD1306_COMMAND_STREAM_LENGTH
 * @details
 * `SSD1306_SendCommand` transmits one command framing transaction: the
 * command control byte followed by exactly one opcode byte. The value is
 * typed for direct use as the I2C transmit size parameter.
 */
#define SSD1306_COMMAND_STREAM_LENGTH				((i2c_data_length_t) 2U)

/**
 * @brief Bit shift splitting a vertical row address into its GDDRAM page index
 * @def SSD1306_PAGE_ROW_SHIFT
 * @details
 * The page-selection field width: each GDDRAM byte stores eight vertical
 * pixels of one column, so `pageIndex = Y >> SSD1306_PAGE_ROW_SHIFT` yields
 * the page holding row `Y` (e.g. Y = 37 shifts to page 4). It derives from
 * `SSD1306_VERTICAL_PIXELS` spanning eight pages and fixes the largest
 * shift amount the in-page row stage may apply.
 */
#define SSD1306_PAGE_ROW_SHIFT						((ssd1306_row_t) 3U)

/**
 * @brief Bit mask selecting the row-within-page field of one vertical row
 * @def SSD1306_PAGE_ROW_MASK
 * @details
 * The in-page remainder mask paired with @ref `SSD1306_PAGE_ROW_SHIFT`:
 * `rowWithinPage = Y & SSD1306_PAGE_ROW_MASK` isolates the row a pixel
 * occupies inside its page byte, and that row value doubles as the bit
 * position to set in the page's column byte. The mask value `0x07` covers
 * every row of one page's eight-row field and clears the page-selection bits
 * above it.
 */
#define SSD1306_PAGE_ROW_MASK						((ssd1306_row_t) 0x07U)

/**
 * @brief GDDRAM page index holding the vertical pixel row @p Y
 * @def SSD1306_PIXEL_TO_PAGE
 * @details
 * Value-expression form of the page half of the pixel decomposition:
 * `SSD1306_PIXEL_TO_PAGE(Y)` shifts out the in-page bits so, e.g., Y = 37
 * evaluates to page 4. The formal `Y` is parenthesized so a caller-supplied
 * arithmetic expression cannot leak into the shift's discriminating operand.
 */
#define SSD1306_PIXEL_TO_PAGE(Y)						((ssd1306_page_t) ((Y) >> SSD1306_PAGE_ROW_SHIFT))

/**
 * @brief Row-within-page position of the vertical pixel row @p Y
 * @def SSD1306_PIXEL_TO_ROW
 * @details
 * Value-expression form of the in-page half of the pixel decomposition: the
 * row a pixel occupies inside its page byte, which doubles as the bit
 * position of the addressed pixel in that page's column byte (e.g. Y = 37
 * evaluates to bit 5, the same position @ref `SSD1306_PAGE_ROW_MASK`
 * describes). The formal `Y` is parenthesized so a caller-supplied
 * arithmetic expression cannot leak into the mask operation.
 */
#define SSD1306_PIXEL_TO_ROW(Y)						((ssd1306_row_t) ((Y) & SSD1306_PAGE_ROW_MASK))

// ==================================================================================================== //
// Static Data
// ==================================================================================================== //

/**
 * @brief Complete init command-stream length including the control byte
 * @def SSD1306_INIT_STREAM_LENGTH
 * @details
 * The array of bytes below, computed from its actual declared size so the
 * stream definition and its transmission length cannot drift apart.
 */
#define SSD1306_INIT_STREAM_LENGTH					((uint16_t) sizeof(ssd1306InitStream))

/**
 * @brief Complete power-on configuration command stream for the target panel
 * @details
 * The data control byte @ref `SSD1306_CTRL_BYTE_COMMAND` frames every
 * following byte as command vocabulary: opcodes and parameters in datasheet
 * order, ending with the controller running normal display from RAM content.
 */
static const ssd1306_cmd_t ssd1306InitStream[] =
{
	SSD1306_CTRL_BYTE_COMMAND,
	SSD1306_CMD_SET_DISPLAY_OFF,
	SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO,		SSD1306_CONFIG_DISPLAY_CLOCK,
	SSD1306_CMD_SET_MULTIPLEX_RATIO,				SSD1306_CONFIG_MULTIPLEX_RATIO,
	SSD1306_CMD_SET_DISPLAY_OFFSET,					SSD1306_CONFIG_DISPLAY_OFFSET,
	SSD1306_CMD_SET_DISPLAY_START_LINE(SSD1306_CONFIG_DISPLAY_START_LINE),
	SSD1306_CMD_SET_CHARGE_PUMP,					SSD1306_CHARGE_PUMP_ENABLE,
	SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE,			SSD1306_MEMORY_ADDRESSING_MODE_HORIZONTAL,
	SSD1306_CMD_SET_SEGMENT_REMAP_COL127_SEG0,
	SSD1306_CMD_SET_COM_SCAN_REMAPPED,
	SSD1306_CMD_SET_COM_PINS_HARDWARE_CONFIG,		SSD1306_COM_PINS_ALTERNATIVE,
	SSD1306_CMD_SET_CONTRAST,						SSD1306_CONFIG_CONTRAST,
	SSD1306_CMD_SET_PRECHARGE_PERIOD,				SSD1306_CONFIG_PRECHARGE_PERIOD,
	SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL,			SSD1306_VCOMH_DESELECT_0_65_VCC,
	SSD1306_CMD_ENTIRE_DISPLAY_FOLLOW_RAM,
	SSD1306_CMD_SET_NORMAL_DISPLAY,
	SSD1306_CMD_SET_DISPLAY_ON,
};

// ==================================================================================================== //
// Private Functions
// ==================================================================================================== //

/**
 * @brief Transmits one prepared SSD1306 control-byte stream over the bound I2C instance
 * @param[in] pSSD1306 Validated display state owning the initialized I2C peripheral
 * @param[in] pStream Complete stream: control byte followed by command or data payload
 * @param[in] length Total stream length in bytes, control byte included
 * @returns @ref driver_status_t "SSD1306 stream-transmit operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Complete stream was transmitted and STOP was generated
 * @note The remaining failure statuses map one-to-one from @ref I2C_MasterTransmit
 * @pre The caller already validated @p `pSSD1306` and its `pI2C` member; this helper performs no validation
 */
__STATIC_FORCEINLINE driver_status_t _SSD1306_TransmitStream
(
	const ssd1306_t* const		pSSD1306,
	const ssd1306_data_t* const	pStream,
	const i2c_data_length_t		length
)
{
	//! Delegation to the bounded controller transmit keeps address/START/STOP framing out of this driver.
	return I2C_MasterTransmit
	(
		pSSD1306->pI2C,
		SSD1306_I2C_ADDRESS,
		pStream,
		length,
		SSD1306_I2C_TIMEOUT_MS
	);
}

// ==================================================================================================== //
// Public Functions
// ==================================================================================================== //

driver_status_t SSD1306_SendCommand
(
	const ssd1306_t* const	pSSD1306,
	const ssd1306_cmd_t		command
)
{
	// Local Variables
	ssd1306_cmd_t stream[SSD1306_COMMAND_STREAM_LENGTH] = {0x00U, 0x00U};

	// Validate Input and State
	if (pSSD1306 == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if (pSSD1306->pI2C == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! The control byte marks this two-byte transaction as a command payload.
	stream[0] = SSD1306_CTRL_BYTE_COMMAND;
	stream[1] = command;

	return _SSD1306_TransmitStream(pSSD1306, stream, SSD1306_COMMAND_STREAM_LENGTH);
}

driver_status_t SSD1306_Init(ssd1306_t* const pSSD1306)
{
	// Local Variables
	ssd1306_page_t pageIndex = SSD1306_PAGE_MIN;
	ssd1306_column_t columnIndex = SSD1306_COLUMN_MIN;
	driver_status_t i2cState = DRIVER_STATUS_ERROR;

	// Validate Input and State
	if (pSSD1306 == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if (pSSD1306->pI2C == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Validate if I2C Peripheral is Clocked & Operational
	{
		i2cState = I2C_GetOperationState(pSSD1306->pI2C);
		if(i2cState != DRIVER_STATUS_ON)
		{
			return i2cState;
		}
	}

	//! Commit the panel configuration before local state is published so a bus
	//! failure leaves the framebuffer untouched and no partial controller setup behind.
	i2cState = _SSD1306_TransmitStream(pSSD1306, ssd1306InitStream, SSD1306_INIT_STREAM_LENGTH);
	ASSERT_DRIVER_STATUS(i2cState);

	//! Clearing every page-column byte zeroes all encoded pixels for a clean baseline.
	for (pageIndex = SSD1306_PAGE_MIN; pageIndex <= SSD1306_PAGE_MAX; ++pageIndex)
	{
		for (columnIndex = SSD1306_COLUMN_MIN; columnIndex <= SSD1306_COLUMN_MAX; ++columnIndex)
		{
			pSSD1306->frameBuffer[pageIndex][columnIndex] = SSD1306_FILL_PATTERN_BLACK;
		}
	}

	return DRIVER_STATUS_SUCCESS;
}

driver_status_t SSD1306_FillFrameBuffer
(
	ssd1306_t* const		pSSD1306,
	const ssd1306_data_t	pattern
)
{
	// Local Variables
	ssd1306_page_t pageIndex = 0x00U;
	ssd1306_column_t columnIndex = 0x00U;

	// Validate Input and State
	if (pSSD1306 == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Each byte encodes eight vertical pixels, so one byte fill sets the pattern per column.
	for (pageIndex = SSD1306_PAGE_MIN; pageIndex <= SSD1306_PAGE_MAX; ++pageIndex)
	{
		for (columnIndex = SSD1306_COLUMN_MIN; columnIndex <= SSD1306_COLUMN_MAX; ++columnIndex)
		{
			pSSD1306->frameBuffer[pageIndex][columnIndex] = pattern;
		}
	}

	return DRIVER_STATUS_SUCCESS;
}

driver_status_t SSD1306_WritePixel
(
	ssd1306_t* const		pSSD1306,
	const ssd1306_column_t	X,
	const ssd1306_row_t		Y,
	const ssd1306_data_t	color
)
{
	// Local Variables
	ssd1306_page_t pageIndex = 0x00U;
	ssd1306_row_t rowWithinPage = 0x00U;

	// Validate Input and State
	if (pSSD1306 == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! Reject out-of-geometry coordinates before touching any framebuffer byte.
	if ((X >= SSD1306_COLUMN_COUNT) || (Y >= SSD1306_VERTICAL_PIXELS) || ((color != SSD1306_COLOR_BLACK) && (color != SSD1306_COLOR_WHITE)))
	{
		return DRIVER_STATUS_ERROR_INVALID_ARG;
	}

	//! (SSD1306_PIXEL_TO_PAGE, SSD1306_PIXEL_TO_ROW) maps one vertical pixel to its
	//! page and bit-in-byte because each GDDRAM byte stores eight pixels of
	//! one column: e.g. Y = 37 evaluates to page 4, row 5, so bit 0x20 is the
	//! addressed pixel. One entry point serves white (set the bit) and black
	//! (clear the bit in place) without an OR-only set-pixel asymmetry.
	pageIndex = SSD1306_PIXEL_TO_PAGE(Y);
	rowWithinPage = SSD1306_PIXEL_TO_ROW(Y);
	if (color == SSD1306_COLOR_WHITE)
	{
		//! OR-merge keeps the seven co-resident pixels intact while lighting the addressed one.
		pSSD1306->frameBuffer[pageIndex][X] |= (ssd1306_data_t) ((ssd1306_row_t) 1U << rowWithinPage);
	}
	else
	{
		//! Invert-and-AND keeps the seven co-resident pixels intact while clearing only the addressed one.
		pSSD1306->frameBuffer[pageIndex][X] &= (ssd1306_data_t) ~((ssd1306_row_t) 1U << rowWithinPage);
	}

	return DRIVER_STATUS_SUCCESS;
}

driver_status_t SSD1306_Refresh(ssd1306_t* const pSSD1306)
{
	// Local Variables
	const ssd1306_cmd_t windowStream[SSD1306_WINDOW_STREAM_LENGTH] =
	{
		SSD1306_CTRL_BYTE_COMMAND,
		SSD1306_CMD_SET_COLUMN_ADDRESS, (ssd1306_cmd_t) SSD1306_COLUMN_MIN, (ssd1306_cmd_t) SSD1306_COLUMN_MAX,
		SSD1306_CMD_SET_PAGE_ADDRESS, (ssd1306_cmd_t) SSD1306_PAGE_MIN, (ssd1306_cmd_t) SSD1306_PAGE_MAX,
	};
	//! Zero-filling first guarantees every byte has a defined value even if a
	//! later copy path ever writes fewer elements than it transmits.
	ssd1306_cmd_t pageStream[SSD1306_PAGE_STREAM_DATA_LENGTH] =
	{
		[0 ... SSD1306_PAGE_STREAM_DATA_LENGTH - 1] = SSD1306_FILL_PATTERN_BLACK
	};
	ssd1306_page_t pageIndex = 0x00U;
	ssd1306_column_t columnIndex = 0x00U;

	// Validate Input and State
	if (pSSD1306 == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}
	if (pSSD1306->pI2C == NULL)
	{
		return DRIVER_STATUS_ERROR_NULL_PTR;
	}

	//! The full-panel window must be reselected so every page transaction starts at column 0.
	ASSERT_DRIVER_STATUS(_SSD1306_TransmitStream(pSSD1306, windowStream, SSD1306_WINDOW_STREAM_LENGTH));

	//! One data transaction per page: the control byte frames the whole page,
	//! and horizontal addressing advances to the next page when 128 columns
	//! complete, so sequential writes walk the whole window without per-page addressing.
	pageStream[0] = SSD1306_CTRL_BYTE_DATA;
	for (pageIndex = SSD1306_PAGE_MIN; pageIndex <= SSD1306_PAGE_MAX; ++pageIndex)
	{
		for (columnIndex = SSD1306_COLUMN_MIN; columnIndex <= SSD1306_COLUMN_MAX; ++columnIndex)
		{
			//! Positive offset 1 skips the data control byte in the local copy.
			pageStream[columnIndex + 1U] = pSSD1306->frameBuffer[pageIndex][columnIndex];
		}

		//! The page stream is complete and ready to transmit; the controller auto-advances to the next page after 128 columns.
		ASSERT_DRIVER_STATUS(_SSD1306_TransmitStream(pSSD1306, pageStream, SSD1306_PAGE_STREAM_DATA_LENGTH));
	}

	return DRIVER_STATUS_SUCCESS;
}
