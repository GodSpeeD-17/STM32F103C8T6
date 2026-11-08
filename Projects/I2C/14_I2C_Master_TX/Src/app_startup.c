/**
 * @file	app_startup.c
 * @author	Shrey Shah
 * @brief	Owns the application vector table and reset-to-main sequence
 * @version	v1.0
 * @date	22-08-2026
 *
 * @details
 * @section APP_STARTUP_C_HIERARCHY Hierarchy
 * - Position: Layer 4 - Processor entry implementation
 * - Invoked by: Processor reset and exception dispatch
 * - Uses: Layer 3 App_Init() and main()
 *
 * @section APP_STARTUP_C_RESPONSIBILITY Responsibility
 * This is the only translation unit that defines the vector table and weak
 * fallback handlers. Reset_Handler() copies `.data`, clears `.bss`, calls
 * App_Init(), and transfers control to main().
 *
 * @section APP_STARTUP_C_BOUNDARY Dependency Boundary
 * Startup owns processor and C-runtime handoff only. Peripheral policy, heap
 * adaptation, and application behavior remain in lower modules, and no lower
 * module includes `app_startup.h`.
 */

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "app_startup.h"
#include "app_init.h"
#include "main.h"
#include "stm32f1xx.h"

// ==================================================================================================== //
// Private Defines
// ==================================================================================================== //

/** @brief Number of Cortex-M3 core vector entries @def APP_CORE_VECTOR_COUNT */
#define APP_CORE_VECTOR_COUNT			((uint32_t) 16UL)
/** @brief Number of STM32F103C8T6 external interrupt vectors @def APP_EXTERNAL_VECTOR_COUNT */
#define APP_EXTERNAL_VECTOR_COUNT		((uint32_t) 43UL)

/**
 * @brief Declares a processor handler as a weak alias of Default_Handler()
 * @details A project may override any declaration with one strong handler definition.
 * @param[in] handlerName Processor-defined exception or interrupt handler identifier
 * @def APP_DECLARE_DEFAULT_HANDLER
 */
#define APP_DECLARE_DEFAULT_HANDLER(handlerName)		\
	void handlerName(void) __attribute__((weak, alias("Default_Handler")))

// ==================================================================================================== //
// Weak Exception Handlers
// ==================================================================================================== //

