/**
 * @file	main.c
 * @author	Shrey Shah
 * @brief	Implements the SSD1306 horizontal-line glow/fade state-machine demo behavior
 * @version	v1.0
 * @date	09-11-2026
 *
 * @details
 * @section MAIN_C_HIERARCHY Hierarchy
 * - Position: Layer 3 - Application behavior implementation
 * - Called by: Layer 4 Reset_Handler() after App_Init() succeeds
 * - Uses: Layer 2 `app_time` and Layer 1 RCC, GPIO, BSP GPIO, I2C, and
 *   SSD1306 services
 *
 * @section MAIN_C_RESPONSIBILITY Responsibility
 * The application's single aim is to demonstrate the SSD1306 driver by
 * animating a horizontal line at the panel's vertical center through a
 * two-phase state machine. The GROW phase lights one line pixel every
 * 50 ms marching from the start column toward the end column; once the line
 * is complete the FADE phase darkens one line pixel every 50 ms marching
 * from the start column toward the end column until the line is empty, then
 * the machine re-enters GROW. The two phase actions are isolated in static
 * App_GlowLine()/App_FadeLine() functions and the while(1) loop owns only
 * the state transition and cursor arithmetic. main() owns the complete I2C1
 * and SSD1306 bring-up in one documented sequence: enable the GPIOB and
 * I2C1 gates, attach the default SCL/SDA pins, disable I2C1 for a known
 * PE=0 precondition, apply the Standard-mode root configuration, enable
 * I2C1, and hand the enabled instance to SSD1306_Init() which transmits the
 * panel power-on configuration and clears the framebuffer. A failed
 * bring-up or refresh step stops in App_ContainFault() with the board LED
 * forced high.
 *
 * @section MAIN_C_BOUNDARY Dependency Boundary
 * Application behavior belongs here. Processor startup, clock-tree
 * configuration, timebase, and optional service initialization do not.
 */

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "main.h"
#include "app_time.h"
#include "rcc.h"
#include "gpio.h"
#include "bsp_gpio.h"
#include "i2c.h"
#include "ssd1306.h"

// ==================================================================================================== //
// Private Defines
// ==================================================================================================== //

/** @brief Millisecond interval between consecutive demonstrated pixels @def SSD1306_DEMO_PIXEL_STEP_DELAY_MS */
#define SSD1306_DEMO_PIXEL_STEP_DELAY_MS			((uint32_t) 50UL)
/** @brief One-tick cadence of the steady-state LED heartbeat: the LED state flips once per interval @def SSD1306_DEMO_HEARTBEAT_PERIOD_MS */
#define SSD1306_DEMO_HEARTBEAT_PERIOD_MS			((uint32_t) 1000UL)
/** @brief Panel row the demo draws on: the vertical center of the panel @def SSD1306_DEMO_PIXEL_Y */
#define SSD1306_DEMO_PIXEL_Y						((ssd1306_row_t) (SSD1306_VERTICAL_PIXELS / 2U))
/** @brief Horizontal line start column: the leftmost column the drawn line begins at @def SSD1306_DEMO_LINE_START_X */
#define SSD1306_DEMO_LINE_START_X					(SSD1306_COLUMN_MIN)
/** @brief Horizontal line end column: the rightmost column the drawn line stops at @def SSD1306_DEMO_LINE_END_X */
#define SSD1306_DEMO_LINE_END_X						(SSD1306_COLUMN_MAX)

/**
 * @brief Contains a failed main()-owned bring-up or refresh step in a hang
 * @def SSD1306_DEMO_VALIDATE_STATUS
 * @details
 * A `return` from `main()` is not contained here: @ref `app_startup.c`
 * `Reset_Handler()` falls through it into its own infinite containment loop,
 * which would erase any observable evidence of the failing step. This local
 * check instead stops inside `main()` through @ref `App_ContainFault()`, so
 * the requested LED-fault witness and the processor halt stay reachable in
 * the same context that observed the failure.
 * @param[in] operationStatus Status value returned by the guarded service call
 */
