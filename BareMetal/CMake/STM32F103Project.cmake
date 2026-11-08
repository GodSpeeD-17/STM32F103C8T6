include_guard(GLOBAL)

# Public project-facing entrypoints. `stm32f103_setup_toolchain()` selects the
# canonical toolchain file before `project(...)`, while
# `stm32f103_configure_project()` applies application policy afterward.

set(STM32F103_PROJECT_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

macro(stm32f103_setup_toolchain)
    set(options)
    set(one_value_args TOOLCHAIN_PATH)
    cmake_parse_arguments(STM32_TOOLCHAIN "${options}" "${one_value_args}" "" ${ARGN})

    # Preserve a command-line cache override, then let an explicit function
    # argument take final precedence over the workstation default.
    set(_toolchain_path "/opt/arm-gnu-toolchain-14.3/bin")
    if(DEFINED TOOLCHAIN_PATH AND NOT "${TOOLCHAIN_PATH}" STREQUAL "")
        set(_toolchain_path "${TOOLCHAIN_PATH}")
    endif()
    if(STM32_TOOLCHAIN_TOOLCHAIN_PATH)
        set(_toolchain_path "${STM32_TOOLCHAIN_TOOLCHAIN_PATH}")
    endif()
    set(TOOLCHAIN_PATH "${_toolchain_path}" CACHE PATH "Arm GNU toolchain binary directory" FORCE)

    # Select the standard CMake toolchain entrypoint before project() performs
    # compiler identification. Existing projects retain one simple bootstrap
    # call while tool selection is no longer implemented inside that call.
    set(
        CMAKE_TOOLCHAIN_FILE
        "${STM32F103_PROJECT_MODULE_DIR}/Toolchains/arm-none-eabi-gcc.cmake"
        CACHE FILEPATH
        "STM32F103 Arm GNU toolchain file"
        FORCE
    )
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compiler commands for tooling" FORCE)

    # An existing binary directory does not reload a newly selected toolchain
    # file after compiler identification. Mirror its non-compiler prefix state
    # so legacy Build directories can migrate without requiring deletion.
    set(CMAKE_SYSTEM_TOOLCHAIN_PREFIX "arm-none-eabi")
    set(TOOLCHAIN_PREFIX "${TOOLCHAIN_PATH}/${CMAKE_SYSTEM_TOOLCHAIN_PREFIX}")
endmacro()

macro(stm32f103_configure_project)
    set(options)
    set(one_value_args
        BUILD_OUTPUT_DIR
        DRIVER_MODULE_TREE
        OPTIMIZATION_LEVEL
        FLASH_ADDRESS
        LINKER_FILE
        SAVE_TEMPS
        ST_FLASH_PATH
        ST_UTIL_PATH
        OPENOCD_PATH
        GDB_PATH_MULTIARCH_PATH
        ST_UART_FLASH_PORT
    )
    set(multi_value_args DRIVER_MODULES)
    cmake_parse_arguments(STM32 "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT STM32_DRIVER_MODULES)
        message(FATAL_ERROR "stm32f103_configure_project() requires at least one DRIVER_MODULES entry")
    endif()

    # Resolve the repository layout once so every downstream module can rely
    # on the same canonical paths.
    get_filename_component(PROJ_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
    get_filename_component(BAREMETAL_ROOT "${STM32F103_PROJECT_MODULE_DIR}/.." ABSOLUTE)
    set(STM32_CMAKE_ROOT "${BAREMETAL_ROOT}/CMake")
    get_filename_component(CORE_ROOT "${BAREMETAL_ROOT}/Core" ABSOLUTE)
    get_filename_component(REPO_ROOT "${BAREMETAL_ROOT}/.." ABSOLUTE)
    set(DRIVER_ROOT "${BAREMETAL_ROOT}/Driver")

    # Everything below remains project-specific on purpose. These are the
    # knobs a project may reasonably change for bootloaders, custom linkers,
    # alternate tool paths, or different output locations.
    set(_build_output_dir "${CMAKE_CURRENT_BINARY_DIR}/Artifacts")
    if(STM32_BUILD_OUTPUT_DIR)
        set(_build_output_dir "${STM32_BUILD_OUTPUT_DIR}")
    endif()
    set(BUILD_OUTPUT_DIR "${_build_output_dir}" CACHE PATH "Build artifact output directory")

    set(_driver_module_tree "SHOW_FILENAME_ONLY")
    if(STM32_DRIVER_MODULE_TREE)
        set(_driver_module_tree "${STM32_DRIVER_MODULE_TREE}")
    endif()
    set(DRIVER_MODULE_TREE "${_driver_module_tree}")

    set(_optimization_level "1")
    if(DEFINED STM32_OPTIMIZATION_LEVEL AND NOT "${STM32_OPTIMIZATION_LEVEL}" STREQUAL "")
        set(_optimization_level "${STM32_OPTIMIZATION_LEVEL}")
    endif()
    set(OPTIMIZATION_LEVEL "${_optimization_level}")

    set(_flash_address "0x08000000")
    if(STM32_FLASH_ADDRESS)
        set(_flash_address "${STM32_FLASH_ADDRESS}")
    endif()
    set(FLASH_ADDRESS "${_flash_address}")

    set(_linker_file "${CORE_ROOT}/Src/stm32f103c8t6.ld")
    if(STM32_LINKER_FILE)
        set(_linker_file "${STM32_LINKER_FILE}")
    endif()
    set(LINKER_FILE "${_linker_file}" CACHE FILEPATH "Linker script for the target MCU")

    set(_save_temps OFF)
    if(DEFINED STM32_SAVE_TEMPS)
        set(_save_temps "${STM32_SAVE_TEMPS}")
    endif()
    set(SAVE_TEMPS "${_save_temps}")

    set(_st_flash_path "/usr/bin/st-flash")
    if(STM32_ST_FLASH_PATH)
        set(_st_flash_path "${STM32_ST_FLASH_PATH}")
    endif()
    set(ST_FLASH_PATH "${_st_flash_path}" CACHE FILEPATH "Path to st-flash")

    set(_st_util_path "/usr/bin/st-util")
    if(STM32_ST_UTIL_PATH)
        set(_st_util_path "${STM32_ST_UTIL_PATH}")
    endif()
    set(ST_UTIL_PATH "${_st_util_path}" CACHE FILEPATH "Path to st-util")

    set(_openocd_path "/usr/bin/openocd")
    if(STM32_OPENOCD_PATH)
        set(_openocd_path "${STM32_OPENOCD_PATH}")
    endif()
    set(OPENOCD_PATH "${_openocd_path}" CACHE FILEPATH "Path to OpenOCD")

    set(_gdb_path "/usr/bin/gdb-multiarch")
    if(STM32_GDB_PATH_MULTIARCH_PATH)
        set(_gdb_path "${STM32_GDB_PATH_MULTIARCH_PATH}")
    endif()
    set(GDB_PATH_MULTIARCH_PATH "${_gdb_path}" CACHE FILEPATH "Path to gdb-multiarch")

    set(_uart_port "/dev/ttyUSB0")
    if(STM32_ST_UART_FLASH_PORT)
        set(_uart_port "${STM32_ST_UART_FLASH_PORT}")
    endif()
    set(ST_UART_FLASH_PORT "${_uart_port}" CACHE STRING "UART device for stm32flash")

    set(DRIVER_MODULES ${STM32_DRIVER_MODULES})

    # Hand off to the shared build pipeline once the project config is ready.
    include("${STM32_CMAKE_ROOT}/CMake_Template.cmake")
endmacro()
