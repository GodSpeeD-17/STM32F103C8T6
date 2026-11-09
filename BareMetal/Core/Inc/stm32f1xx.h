/**
 * @file	stm32f1xx.h
 * @author  Shrey Shah
 * @version v1.2
 * @date	21-10-2025
 *
 * 
 * @defgroup STM32F1xx STM32F1 Memory Address Mapping
 * 
 * @brief   STM32F1xx - Peripheral Register Mapping
 *
 * @details 
 * This file provides centralized memory mapping for all peripherals for STM32F1xx Series
 *  
 */
 
// Header Guards
#ifndef STM32F1XX_H_
#define STM32F1XX_H_

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// ------------------------------------------------------------------------------------------
// Data Types
// ------------------------------------------------------------------------------------------
#include "stm32f1xx_data_types.h"

// ------------------------------------------------------------------------------------------
// Cortex-M3 Core Peripheral Register Structures
// ------------------------------------------------------------------------------------------
#include "cmsis_gcc.h"
#include "stm32f1xx_defines.h"
#include "stm32f1xx_scb.h"
#include "stm32f1xx_systick.h"

// ------------------------------------------------------------------------------------------
// STM32F103C8T6
// ------------------------------------------------------------------------------------------
#include "stm32f1xx_utils.h"
#include "stm32f1xx_base_address.h"
#include "stm32f1xx_adc.h"
#include "stm32f1xx_advtim.h"
#include "stm32f1xx_afio.h"
#include "stm32f1xx_can.h"
#include "stm32f1xx_dma.h"
#include "stm32f1xx_exti.h"
#include "stm32f1xx_gpio.h"
#include "stm32f1xx_i2c.h"
#include "stm32f1xx_spi.h"
#include "stm32f1xx_timer.h"
#include "stm32f1xx_rcc.h"
#include "stm32f1xx_usart.h"
#include "stm32f1xx_watchdog.h"
#include "stm32f1xx_flash.h"
#include "stm32f1xx_nvic.h"

// ========================================================================================== //
//										Address Mapping											//
// ========================================================================================== //

// ------------------------------------------------------------------------------------------
// Cortex-M3 Core Peripherals
// ------------------------------------------------------------------------------------------
/** @brief System Control Block  (Cortex-M3 Core Peripheral) @def SCB */
#define SCB										((SCB_TypeDef*) SCB_BASE_ADDR)
/** @brief Nested Vectored Interrupt Controller  (Cortex-M3 Core Peripheral) @def NVIC */
#define NVIC									((NVIC_TypeDef*) NVIC_BASE_ADDR)
// ------------------------------------------------------------------------------------------
// AHB Peripherals
// ------------------------------------------------------------------------------------------
/** @brief DMA1 Controller @def DMA1 */
#define DMA1									((DMA_TypeDef*) DMA1_BASE_ADDR)
/**
 * @brief DMA1 Channel 1 @def DMA1_Channel1
 * @details Channel 1 of DMA1 controller
 * @note  - Address: DMA1 base + 0x08 (channel 1 register offset)
 * @note  - Connected peripherals: ADC1, TIM2_CH3, TIM4_CH1
 */
#define DMA1_Channel1 							((DMA_Channel_TypeDef*) (&DMA1->CHANNEL[0]))
/**
 * @brief DMA1 Channel 2 @def DMA1_Channel2
 * @details Channel 2 of DMA1 controller
 * @note  - Address: DMA1 base + 0x1C (channel 2 register offset)
 * @note  - Connected peripherals: SPI1_RX, TIM1_CH1, TIM2_UP, TIM3_CH3
 */
#define DMA1_Channel2 							((DMA_Channel_TypeDef*) (&DMA1->CHANNEL[1]))
/**
 * @brief DMA1 Channel 3 @def DMA1_Channel3
 * @details Channel 3 of DMA1 controller
 * @note  - Address: DMA1 base + 0x30 (channel 3 register offset)
 * @note  - Connected peripherals: SPI1_TX, TIM1_CH2, TIM3_CH4, TIM3_UP
 */