#define SSD1306_DEMO_VALIDATE_STATUS(operationStatus)				\
	do																\
	{																\
		if ((operationStatus) != DRIVER_STATUS_SUCCESS)				\
		{															\
			App_ContainFault();										\
		}															\
	} while (0U)

// ==================================================================================================== //
// Local Types
// ==================================================================================================== //

/**
 * @brief One phase of the demo line's glow/fade animation state machine
 * @details
 * Each enumeration value names exactly the phase the while(1) loop is in,
 * and each transition follows one full pixel step: GROW until the drawing
 * head passes the line end, then FADE until the erasing head passes the
 * line end, then GROW again from a fresh pair of cursors.
 */
typedef enum _demo_state_t
{
	/** @brief Growing phase: light one pixel per step at the drawing head @memberof demo_state_t */
	DEMO_STATE_GROW_LINE = 0x00U,
	/** @brief Fading phase: darken one pixel per step at the erasing head @memberof demo_state_t */
	DEMO_STATE_FADE_LINE = 0x01U,
} demo_state_t;

// ==================================================================================================== //
// Private Data
// ==================================================================================================== //

/**
 * @brief Display state owned by the demo application
 * @details
 * The framebuffer lives in application-owned RAM instead of the main() stack
 * so its complete 1-KiB panel image does not consume stack headroom. The
 * bound I2C instance selects I2C1, matching the transport configured in
 * main(). The framebuffer bytes start uninitialized at file scope, but
 * SSD1306_Init() clears the complete framebuffer before any pixel write, and
 * the pixel coordinates start at the panel origin so the first steady-state
 * frame is fully defined even before App-flow Init runs.
 */
static ssd1306_t demoDisplay =
{
	.pI2C = I2C1,
	.pixel =
	{
		.X = SSD1306_COLUMN_MIN,
		.Y = SSD1306_DEMO_PIXEL_Y,
	}
};

// ==================================================================================================== //
// Private Functions
// ==================================================================================================== //

/**
 * @brief Halts application behavior with the fault witness forced on the board
 * @details
 * Every failed bring-up or steady-state step stops in this function instead
 * of returning to @ref `Reset_Handler()`, so the failure can be observed on
 * the board LED and in the debugger at its source.
 */
static void App_ContainFault(void)
{
#if (APP_ENABLE_ONBOARD_LED == 1U)
	//! Force the requested fault witness level on the on-board LED. The BSP
	//! on-board LED is preconfigured by App_Init(), so this call meets its
	//! public precondition.
	BSP_OB_LED_Set();
#endif /* APP_ENABLE_ONBOARD_LED */

	//! Contain the failed demo without consuming cycles while a debugger inspects it.
	while (1)
	{
		__WFI();
	}
}

/**
 * @brief Glows one line pixel: lights the addressed column
 * @details
 * One glow step of the demo state machine: writes the white pixel at the
 * drawing head so the lit prefix extends one column of the shared
 * framebuffer while the loop-level @ref SSD1306_Refresh call publishes it.
 */
__STATIC_FORCEINLINE driver_status_t App_GlowLine
(
	ssd1306_t* const		pSSD1306,
	const ssd1306_column_t	pixelX
)
{
	//! Light the drawing head so the lit prefix grows toward the end column.
	return SSD1306_WritePixel(pSSD1306, pixelX, SSD1306_DEMO_PIXEL_Y, SSD1306_COLOR_WHITE);
}

/**
 * @brief Fades one line pixel: darkens the addressed column
 * @details
 * One fade step of the demo state machine: writes the black pixel at the
 * erasing head so the lit prefix loses its leftmost column of the shared
 * framebuffer while the loop-level @ref SSD1306_Refresh call publishes it.
 */
