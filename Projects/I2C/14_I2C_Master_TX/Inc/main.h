/**
 * @file	main.h
 * @author	Shrey Shah
 * @brief	Declares the application control-loop entry point
 * @version	v1.0
 * @date	22-08-2026
 *
 * @details
 * @section MAIN_H_HIERARCHY Hierarchy
 * - Position: Layer 3 - Application behavior
 * - Invoked by: Layer 4 `app_startup` after initialization succeeds
 * - Uses: Layer 2 application services through `main.c`
 *
 * @section MAIN_H_RESPONSIBILITY Responsibility
 * This header declares the replaceable application entry point. Application
 * behavior belongs in `main.c`; startup and service initialization do not.
 *
 * @section MAIN_H_BOUNDARY Dependency Boundary
 * This header is standalone so startup does not acquire application-service or
 * hardware dependencies through `main.h`.
 */

// Header Guard
#ifndef MAIN_H_
#define MAIN_H_

// --- C++ Compatibility ---
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Template application control loop
 * @defgroup App_Main Application Control Loop
 * @ingroup App_Template
 * @{
 */

// ==================================================================================================== //
// Entry Point
// ==================================================================================================== //

/**
 * @brief Executes the application control loop
 * @details
 * Observes the monotonic application timebase for periodic non-blocking work
 * and sleeps between interrupts. The supplied Template implementation contains
 * an infinite loop.
 * @returns Application-defined exit status if the Template loop is replaced
 * @note The supplied Template implementation does not return
 */
int main(void);

/** @} */

// --- C++ Compatibility ---
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MAIN_H_ */
