/**
 * @file	main.c
 * @author	Shrey Shah
 * @brief	Implements the I2C master-transmit demo application behavior
 * @version	v1.0
 * @date	22-08-2026
 *
 * @details
 * @section MAIN_C_HIERARCHY Hierarchy
 * - Position: Layer 3 - Application behavior implementation
 * - Called by: Layer 4 Reset_Handler() after App_Init() succeeds
 * - Uses: Layer 2 `app_time` and Layer 1 RCC, GPIO, BSP GPIO, and I2C services
 *
 * @section MAIN_C_RESPONSIBILITY Responsibility
 * The application's single aim is to get an ACK from the test slave at the
 * fixed 7-bit address, proving the slave is present and responded to the
 * I2C master. main() owns the complete I2C1 test bring-up in one documented
 * sequence: enable the GPIOB and I2C1 gates, attach the default SCL/SDA
 * pins, disable I2C1 for a known PE=0 precondition, apply the Standard-mode
 * root configuration, enable I2C1, and probe the slave with the smallest
 * legal one-byte transmission whose addressee ACK is the success signal.
 * A failed step stops in App_ContainFault() with the board LED forced high;
 * a proven presence is witnessed by a steady one-second on-board LED toggle.
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

// ==================================================================================================== //
// Private Defines
// ==================================================================================================== //

/** @brief 7-bit address of the test slave whose ACK proves its presence @def TEST_SLAVE_ADDRESS */
#define TEST_SLAVE_ADDRESS						((i2c_address_t) 0x3CU)
// #define TEST_SLAVE_ADDRESS						((i2c_address_t) 0x57U)
/** @brief Millisecond budget granted to the validation transmit @def TEST_TRANSFER_TIMEOUT_MS */
#define TEST_TRANSFER_TIMEOUT_MS				((uint32_t) 100UL)
/** @brief On-board LED toggle interval after the presence proof succeeded @def TEST_LED_TOGGLE_DELAY_MS */
#define TEST_LED_TOGGLE_DELAY_MS				((uint32_t) 1000UL)

/**
 * @brief Contains a failed main()-owned bring-up or probe step in a hang
 * @def TEST_VALIDATE_STATUS
 * @details
 * A `return` from `main()` is not contained here: @ref `app_startup.c`
 * `Reset_Handler()` falls through it into its own infinite containment loop,
 * which would erase any observable evidence of the failing step. This local
 * check instead stops inside `main()` through @ref `App_ContainFault()`, so
 * the requested LED-fault witness and the processor halt stay reachable in
 * the same context that observed the failure.
 * @param[in] operationStatus Status value returned by the guarded service call
 */
#define TEST_VALIDATE_STATUS(operationStatus)				\
	do														\
	{														\
		if ((operationStatus) != DRIVER_STATUS_SUCCESS)		\
		{													\
			App_ContainFault();								\
		}													\
	} while (0U)

// ==================================================================================================== //
// Private Functions
// ==================================================================================================== //

/**
 * @brief Halts application behavior with the fault witness forced on the board
 * @details
 * Every failed Step 1..6 sequence stops in this function instead of returning
 * to @ref `Reset_Handler()`, so the failure can be observed on the board LED
 * and in the debugger at its source.
 */
static void App_ContainFault(void)
{
#if (APP_ENABLE_ONBOARD_LED == 1U)
	//! Force the requested fault witness level on the on-board LED. The BSP
	//! on-board LED is preconfigured by App_Init(), so this call meets its
	//! public precondition.
	BSP_OB_LED_Set();
#endif /* APP_ENABLE_ONBOARD_LED */

	//! Contain the failed test without consuming cycles while a debugger inspects it.
	while (1)
	{
		__WFI();
	}
}

// ==================================================================================================== //
// Application Entry Point
// ==================================================================================================== //

int main(void)
{
	// Local Variables
	//! The transfer API requires at least one payload byte, so a single dummy byte
	i2c_data_t dummyData = 0x00U;

	//! Step 1: Enable the application-owned clock gates for the GPIOB port that
	//! holds the default pins and the I2C1 peripheral itself.
	TEST_VALIDATE_STATUS(RCC_SetPeripheralClockState(RCC_APB2_BUS, RCC_APB2ENR_IOPBEN, DRIVER_STATUS_ON));
	TEST_VALIDATE_STATUS(RCC_SetPeripheralClockState(RCC_APB1_BUS, RCC_APB1ENR_I2C1EN, DRIVER_STATUS_ON));

	//! Step 2: Attach the BSP default SCL and SDA lines to I2C1 in
	//! alternate-function open-drain mode.
	TEST_VALIDATE_STATUS
	(
		GPIO_SetPinModeConfig
		(
			BSP_GPIO_I2C1_DEFAULT_SCL_PORT,
			BSP_GPIO_I2C1_DEFAULT_SCL_PIN,
			GPIO_PIN_MODE_I2Cx_SCL,
			GPIO_PIN_CONFIG_I2Cx_SCL
		)
	);
	TEST_VALIDATE_STATUS
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
	TEST_VALIDATE_STATUS(I2C_SetOperationState(I2C1, DRIVER_STATUS_OFF));

	//! Step 4: Configure I2C1 with the Standard-mode default test timing.
	i2c_config_t i2cConfig =
	{
		.busMode = I2C_BUS_STANDARD_MODE,
		.fastModeDuty = I2C_FAST_DUTY_2_1
	};
	TEST_VALIDATE_STATUS(I2C_Init(I2C1, &i2cConfig));

	//! Step 5: Enable I2C1 operation for the validation transfer.
	TEST_VALIDATE_STATUS(I2C_SetOperationState(I2C1, DRIVER_STATUS_ON));

	//! Step 6: Probe the test slave. A returned success is the whole aim of the
	//! project: it proves the slave was present and acknowledged the I2C
	//! master. A missing slave instead NACKs the address phase, so the transfer
	//! fails through the single cleanup path and the fault branch contains it.
	//! Each status family below sits in its own debugger-reachable branch, so
	//! the breakpoint on @ref `App_ContainFault()` names which family ended the
	//! probe instead of a macro collapsing all of them together.
	driver_status_t probeStatus = I2C_MasterTransmit
	(
		I2C1,
		TEST_SLAVE_ADDRESS,
		&dummyData,
		1U,
		TEST_TRANSFER_TIMEOUT_MS
	);
	//! Non-Success Chain
	if (probeStatus != DRIVER_STATUS_SUCCESS)
	{
		App_ContainFault();
	}

	//! Infinite Loop
	while (1)
	{
		//! Application Logic
		//! The observed ACK proves the slave is present, so the steady one-per-second
		//! toggle is the success witness a board observer can see without tools.
		//! App_DelayMs() reports its own substatus, but the presence proof already
		//! passed, so a timebase failure here cannot retroactively change the verdict.
		BSP_OB_LED_Toggle();
		(void) App_DelayMs(TEST_LED_TOGGLE_DELAY_MS);
	}
}