APP_DECLARE_DEFAULT_HANDLER(NMI_Handler);
APP_DECLARE_DEFAULT_HANDLER(HardFault_Handler);
APP_DECLARE_DEFAULT_HANDLER(MemManage_Handler);
APP_DECLARE_DEFAULT_HANDLER(BusFault_Handler);
APP_DECLARE_DEFAULT_HANDLER(UsageFault_Handler);
APP_DECLARE_DEFAULT_HANDLER(SVC_Handler);
APP_DECLARE_DEFAULT_HANDLER(DebugMon_Handler);
APP_DECLARE_DEFAULT_HANDLER(PendSV_Handler);
APP_DECLARE_DEFAULT_HANDLER(SysTick_Handler);
APP_DECLARE_DEFAULT_HANDLER(WWDG_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(PVD_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TAMPER_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(RTC_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(FLASH_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(RCC_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(EXTI0_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(EXTI1_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(EXTI2_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(EXTI3_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(EXTI4_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(DMA1_Channel1_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(DMA1_Channel2_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(DMA1_Channel3_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(DMA1_Channel4_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(DMA1_Channel5_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(DMA1_Channel6_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(DMA1_Channel7_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(ADC1_2_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(USB_HP_CAN_TX_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(USB_LP_CAN_RX0_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(CAN_RX1_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(CAN_SCE_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(EXTI9_5_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TIM1_BRK_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TIM1_UP_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TIM1_TRG_COM_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TIM1_CC_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TIM2_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TIM3_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(TIM4_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(I2C1_EV_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(I2C1_ER_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(I2C2_EV_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(I2C2_ER_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(SPI1_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(SPI2_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(USART1_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(USART2_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(USART3_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(EXTI15_10_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(RTCAlarm_IRQHandler);
APP_DECLARE_DEFAULT_HANDLER(USBWakeUp_IRQHandler);

// ==================================================================================================== //
// Vector Table
// ==================================================================================================== //

/**
 * @brief Project-owned processor vector table placed at the Flash image origin
 * @details Contains 16 Cortex-M3 core entries followed by 43 device IRQ entries.
 */
__attribute__((used, section(".isr_vector")))
static const uintptr_t appVectorTable[APP_CORE_VECTOR_COUNT + APP_EXTERNAL_VECTOR_COUNT] =
{
	(uintptr_t) &_estack,
	(uintptr_t) Reset_Handler,
	(uintptr_t) NMI_Handler,
	(uintptr_t) HardFault_Handler,
	(uintptr_t) MemManage_Handler,
	(uintptr_t) BusFault_Handler,
	(uintptr_t) UsageFault_Handler,
	0UL,
	0UL,
	0UL,
	0UL,
	(uintptr_t) SVC_Handler,
	(uintptr_t) DebugMon_Handler,
	0UL,
	(uintptr_t) PendSV_Handler,
	(uintptr_t) SysTick_Handler,
	(uintptr_t) WWDG_IRQHandler,
	(uintptr_t) PVD_IRQHandler,
	(uintptr_t) TAMPER_IRQHandler,
	(uintptr_t) RTC_IRQHandler,
	(uintptr_t) FLASH_IRQHandler,
	(uintptr_t) RCC_IRQHandler,
	(uintptr_t) EXTI0_IRQHandler,
	(uintptr_t) EXTI1_IRQHandler,
	(uintptr_t) EXTI2_IRQHandler,
	(uintptr_t) EXTI3_IRQHandler,
	(uintptr_t) EXTI4_IRQHandler,
	(uintptr_t) DMA1_Channel1_IRQHandler,
	(uintptr_t) DMA1_Channel2_IRQHandler,
	(uintptr_t) DMA1_Channel3_IRQHandler,
	(uintptr_t) DMA1_Channel4_IRQHandler,
	(uintptr_t) DMA1_Channel5_IRQHandler,
	(uintptr_t) DMA1_Channel6_IRQHandler,
	(uintptr_t) DMA1_Channel7_IRQHandler,
	(uintptr_t) ADC1_2_IRQHandler,
	(uintptr_t) USB_HP_CAN_TX_IRQHandler,
	(uintptr_t) USB_LP_CAN_RX0_IRQHandler,
	(uintptr_t) CAN_RX1_IRQHandler,
	(uintptr_t) CAN_SCE_IRQHandler,
	(uintptr_t) EXTI9_5_IRQHandler,
	(uintptr_t) TIM1_BRK_IRQHandler,
	(uintptr_t) TIM1_UP_IRQHandler,
	(uintptr_t) TIM1_TRG_COM_IRQHandler,
	(uintptr_t) TIM1_CC_IRQHandler,
	(uintptr_t) TIM2_IRQHandler,
	(uintptr_t) TIM3_IRQHandler,
	(uintptr_t) TIM4_IRQHandler,
	(uintptr_t) I2C1_EV_IRQHandler,
	(uintptr_t) I2C1_ER_IRQHandler,
	(uintptr_t) I2C2_EV_IRQHandler,
	(uintptr_t) I2C2_ER_IRQHandler,
	(uintptr_t) SPI1_IRQHandler,
	(uintptr_t) SPI2_IRQHandler,
	(uintptr_t) USART1_IRQHandler,
	(uintptr_t) USART2_IRQHandler,
	(uintptr_t) USART3_IRQHandler,
	(uintptr_t) EXTI15_10_IRQHandler,
	(uintptr_t) RTCAlarm_IRQHandler,
	(uintptr_t) USBWakeUp_IRQHandler
};

// ==================================================================================================== //
// Default Exception Handler
// ==================================================================================================== //

void Default_Handler(void)
{
	while (1)
	{
		//! Contain the fault without consuming cycles while a debugger inspects processor state.
		__WFI();
	}
}

// ==================================================================================================== //
// Reset Handler
// ==================================================================================================== //

__attribute__((noreturn)) void Reset_Handler(void)
{
	uint32_t* pSource = &_sidata;
	uint32_t* pDestination = &_sdata;

	//! Materialize initialized C objects from their linker-defined Flash load image.
	while (pDestination < &_edata)
	{
		*pDestination = *pSource;
		pDestination++;
		pSource++;
	}

	//! Establish the zero-initialization guarantee required before any C service executes.
	pDestination = &_sbss;
	while (pDestination < &_ebss)
	{
		*pDestination = 0UL;
		pDestination++;
	}

	//! Prevent partially initialized hardware from reaching application behavior.
	if (App_Init() != DRIVER_STATUS_SUCCESS)
	{
		Default_Handler();
	}

	//! Transfer control only after every selected application service is ready.
	(void) main();

	//! Contain an application that unexpectedly returns through the reset entry path.
	while (1)
	{
		Default_Handler();
	}
}
