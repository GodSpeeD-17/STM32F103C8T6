# This file is the complete build entrypoint copied with every application.
# It owns application identity, feature selection, and the list of shared
# Drivers consumed by that application. Compiler flags, Core sources, linker
# behavior, utility targets, and artifact generation remain centralized in
# BareMetal/CMake so copied projects do not duplicate platform policy.
# Repository-wide application build-policy changes originate and are verified
# here before being propagated to existing projects; numbered examples are not
# templates for new applications.
# CMake grammar requires each command name and its opening parenthesis to share
# one line; the repository's multi-line C function-call layout does not apply.

# ====================================================================================================
# CMake Compatibility
# ====================================================================================================

# Require the CMake version against which the shared STM32F103 pipeline is
# maintained. Rejecting older versions avoids partially supported behavior.
cmake_minimum_required(VERSION 3.28)

# ====================================================================================================
# Shared Repository Discovery
# ====================================================================================================

# External projects may explicitly identify the repository containing the
# shared BareMetal platform. An empty value preserves zero-configuration use
# for projects stored anywhere below this repository root.
set(STM32F103_REPO_ROOT "" CACHE PATH "STM32F103C8T6 repository root containing BareMetal")

if(STM32F103_REPO_ROOT)
    # Normalize an explicit absolute or project-relative override before it is
    # validated and consumed by the shared build entrypoint.
    get_filename_component(
        _stm32_repo_root
        "${STM32F103_REPO_ROOT}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_LIST_DIR}"
    )
else()
    # Walk upward from the copied project until the shared build entrypoint is
    # found. This permits arbitrary project nesting within the repository.
    set(_stm32_repo_root "${CMAKE_CURRENT_LIST_DIR}")
    while(NOT EXISTS "${_stm32_repo_root}/BareMetal/CMake/STM32F103Project.cmake")
        get_filename_component(_stm32_repo_parent "${_stm32_repo_root}" DIRECTORY)
        # Stop at the filesystem root so a missing repository fails cleanly
        # through the common validation below instead of looping forever.
        if(_stm32_repo_parent STREQUAL _stm32_repo_root)
            break()
        endif()
        set(_stm32_repo_root "${_stm32_repo_parent}")
    endwhile()
endif()

# Validate the one shared entrypoint required by both discovery paths. The
# diagnostic also tells an external application how to repair its configuration.
if(NOT EXISTS "${_stm32_repo_root}/BareMetal/CMake/STM32F103Project.cmake")
    message(
        FATAL_ERROR
        "Unable to locate BareMetal/CMake/STM32F103Project.cmake. "
        "Set STM32F103_REPO_ROOT to the STM32F103C8T6 repository root."
    )
endif()

# Import only the public project-configuration interface. That interface owns
# all lower shared CMake modules and prevents application-level include chains.
include("${_stm32_repo_root}/BareMetal/CMake/STM32F103Project.cmake")

# ====================================================================================================
# Toolchain and Application Identity
# ====================================================================================================

# Select the Arm cross-toolchain before project() allows CMake to probe a host
# compiler. The shared function also enables compile_commands.json generation.
stm32f103_setup_toolchain()

# Derive the application and firmware-target name from the copied directory.
# Renaming the project directory therefore renames its ELF/BIN/HEX/MAP outputs.
# C, C++, and assembly are enabled up front so adding a C++ application source
# does not require restructuring this build entrypoint.
get_filename_component(_stm32_project_name "${CMAKE_CURRENT_LIST_DIR}" NAME)
project(${_stm32_project_name} VERSION 1.0.0 LANGUAGES C CXX ASM)

# ====================================================================================================
# Application Feature Selection
# ====================================================================================================

# These cache options are the application-facing hardware-service policy. They
# control both shared Driver inclusion below and conditional Template code
# through the compile definitions published at the end of this file.
set(APP_TIMEBASE_SOURCE "SYSTICK" CACHE STRING "Application timebase source: OFF, SYSTICK, TIM2, TIM3, or TIM4")
set_property(CACHE APP_TIMEBASE_SOURCE PROPERTY STRINGS OFF SYSTICK TIM2 TIM3 TIM4)
option(APP_ENABLE_TIMER_US_DELAY "Enable the optional TIM4 microsecond delay service" OFF)
option(APP_ENABLE_ONBOARD_LED "Enable the optional BSP on-board LED initialization" ON)
option(APP_ENABLE_DEBUG "Enable optional BSP USART debug transport initialization" OFF)
option(APP_ENABLE_FLOAT "Enable newlib-nano floating-point printf/scanf support (-u _printf_float)" OFF)