#define DMA1_Channel3 							((DMA_Channel_TypeDef*) (&DMA1->CHANNEL[2]))
/**
 * @brief DMA1 Channel 4 @def DMA1_Channel4
 * @details Channel 4 of DMA1 controller
 * @note  - Address: DMA1 base + 0x44 (channel 4 register offset)
 * @note  - Connected peripherals: SPI2_RX, USART1_TX, I2C2_TX, TIM1_CH4, TIM2_CH1, TIM4_CH3
 */
#define DMA1_Channel4 							((DMA_Channel_TypeDef*) (&DMA1->CHANNEL[3]))
/**
 * @brief DMA1 Channel 5 @def DMA1_Channel5
 * @details Channel 5 of DMA1 controller
 * @note  - Address: DMA1 base + 0x58 (channel 5 register offset)
 * @note  - Connected peripherals: SPI2_TX, USART1_RX, I2C2_RX, TIM1_CH1, TIM2_CH2, TIM4_CH4
 */
#define DMA1_Channel5 							((DMA_Channel_TypeDef*) (&DMA1->CHANNEL[4]))
/**
 * @brief DMA1 Channel 6 @def DMA1_Channel6
 * @details Channel 6 of DMA1 controller
 * @note  - Address: DMA1 base + 0x6C (channel 6 register offset)
 * @note  - Connected peripherals: USART2_RX, I2C1_TX, TIM1_CH3, TIM3_CH1, TIM3_TRIG
 */
#define DMA1_Channel6 							((DMA_Channel_TypeDef*) (&DMA1->CHANNEL[5]))
/**
 * @brief DMA1 Channel 7 @def DMA1_Channel7
 * @details Channel 7 of DMA1 controller
 * @note  - Address: DMA1 base + 0x80 (channel 7 register offset)
 * @note  - Connected peripherals: USART2_TX, I2C1_RX, TIM2_CH2, TIM2_CH4, TIM4_UP
 */
#define DMA1_Channel7 							((DMA_Channel_TypeDef*) (&DMA1->CHANNEL[6]))
/** @brief DMA2 Controller @def DMA2 */
#define DMA2									((DMA_TypeDef*) DMA2_BASE_ADDR)
/**
 * @brief DMA2 Channel 1 @def DMA2_Channel1
 * @details Channel 1 of DMA2 controller
 * @note  - Available only on high-density STM32F103 devices
 * @note  - Address: DMA2 base + 0x08 (channel 1 register offset)
 * @note  - Connected peripherals: ADC3, TIM1_CH1, TIM8_CH3, TIM8_UP
 */
#define DMA2_Channel1 							((DMA_Channel_TypeDef*)(&DMA2->CHANNEL[0]))
/**
 * @brief DMA2 Channel 2 @def DMA2_Channel2
 * @details Channel 2 of DMA2 controller
 * @note  - Available only on high-density STM32F103 devices
 * @note  - Address: DMA2 base + 0x1C (channel 2 register offset)
 * @note  - Connected peripherals: TIM1_CH2, TIM8_CH4, SPI1_RX, USART3_TX
 */
#define DMA2_Channel2 							((DMA_Channel_TypeDef*)(&DMA2->CHANNEL[1]))
/**
 * @brief DMA2 Channel 3 @def DMA2_Channel3
 * @details Channel 3 of DMA2 controller
 * @note  - Available only on high-density STM32F103 devices
 * @note  - Address: DMA2 base + 0x30 (channel 3 register offset)
 * @note  - Connected peripherals: TIM1_CH3, TIM8_CH1, SPI1_TX, USART3_RX
 */
