/**
 * @file	syscalls.c
 * @author	Shrey Shah
 * @brief	Adapts the linker-defined application heap to the C library
 * @version	v1.0
 * @date	22-08-2026
 *
 * @details
 * @section SYSCALLS_C_HIERARCHY Hierarchy
 * - Position: Platform adapter outside the application runtime layers
 * - Called by: C library allocation routines
 * - Uses: Linker-provided `_sheap` / `_eheap` symbols and Core shared types
 *
 * @section SYSCALLS_C_RESPONSIBILITY Responsibility
 * _sbrk() owns the current program break and admits allocations only inside
 * the heap interval defined by the linker script. The remaining stubs
 * (_close/_lseek/_read/_write/_fstat/_isatty/_exit/_kill/_getpid) satisfy
 * link-time references that may be pulled in by the C library. When
 * `APP_ENABLE_FLOAT` is enabled, this project's CMakeLists.txt adds
 * `-u _printf_float` for floating-point printf-family conversions under
 * `--specs=nano.specs`. None of this demo's code performs file I/O, so
 * every stub below is link-satisfying only and is never called.
 *
 * @section SYSCALLS_C_BOUNDARY Dependency Boundary
 * This module has no dependency on startup, application services, Core
 * registers, or peripheral Drivers.
 */

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include <errno.h>
#include <sys/stat.h>
#include "stm32f1xx_data_types.h"

// ==================================================================================================== //
// Linker Symbols
// ==================================================================================================== //

/** @brief First byte of the linker-defined application heap */
extern uint8_t _sheap;
/** @brief One-past-last byte of the linker-defined application heap */
extern uint8_t _eheap;

// ==================================================================================================== //
// C Library Declaration
// ==================================================================================================== //

void* _sbrk(intptr_t increment);
//! newlib declares only the _r reentrant wrappers in reent.h; the plain syscalls below have no
//! toolchain-provided prototype, so each is declared here immediately ahead of its definition.
int _close(int file);
int _lseek(int file, int offset, int whence);
int _read(int file, char* pBuffer, int length);
int _write(int file, const char* pBuffer, int length);
int _fstat(int file, struct stat* pStat);
int _isatty(int file);
int _kill(int pid, int signal);
int _getpid(void);
void _exit(int status);

// ==================================================================================================== //
// Private Data
// ==================================================================================================== //

/** @brief Current application heap boundary managed by _sbrk() */
static uint8_t* pHeapEnd = &_sheap;

// ==================================================================================================== //
// C Library Heap Adapter
// ==================================================================================================== //

/**
 * @brief Moves the application heap boundary for the C library allocator
 * @details
 * Applies a signed byte adjustment only when the resulting boundary remains
 * inside the linker-defined interval. Positive adjustments allocate upward;
 * negative adjustments release previously admitted heap space.
 * @param[in] increment Signed byte adjustment requested by the allocator
 * Expected values:
 * - Non-negative values no larger than the remaining heap capacity
 * - Negative values whose magnitude does not exceed the admitted heap growth
 * @returns Previous heap boundary on success or the C library failure sentinel
 * @retval - `&_sheap..&_eheap`: Previous heap boundary after an admitted adjustment
 * @retval - `(void*) -1`: The requested adjustment crossed a linker-defined
 * heap boundary and `errno` was set to `ENOMEM`
 * @warning This implementation is not reentrant; the application must serialize allocation across execution contexts
 * @warning Do not allocate memory from interrupt handlers
 */
void* _sbrk(intptr_t increment)
{
	const uintptr_t heapStartAddress = (uintptr_t) &_sheap;
	const uintptr_t heapLimitAddress = (uintptr_t) &_eheap;
	const uintptr_t currentHeapAddress = (uintptr_t) pHeapEnd;
	uintptr_t incrementMagnitude = 0UL;
	void* pPreviousHeapEnd = pHeapEnd;

	if (increment >= 0)
	{
		incrementMagnitude = (uintptr_t) increment;
		//! Reject upward growth before pointer arithmetic can cross the heap limit.
		if (incrementMagnitude > (heapLimitAddress - currentHeapAddress))
		{
			errno = ENOMEM;
			return (void*) -1;
		}

		//! Commit upward growth only after proving the complete adjustment fits.
		pHeapEnd += incrementMagnitude;
	}
	else
	{
		incrementMagnitude = ((uintptr_t) (-(increment + 1))) + 1UL;
		//! Admit heap contraction only while the boundary remains at or above its origin.
		if (incrementMagnitude > (currentHeapAddress - heapStartAddress))
		{
			errno = ENOMEM;
			return (void*) -1;
		}

		//! Commit contraction only after proving the original heap boundary is preserved.
		pHeapEnd -= incrementMagnitude;
	}

	return pPreviousHeapEnd;
}

// ==================================================================================================== //
// C Library File-Descriptor Stubs
// ==================================================================================================== //

/**
 * @brief Link-satisfying stub for the C library's close() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `-1` with `errno` set to `ENOSYS`
 */
int _close(int file)
{
	(void) file;
	errno = ENOSYS;
	return -1;
}

/**
 * @brief Link-satisfying stub for the C library's lseek() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `-1` with `errno` set to `ENOSYS`
 */
int _lseek(int file, int offset, int whence)
{
	(void) file;
	(void) offset;
	(void) whence;
	errno = ENOSYS;
	return -1;
}

/**
 * @brief Link-satisfying stub for the C library's read() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `-1` with `errno` set to `ENOSYS`
 */
int _read(int file, char* pBuffer, int length)
{
	(void) file;
	(void) pBuffer;
	(void) length;
	errno = ENOSYS;
	return -1;
}

/**
 * @brief Link-satisfying stub for the C library's write() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `-1` with `errno` set to `ENOSYS`
 */
int _write(int file, const char* pBuffer, int length)
{
	(void) file;
	(void) pBuffer;
	(void) length;
	errno = ENOSYS;
	return -1;
}

/**
 * @brief Link-satisfying stub for the C library's fstat() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `0` describing every file descriptor as a character device
 */
int _fstat(int file, struct stat* pStat)
{
	(void) file;
	pStat->st_mode = S_IFCHR;
	return 0;
}

/**
 * @brief Link-satisfying stub for the C library's isatty() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `0` (never a terminal)
 */
int _isatty(int file)
{
	(void) file;
	return 0;
}

/**
 * @brief Link-satisfying stub for the C library's kill() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `-1` with `errno` set to `ENOSYS`
 */
int _kill(int pid, int signal)
{
	(void) pid;
	(void) signal;
	errno = ENOSYS;
	return -1;
}

/**
 * @brief Link-satisfying stub for the C library's getpid() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * @returns `1`, the traditional bare-metal single-process identifier
 */
int _getpid(void)
{
	return 1;
}

/**
 * @brief Link-satisfying stub for the C library's exit() syscall
 * @details Never exercised at runtime; see this file's Responsibility section.
 * Halts instead of returning, matching `_exit()`'s no-return contract.
 * @returns Does not return
 */
void _exit(int status)
{
	(void) status;
	while (1)
	{
		//! Halt: there is no OS to return control to.
	}
}
