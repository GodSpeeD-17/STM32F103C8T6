include_guard(GLOBAL)

# Non-firmware helper targets live here: VS Code metadata generation, flashing,
# erase helpers, and repository maintenance shortcuts.

function(stm32_add_vscode_targets)
    # Materialize valid JSON from reviewed templates inside the binary tree.
    # @ONLY substitution preserves VS Code tokens such as the $gcc matcher.
    file(MAKE_DIRECTORY "${VSCODE_GENERATED_DIR}")
    configure_file(
        "${STM32_CMAKE_ROOT}/Templates/launch.json.in"
        "${VSCODE_GENERATED_DIR}/launch.json"
        @ONLY
    )
    configure_file(
        "${STM32_CMAKE_ROOT}/Templates/settings.json.in"
        "${VSCODE_GENERATED_DIR}/settings.json"
        @ONLY
    )
    configure_file(
        "${STM32_CMAKE_ROOT}/Templates/tasks.json.in"
        "${VSCODE_GENERATED_DIR}/tasks.json"
        @ONLY
    )
    configure_file(
        "${STM32_CMAKE_ROOT}/Templates/c_cpp_properties.json.in"
        "${VSCODE_GENERATED_DIR}/c_cpp_properties.json"
        @ONLY
    )

    # The source-tree .vscode folder is materialized at configure time so a
    # fresh project is immediately debuggable: VS Code reads only .vscode/ at
    # the workspace root, never the Build/VSCode staging folder.
    file(MAKE_DIRECTORY "${VSCODE_DIR}")
    file(COPY
        "${VSCODE_GENERATED_DIR}/launch.json"
        "${VSCODE_GENERATED_DIR}/settings.json"
        "${VSCODE_GENERATED_DIR}/tasks.json"
        "${VSCODE_GENERATED_DIR}/c_cpp_properties.json"
        DESTINATION "${VSCODE_DIR}"
    )

    # Copy debugger metadata refreshes every reconfigure, keeping ambiguous
    # manual sync steps out of the editor workflow.
    add_custom_target(vscode_launch
        COMMAND ${CMAKE_COMMAND} -E make_directory "${VSCODE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VSCODE_GENERATED_DIR}/launch.json"
            "${VSCODE_DIR}/launch.json"
        COMMENT "Generating launch.json"
    )

    # Settings include the active compile database, making IntelliSense consume
    # the same definitions, include paths, and flags as the real compiler.
    add_custom_target(vscode_settings
        COMMAND ${CMAKE_COMMAND} -E make_directory "${VSCODE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VSCODE_GENERATED_DIR}/settings.json"
            "${VSCODE_DIR}/settings.json"
        COMMENT "Generating settings.json"
    )

    # Keep VS Code tasks aligned with the same CMake build entrypoints the
    # terminal workflow already uses.
    add_custom_target(vscode_tasks
        COMMAND ${CMAKE_COMMAND} -E make_directory "${VSCODE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VSCODE_GENERATED_DIR}/tasks.json"
            "${VSCODE_DIR}/tasks.json"
        COMMENT "Generating tasks.json"
    )

    # c_cpp_properties points to compile_commands.json instead of duplicating
    # include paths and macros that would drift from target configuration.
    add_custom_target(vscode_c_cpp_properties
        COMMAND ${CMAKE_COMMAND} -E make_directory "${VSCODE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VSCODE_GENERATED_DIR}/c_cpp_properties.json"
            "${VSCODE_DIR}/c_cpp_properties.json"
        COMMENT "Generating c_cpp_properties.json"
    )

    add_custom_target(vscode_files
        DEPENDS
            vscode_launch
            vscode_settings
            vscode_tasks
            vscode_c_cpp_properties
        COMMENT "----------------------------------- Generating vscode files -----------------------------------"
    )

    add_custom_target(debug
        DEPENDS ${PROJECT_NAME}.elf vscode_files
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Debug environment ready."
        COMMENT "Setting up debug environment: building project and generating VSCode configs"
    )