#define DMA2_Channel3 							((DMA_Channel_TypeDef*)(&DMA2->CHANNEL[2]))
/**
 * @brief DMA2 Channel 4 @def DMA2_Channel4
 * @details Channel 4 of DMA2 controller
 * @note  - Available only on high-density STM32F103 devices
 * @note  - Address: DMA2 base + 0x44 (channel 4 register offset)
 * @note  - Connected peripherals: TIM1_CH4, TIM8_CH2, I2C2_TX, USART1_TX
 */
#define DMA2_Channel4 							((DMA_Channel_TypeDef*)(&DMA2->CHANNEL[3]))
/**
 * @brief DMA2 Channel 5 @def DMA2_Channel5
 * @details Channel 5 of DMA2 controller
 * @note  - Available only on high-density STM32F103 devices
 * @note  - Address: DMA2 base + 0x58 (channel 5 register offset)
 * @note  - Connected peripherals: TIM1_UP, TIM8_CH3, I2C2_RX, USART1_RX
 */
#define DMA2_Channel5 							((DMA_Channel_TypeDef *)(&DMA2->CHANNEL[4]))
/** @brief RCC Memory Address @def RCC  */
#define RCC 									((RCC_TypeDef*) RCC_BASE_ADDRESS)
/** @brief Flash Memory Interface  @def FLASH */
#define FLASH									((FLASH_TypeDef*) FLASH_BASE_ADDR)

// ------------------------------------------------------------------------------------------
// APB1 Peripherals
// ------------------------------------------------------------------------------------------
/** @brief General-purpose Timer 2  @def TIM2 */
#define TIM2									((TIM_TypeDef*) TIM2_BASE_ADDRESS)
/** @brief General-purpose Timer 3  @def TIM3 */
#define TIM3									((TIM_TypeDef*) TIM3_BASE_ADDRESS)
/** @brief General-purpose Timer 4  @def TIM4 */
#define TIM4									((TIM_TypeDef*) TIM4_BASE_ADDRESS)
/** @brief General-purpose Timer 5  @def TIM5 */
#define TIM5									((TIM_TypeDef*) TIM5_BASE_ADDRESS)
/** @brief Basic Timer 6  @def TIM6 */
#define TIM6									((TIM_TypeDef*) TIM6_BASE_ADDRESS)
/** @brief Basic Timer 7  @def TIM7 */
#define TIM7									((TIM_TypeDef*) TIM7_BASE_ADDRESS)
/** @brief Window Watchdog @def WWDG */
#define WWDG									((WWDG_TypeDef*) WWDG_BASE_ADDR)
/** @brief Independent Watchdog @def IWDG */
#define IWDG									((IWDG_TypeDef*) IWDG_BASE_ADDR)
/** @brief SPI2/I2S2 Peripheral @def SPI2 */
#define SPI2									((SPI_TypeDef*) SPI2_BASE_ADDRESS)
/** @brief USART2 Peripheral @def USART2 */
#define USART2									((USART_TypeDef*) USART2_BASE_ADDRESS)
/** @brief USART3 Peripheral @def USART3 */
#define USART3									((USART_TypeDef*) USART3_BASE_ADDRESS)
/** @brief I2C1 Peripheral  @def I2C1 */ 
#define I2C1 									((I2C_TypeDef*) I2C1_BASE_ADDRESS)
/** @brief I2C2 Peripheral  @def I2C2 */ 
#define I2C2 									((I2C_TypeDef*) I2C2_BASE_ADDRESS)
/** @brief Controller Area Network peripheral @def CAN1 */
#define CAN1									((CAN_TypeDef*) CAN1_BASE_ADDRESS)

