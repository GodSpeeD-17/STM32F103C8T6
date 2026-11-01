#!/bin/bash
# Regenerate compile_commands.json for VS Code IntelliSense
# Usage: ./regenerate_compile_commands.sh [project_name]
# If project_name is not provided, uses Template as default

PROJECT_NAME=${1:-Template}
PROJECT_DIR="/home/shrey_shah/STM32F103C8T6/Projects/${PROJECT_NAME}"
BUILD_DIR="${PROJECT_DIR}/build"

if [ ! -d "$PROJECT_DIR" ]; then
    echo "Error: Project directory $PROJECT_DIR does not exist"
    echo "Available projects:"
    ls -d /home/shrey_shah/STM32F103C8T6/Projects/*/ | xargs -n1 basename
    exit 1
fi

echo "Regenerating compile_commands.json for project: $PROJECT_NAME"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..

if [ -f "compile_commands.json" ]; then
    cp compile_commands.json /home/shrey_shah/STM32F103C8T6/compile_commands.json
    echo "Successfully updated compile_commands.json at workspace root"
    echo "Reload VS Code window (Ctrl+Shift+P -> 'Developer: Reload Window') to apply changes"
else
    echo "Error: compile_commands.json was not generated"
    exit 1
fi