# Normalize the human-readable selector once so module resolution and C
# preprocessing consume the same application timebase decision.
string(TOUPPER "${APP_TIMEBASE_SOURCE}" _app_timebase_source)
if(_app_timebase_source STREQUAL "OFF")
    set(_app_timebase_source_define APP_TIME_SOURCE_NONE)
elseif(_app_timebase_source STREQUAL "SYSTICK")
    set(_app_timebase_source_define APP_TIME_SOURCE_SYSTICK)
elseif(_app_timebase_source STREQUAL "TIM2")
    set(_app_timebase_source_define APP_TIME_SOURCE_TIM2)
elseif(_app_timebase_source STREQUAL "TIM3")
    set(_app_timebase_source_define APP_TIME_SOURCE_TIM3)
elseif(_app_timebase_source STREQUAL "TIM4")
    set(_app_timebase_source_define APP_TIME_SOURCE_TIM4)
else()
    message(FATAL_ERROR "APP_TIMEBASE_SOURCE must be OFF, SYSTICK, TIM2, TIM3, or TIM4.")
endif()

if(APP_ENABLE_TIMER_US_DELAY AND _app_timebase_source STREQUAL "TIM4")
    message(FATAL_ERROR "APP_TIMEBASE_SOURCE=TIM4 conflicts with APP_ENABLE_TIMER_US_DELAY=ON because BSP_TIMER exclusively reserves TIM4.")
endif()

# RCC is required by the base clock initialization path. Optional services add
# only their owning Driver module; disabled peripherals contribute no sources
# or public Driver include directory to this application target. BSP_GPIO resolves
# GPIO -> RCC automatically through the shared component graph.
set(app_driver_modules RCC I2C)
if(APP_ENABLE_DEBUG)
    list(APPEND app_driver_modules BSP_USART)
endif()
if(_app_timebase_source STREQUAL "SYSTICK")
    list(APPEND app_driver_modules SysTick)
elseif(_app_timebase_source MATCHES "^TIM[234]$")
    list(APPEND app_driver_modules Timer NVIC)
endif()
if(APP_ENABLE_TIMER_US_DELAY)
    list(APPEND app_driver_modules BSP_TIMER)
endif()
if(APP_ENABLE_ONBOARD_LED)
    list(APPEND app_driver_modules BSP_GPIO)
endif()

# ====================================================================================================
# Shared Firmware Build Configuration
# ====================================================================================================

# Hand application policy to the shared pipeline. It discovers project sources,
# adds the selected Driver stacks, applies the common Cortex-M3 flags and linker
# script, and generates firmware plus utility targets. CMake metadata remains in
# the selected binary directory; final firmware files are grouped in Artifacts.
stm32f103_configure_project(
    # Keep deployable outputs separate from CMake cache and generator files.
    BUILD_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Artifacts"
    # Print selected Driver filenames during configuration without full paths.
    DRIVER_MODULE_TREE SHOW_FILENAME_ONLY
    # Compile and expose exactly the shared Driver stacks selected above.
    DRIVER_MODULES
        ${app_driver_modules}
    # Build with -O0 while hardware-debugging this application so every C
    # variable and register-read sequence stays inspectable in the debugger.
    OPTIMIZATION_LEVEL 2
    # Link the application at the STM32F103C8T6 internal-Flash base address.
    FLASH_ADDRESS 0x08000000
)

# Publish normalized 0/1 feature states only to this firmware target. Template
# sources and app_config.h consume the same values used for Driver selection,
# keeping compiled application APIs synchronized with linked Driver modules.
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    APP_TIMEBASE_SOURCE=${_app_timebase_source_define}
    APP_ENABLE_TIMER_US_DELAY=$<BOOL:${APP_ENABLE_TIMER_US_DELAY}>
    APP_ENABLE_ONBOARD_LED=$<BOOL:${APP_ENABLE_ONBOARD_LED}>
    APP_ENABLE_DEBUG=$<BOOL:${APP_ENABLE_DEBUG}>
    APP_ENABLE_FLOAT=$<BOOL:${APP_ENABLE_FLOAT}>
)

# The shared --specs=nano.specs link policy strips floating-point printf
# support by default. APP_ENABLE_FLOAT controls whether this target
# pulls the newlib-nano float formatter in via -u _printf_float, scoped to
# this one target rather than widening the shared STM32F103Flags.cmake link
# policy for every project.
if(APP_ENABLE_FLOAT)
    target_link_options(${PROJECT_NAME}.elf PRIVATE -u _printf_float)
endif()