// ------------------------------------------------------------------------------------------
// APB2 Peripherals
// ------------------------------------------------------------------------------------------
/** @brief Alternate Function I/O @def AFIO */
#define AFIO									((AFIO_TypeDef*) AFIO_BASE_ADDRESS)
/** @brief External Interrupt/Event Controller @def EXTI */
#define EXTI									((EXTI_TypeDef*) EXTI_BASE_ADDRESS)
/** @brief General Purpose I/O Port A @def GPIOA */
#define GPIOA									((GPIO_TypeDef*) GPIOA_BASE_ADDRESS)
/** @brief General Purpose I/O Port B @def GPIOB */
#define GPIOB									((GPIO_TypeDef*) GPIOB_BASE_ADDRESS)
/** @brief General Purpose I/O Port C @def GPIOC */
#define GPIOC									((GPIO_TypeDef*) GPIOC_BASE_ADDRESS)
/** @brief General Purpose I/O Port D @def GPIOD */
#define GPIOD									((GPIO_TypeDef*) GPIOD_BASE_ADDRESS)
/** @brief General Purpose I/O Port E @def GPIOE */
#define GPIOE									((GPIO_TypeDef*) GPIOE_BASE_ADDRESS)
/** @brief General Purpose I/O Port F @def GPIOF */
#define GPIOF									((GPIO_TypeDef*) GPIOF_BASE_ADDRESS)
/** @brief General Purpose I/O Port G @def GPIOG */
#define GPIOG									((GPIO_TypeDef*) GPIOG_BASE_ADDRESS)
/** @brief ADC1  @def ADC1 */
#define ADC1									((ADC_TypeDef*) ADC1_BASE_ADDRESS)
/** @brief ADC2  @def ADC2 */
#define ADC2									((ADC_TypeDef*) ADC2_BASE_ADDRESS)
/** @brief Advanced-control Timer 1 @def TIM1 */
#define TIM1									((Adv_TIM_TypeDef*) TIM1_BASE_ADDRESS)
/** @brief SPI1 Peripheral @def SPI1 */
#define SPI1									((SPI_TypeDef*) SPI1_BASE_ADDRESS)
/** @brief Advanced-control Timer 8 @def TIM8 */
#define TIM8									((Adv_TIM_TypeDef*) TIM8_BASE_ADDRESS)
/** @brief USART1  @def USART1 */
#define USART1									((USART_TypeDef*) USART1_BASE_ADDRESS)

/*----------------------------------------------- PWM MACROS -----------------------------------------------*/
// PWM Min Duty Cycle: 1%
#define PWM_MIN_DUTY_CYCLE (1)
// PWM Max Duty Cycle: 100%
#define PWM_MAX_DUTY_CYCLE (999)
// Max Counter Value for PWM
#define PWM_DEFAULT_ARR ((uint16_t)999)

// ADC Max Value
#define MAX_ADC_VALUE ((uint16_t)0x0FFF)

// ADC Channel
#define ADC_CHANNEL_0 (0)
#define ADC_CHANNEL_1 (1)
#define ADC_CHANNEL_2 (2)
#define ADC_CHANNEL_3 (3)
#define ADC_CHANNEL_4 (4)
#define ADC_CHANNEL_5 (5)
#define ADC_CHANNEL_6 (6)
#define ADC_CHANNEL_7 (7)
#define ADC_CHANNEL_8 (8)
#define ADC_CHANNEL_9 (9)

// ADC Sample Time
#define ADC_SAMPLE_1_5 (0)
#define ADC_SAMPLE_7_5 (1)
#define ADC_SAMPLE_13_5 (2)
#define ADC_SAMPLE_28_5 (3)
#define ADC_SAMPLE_41_5 (4)
#define ADC_SAMPLE_55_5 (5)
#define ADC_SAMPLE_71_5 (6)
#define ADC_SAMPLE_239_5 (7)

// ADC Continuous Conversion
#define ADC_CONT_CONV_OFF (0)
#define ADC_CONT_CONV_ON (1)

// ADC Data Alignment
#define ADC_DATA_ALIGN_RIGHT (0)
#define ADC_DATA_ALIGN_LEFT (1)

// ADC IRQ
#define ADCx_IRQ_DISABLE (0)
#define ADCx_IRQ_ENABLE (1)