__STATIC_FORCEINLINE driver_status_t App_FadeLine
(
	ssd1306_t* const		pSSD1306,
	const ssd1306_column_t	pixelX
)
{
	//! Darken the erasing head so the lit prefix erodes toward the end column.
	return SSD1306_WritePixel(pSSD1306, pixelX, SSD1306_DEMO_PIXEL_Y, SSD1306_COLOR_BLACK);
}

// ==================================================================================================== //
// Application Entry Point
// ==================================================================================================== //

int main(void)
{
	// Local Variables
	//! The drawing head appends one lit pixel per GROW step toward the line end.
	ssd1306_column_t pixelDrawX = SSD1306_DEMO_LINE_START_X;

	//! The erasing head darkens one lit pixel per FADE step toward the line end.
	ssd1306_column_t pixelEraseX = SSD1306_DEMO_LINE_START_X;

	//! The machine starts glowing: the line grows before any pixel can fade.
	demo_state_t demoState = DEMO_STATE_GROW_LINE;

#if (APP_ENABLE_ONBOARD_LED == 1U)
	//! Tick-derived heartbeat state, tracked across loop steps so the LED
	//! toggles on period-boundary parity flips rather than per step.
	uint32_t heartbeatSecond = 0U;
	uint8_t heartbeatSecondParity = 0U;
	uint8_t currentHeartbeatParity = 0U;
#endif /* APP_ENABLE_ONBOARD_LED */

	//! Step 1: Enable the application-owned clock gates for the GPIOB port that
	//! holds the default pins and the I2C1 peripheral itself.
	SSD1306_DEMO_VALIDATE_STATUS(RCC_SetPeripheralClockState(RCC_APB2_BUS, RCC_APB2ENR_IOPBEN, DRIVER_STATUS_ON));
	SSD1306_DEMO_VALIDATE_STATUS(RCC_SetPeripheralClockState(RCC_APB1_BUS, RCC_APB1ENR_I2C1EN, DRIVER_STATUS_ON));

	//! Step 2: Attach the BSP default SCL and SDA lines to I2C1 in
	//! alternate-function open-drain mode.
	SSD1306_DEMO_VALIDATE_STATUS
	(
		GPIO_SetPinModeConfig
		(
			BSP_GPIO_I2C1_DEFAULT_SCL_PORT,
			BSP_GPIO_I2C1_DEFAULT_SCL_PIN,
			GPIO_PIN_MODE_I2Cx_SCL,
			GPIO_PIN_CONFIG_I2Cx_SCL
		)
	);
	SSD1306_DEMO_VALIDATE_STATUS
	(
		GPIO_SetPinModeConfig
		(
			BSP_GPIO_I2C1_DEFAULT_SDA_PORT,
			BSP_GPIO_I2C1_DEFAULT_SDA_PIN,
			GPIO_PIN_MODE_I2Cx_SDA,
			GPIO_PIN_CONFIG_I2Cx_SDA
		)
	);

	//! Step 3: Disable I2C1 so the root configuration below meets its PE=0
	//! precondition from a known state instead of relying on reset defaults.
	SSD1306_DEMO_VALIDATE_STATUS(I2C_SetOperationState(I2C1, DRIVER_STATUS_OFF));

	//! Step 4: Configure I2C1 with the Standard-mode default display timing.
	i2c_config_t i2cConfig =
	{
		.busMode = I2C_BUS_STANDARD_MODE,
		.fastModeDuty = I2C_FAST_DUTY_2_1
	};
	SSD1306_DEMO_VALIDATE_STATUS(I2C_Init(I2C1, &i2cConfig));

	//! Step 5: Enable I2C1 operation so the display transport is ready before
	//! the SSD1306 controller is initialized over it.
	SSD1306_DEMO_VALIDATE_STATUS(I2C_SetOperationState(I2C1, DRIVER_STATUS_ON));

	//! Step 6: Bring the panel up through the SSD1306 Driver. The driver
	//! transmits the complete power-on configuration stream and clears the
	//! local framebuffer, so any bus failure propagates the exact I2C status
	//! into the fault branch. The framebuffer stays locally defined even when
	//! the panel GDDRAM content is not yet driven until Refresh runs.
	SSD1306_DEMO_VALIDATE_STATUS(SSD1306_Init(&demoDisplay));

	//! Infinite Loop
	while (1)
	{
		//! Application Logic
		//! One fully braced state-machine case per phase: each case performs its
		//! single pixel step through the phase action and owns exactly the cursor
		//! arithmetic and transition its phase requires.
		switch (demoState)
		{
			case DEMO_STATE_GROW_LINE:
			{
				//! Light the pixel at the drawing head so the line grows toward the right.
				SSD1306_DEMO_VALIDATE_STATUS(App_GlowLine(&demoDisplay, pixelDrawX));

				//! Step the drawing head right; once it passes the end column the
				//! line is complete and the erasing head takes over from the start column.
				if (pixelDrawX >= SSD1306_DEMO_LINE_END_X)
				{
					demoState = DEMO_STATE_FADE_LINE;
				}
				else
				{
					++pixelDrawX;
				}
				break;
			}
			case DEMO_STATE_FADE_LINE:
			{
				//! Darken the pixel at the erasing head so the line erodes toward the right.
				SSD1306_DEMO_VALIDATE_STATUS(App_FadeLine(&demoDisplay, pixelEraseX));

				//! Step the erasing head right; once it passes the end column the
				//! line is fully erased and the machine restarts glowing with fresh cursors.
				if (pixelEraseX >= SSD1306_DEMO_LINE_END_X)
				{
					pixelDrawX = SSD1306_DEMO_LINE_START_X;
					pixelEraseX = SSD1306_DEMO_LINE_START_X;
					demoState = DEMO_STATE_GROW_LINE;
				}
				else
				{
					++pixelEraseX;
				}
				break;
			}
			//! The enumeration covers both phases completely, so no default case exists.
		}

		//! Toggle the on-board LED exactly once per heartbeat period using the
		//! monotonic application tick, not the loop cadence: integer-dividing
		//! the tick by the period yields the elapsed period count, and count
		//! parity (count % 2) is 0 for one interval and 1 for the next, so the
		//! LED state changes every time the parity flips. This stays a stable
		//! 1-second cadence even though a Refresh-side step may exceed the
		//! 50 ms nominal delay, and unsigned wrap-safe timekeeping makes the
		//! 32-bit tick rollover seamless. A frozen LED still proves the loop
		//! (or bring-up) stopped, distinguishing a display/I2C problem from a
		//! total software hang.
#if (APP_ENABLE_ONBOARD_LED == 1U)
		//! The period count advances by one at every tick multiple of 1000 ms,
		//! which is the only tick-derived property the LED cares about.
		heartbeatSecond = (App_TimeGetTick() / SSD1306_DEMO_HEARTBEAT_PERIOD_MS);
		//! Each period is either odd-numbered or even-numbered, so a parity
		//! flip marks exactly the moment the next one-second interval begins.
		heartbeatSecondParity = (uint8_t) (heartbeatSecond % 2U);

		if (currentHeartbeatParity != heartbeatSecondParity)
		{
			currentHeartbeatParity = heartbeatSecondParity;
			BSP_OB_LED_Toggle();
		}
#endif /* APP_ENABLE_ONBOARD_LED */

		//! Push the framebuffer into GDDRAM once per state step so the landed pixel
		//! is visible; the full-panel window transaction re-selects column 0 before
		//! every page stream. Keeping the transfer loop-level means both phase
		//! actions stay pure framebuffer mutations.
		SSD1306_DEMO_VALIDATE_STATUS(SSD1306_Refresh(&demoDisplay));

		//! App_DelayMs() reports its own substatus, but the transport already
		//! proved healthy this frame, so a timebase failure here cannot
		//! retroactively invalidate the demonstrated pixel.
		(void) App_DelayMs(SSD1306_DEMO_PIXEL_STEP_DELAY_MS);
	}
}
