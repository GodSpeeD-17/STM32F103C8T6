/**
 * @file	app_startup.h
 * @author	Shrey Shah
 * @brief	Declares the application-owned processor startup contract
 * @version	v1.0
 * @date	22-08-2026
 *
 * @details
 * @section APP_STARTUP_H_HIERARCHY Hierarchy
 * - Position: Layer 4 - Processor entry
 * - Invoked by: Processor reset and exception vectors
 * - Uses: Layer 3 `app_init` and `main` through `app_startup.c`
 *
 * @section APP_STARTUP_H_RESPONSIBILITY Responsibility
 * This contract declares linker symbols plus the reset and fallback entry
 * points. Its source owns the vector table, initializes C runtime memory,
 * calls App_Init(), and transfers control to main().
 *
 * @section APP_STARTUP_H_BOUNDARY Dependency Boundary
 * This header includes only `stm32f1xx_data_types.h`. It owns no peripheral
 * policy, heap adaptation, or application service, and no lower layer includes
 * it.
 */

// Header Guard
#ifndef APP_STARTUP_H_
#define APP_STARTUP_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "stm32f1xx_data_types.h"

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Application-owned processor startup contract
 * @defgroup App_Startup Application Startup
 * @ingroup App_Template
 * @{
 */

// ==================================================================================================== //
// Linker Symbols
// ==================================================================================================== //

/** @brief Flash load address of the initialized `.data` image supplied by the linker */
extern uint32_t _sidata;
/** @brief First `.data` address in RAM supplied by the linker */
extern uint32_t _sdata;
/** @brief One-past-last `.data` address in RAM supplied by the linker */
extern uint32_t _edata;
/** @brief First `.bss` address in RAM supplied by the linker */
extern uint32_t _sbss;
/** @brief One-past-last `.bss` address in RAM supplied by the linker */
extern uint32_t _ebss;
/** @brief Initial main-stack address supplied by the linker */
extern uint32_t _estack;

// ==================================================================================================== //
// Processor Entry Points
// ==================================================================================================== //

/**
 * @brief Initializes C runtime memory and transfers control to the application
 * @details
 * Copies `.data` from its Flash load image into RAM, clears `.bss`, invokes
 * @ref App_Init, then transfers control to @ref main. Any initialization
 * failure or unexpected return from @ref main enters @ref Default_Handler.
 * @returns Nothing
 * @pre The linker provides valid ordered `_sidata`, `_sdata`, `_edata`, `_sbss`, `_ebss`, and `_estack` symbols
 * @note This processor reset entry point is the second vector-table entry and never returns
 */
__attribute__((noreturn)) void Reset_Handler(void);

/**
 * @brief Contains execution after an unimplemented exception or fatal startup error
 * @details
 * Repeatedly executes `WFI` so the processor remains contained without a busy
 * loop. The handler performs no peripheral cleanup and publishes no recovery
 * state.
 * @returns Nothing
 * @note This fallback entry point never returns
 * @warning Attach a debugger and inspect the active exception state to identify the originating fault
 */
void Default_Handler(void);

/** @} */

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_STARTUP_H_ */