// ADC Sample Time Channel Decoding
#define ADC_SAMPLE_CHANNEL(ADC_SAMPLEx) ((uint8_t)(((ADC_SAMPLEx) & 0xF0) >> 4))
// ADC Sample Time Decoding
#define ADC_SAMPLE_TIME(ADC_SAMPLEx) ((uint8_t)((ADC_SAMPLEx) & 0x0F))

/*----------------------------------------------- USART MACROS -----------------------------------------------*/
// TX
#define USARTx_TX_DISABLE (0x00)
#define USARTx_TX_ENABLE (0x01)

// RX
#define USARTx_RX_DISABLE (0x00)
#define USARTx_RX_ENABLE (0x01)

// TX Empty IRQ
#define USARTx_TXEIE_DISABLE (0x00)
#define USARTx_TXEIE_ENABLE (0x01)

// RX Not Empty IRQ
#define USARTx_RXNEIE_DISABLE (0x00)
#define USARTx_RXNEIE_ENABLE (0x01)

// TX Complete IRQ
#define USARTx_TCIE_DISABLE (0x00)
#define USARTx_TCIE_ENABLE (0x01)

// Parity Control
#define USARTx_PARITY_DISABLE (0x00)
#define USARTx_PARITY_ENABLE (0x01)

// Word Length
#define USARTx_WORD_8_BITS (0x00)
#define USARTx_WORD_9_BITS (0x01)

// Stop Bits
#define USARTx_STOP_1_BIT (0x00)
#define USARTx_STOP_0_5_BITS (0x01)
#define USARTx_STOP_2_BITS (0x02)
#define USARTx_STOP_1_5_BITS (0x03)

// Parity Mode
#define USARTx_PARITY_EVEN (0x00)
#define USARTx_PARITY_ODD (0x01)

// Baud Rate
#define USARTx_BAUD_4800 ((uint16_t)4800)
#define USARTx_BAUD_9600 ((uint16_t)9600)
#define USARTx_BAUD_115200 ((uint16_t)115200)

// Local Delay
#define USARTx_STRING_TX_DELAY ((uint16_t)3000)

/*----------------------------------------------- DMA MACROS -----------------------------------------------*/
// DMA Direction
#define DMAx_DIR_READ_PER (0x00)
#define DMAx_DIR_READ_MEM (0x01)

// Memory to Memory Transfer
#define DMAx_MEM2MEM_DISABLE (0x00)
#define DMAx_MEM2MEM_ENABLE (0x01)

// Circular Mode
#define DMAx_CIRC_DISABLE (0x00)
#define DMAx_CIRC_ENABLE (0x01)

// Priority Level
#define DMAx_PRIORITY_LOW (0x00)
#define DMAx_PRIORITY_MEDIUM (0x01)
#define DMAx_PRIORITY_HIGH (0x02)
#define DMAx_PRIORITY_VERY_HIGH (0x03)

// Address Increment
#define DMAx_INC_DISABLE (0x00)
#define DMAx_INC_ENABLE (0x01)

// DMA Data Size for Transfer
#define DMAx_DATA_SIZE_BIT_8 (0x00)
#define DMAx_DATA_SIZE_BIT_16 (0x01)
#define DMAx_DATA_SIZE_BIT_32 (0x02)

// Interrupts
#define DMAx_IRQ_DISABLE (0x00)
#define DMAx_IRQ_ENABLE (0x01)

// DMA Channel 6
#define DMA_USART2_RX (DMA1_Channel6)
#define DMA_I2C1_TX (DMA1_Channel6)
#define DMA_TIM1_CH3 (DMA1_Channel6)
#define DMA_TIM3_CH1 (DMA1_Channel6)
#define DMA_TIM3_TRIG (DMA1_Channel6)

// DMA Channel 7
#define DMA_USART2_TX (DMA1_Channel7)
#define DMA_I2C1_RX (DMA1_Channel7)
#define DMA_TIM2_CH2 (DMA1_Channel7)
#define DMA_TIM2_CH4 (DMA1_Channel7)
#define DMA_TIM4_UP (DMA1_Channel7)

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32F1XX_H_ */