endfunction()

function(stm32_add_flash_targets)
    # `debug_flash` is a dry-run helper that prints the resolved flash command
    # without touching hardware.
    add_custom_target(debug_flash
        COMMAND ${CMAKE_COMMAND} -E echo "ST_FLASH: ${ST_FLASH}"
        COMMAND ${CMAKE_COMMAND} -E echo "BINARY_FILE: ${BINARY_FILE}"
        COMMAND ${CMAKE_COMMAND} -E echo "FLASH_ADDRESS: ${FLASH_ADDRESS}"
        COMMAND ${CMAKE_COMMAND} -E echo "Full command: ${ST_FLASH} --reset write ${BINARY_FILE} ${FLASH_ADDRESS}"
    )

    # `flash` and `erase_flash` delegate to helper scripts so they can retry
    # with connect-under-reset when the target cannot be attached normally.
    add_custom_target(flash
        DEPENDS ${PROJECT_NAME}.elf
        COMMAND ${CMAKE_COMMAND}
            -DST_FLASH=${ST_FLASH}
            -DBINARY_FILE=${BINARY_FILE}
            -DFLASH_ADDRESS=${FLASH_ADDRESS}
            -P ${STM32_CMAKE_ROOT}/STM32F103Flash.cmake
        COMMENT "Flashing ${PROJECT_NAME}.bin to STM32 via ST-Link v2 (SWD) with automatic recovery"
    )

    add_custom_target(flash_uart
        DEPENDS ${PROJECT_NAME}.elf
        COMMAND sudo ${ST_UART_FLASH} -w ${BINARY_FILE} -v -g 0x0 ${ST_UART_FLASH_PORT}
        COMMENT "Flashing ${PROJECT_NAME}.bin to STM32 via UART bootloader"
    )

    add_custom_target(info
        COMMAND ${ST_INFO} --probe
        COMMENT "Show connected ST-Link probe information"
    )

    add_custom_target(erase_flash
        COMMAND ${CMAKE_COMMAND}
            -DST_FLASH=${ST_FLASH}
            -P ${STM32_CMAKE_ROOT}/STM32F103EraseFlash.cmake
        COMMENT "Erasing entire STM32 flash memory via ST-Link with automatic recovery"
    )

    add_custom_target(erase_flash_uart
        COMMAND sudo ${ST_UART_FLASH} -o ${ST_UART_FLASH_PORT}
        COMMENT "Erasing STM32 flash memory via UART bootloader"
    )

endfunction()

function(stm32_add_maintenance_targets)
    # `clean_all` intentionally removes both generated firmware artifacts and
    # generated editor metadata to bring the project back to a clean state.
    add_custom_target(clean_all
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${BUILD_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${VSCODE_DIR}
        COMMENT "Completely cleaning project: build artifacts and VSCode configurations"
    )

    # Repository-wide sync helpers are convenience targets for template-style
    # files that are intentionally kept identical across multiple projects.
    function(add_replace_target TARGET_NAME FILE_PATH IN_PROJECT_DIR)
        add_custom_target(${TARGET_NAME}
            COMMAND ${CMAKE_COMMAND} -E echo "Replacing ${FILE_PATH} in all projects"
            COMMAND find ${REPO_ROOT}/Projects -name "${FILE_PATH}" -not -path "${IN_PROJECT_DIR}/${FILE_PATH}" -print0 | xargs -0 -I {} cp "${IN_PROJECT_DIR}/${FILE_PATH}" "{}"
        )
    endfunction()

    add_replace_target(replace_main_h "main.h" "${PROJ_DIR}/Inc")
    add_replace_target(replace_app_startup_h "app_startup.h" "${PROJ_DIR}/Inc")
    add_replace_target(replace_app_startup_c "app_startup.c" "${PROJ_DIR}/Src")
    add_replace_target(replace_cmake "CMakeLists.txt" "${PROJ_DIR}")
endfunction()
