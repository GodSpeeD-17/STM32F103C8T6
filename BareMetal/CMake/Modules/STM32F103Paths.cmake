include_guard(GLOBAL)

# Output-path preparation is isolated here so configure-time directory creation
# and artifact naming remain consistent across all projects.

function(stm32_configure_output_paths)
    set(VSCODE_DIR ${PROJ_DIR}/.vscode)
    set(VSCODE_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/VSCode)
    if(EXISTS ${VSCODE_DIR})
        set(vscode_dir_status "existing")
    else()
        # stm32_add_vscode_targets() materializes .vscode later in the same
        # configure pass, so no editor metadata step is left to manual setup.
        set(vscode_dir_status "materialized by configure")
    endif()

    set(BINARY_FILE ${BUILD_OUTPUT_DIR}/${PROJECT_NAME}.bin)
    set(HEX_FILE ${BUILD_OUTPUT_DIR}/${PROJECT_NAME}.hex)

    if(NOT EXISTS ${BUILD_OUTPUT_DIR})
        file(MAKE_DIRECTORY ${BUILD_OUTPUT_DIR})
        set(build_dir_status "created")
    else()
        set(build_dir_status "existing")
    endif()

    # Force all generated artifacts into the same Build folder instead of
    # mixing generator files with firmware outputs.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR} PARENT_SCOPE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR} PARENT_SCOPE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR} PARENT_SCOPE)
    set(VSCODE_DIR "${VSCODE_DIR}" PARENT_SCOPE)
    set(VSCODE_GENERATED_DIR "${VSCODE_GENERATED_DIR}" PARENT_SCOPE)
    set(VSCODE_DIR_STATUS "${vscode_dir_status}" PARENT_SCOPE)
    set(BUILD_DIR_STATUS "${build_dir_status}" PARENT_SCOPE)
    set(BINARY_FILE "${BINARY_FILE}" PARENT_SCOPE)
    set(HEX_FILE "${HEX_FILE}" PARENT_SCOPE)
endfunction()
