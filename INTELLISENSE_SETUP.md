# VS Code IntelliSense Setup

This project uses `compile_commands.json` for accurate IntelliSense and Go-to-Definition functionality.

## How It Works

- The CMake build system generates `compile_commands.json` which contains exact compiler flags, include paths, and preprocessor definitions for every source file.
- VS Code uses this file to provide accurate code navigation, autocomplete, and syntax highlighting.

## Initial Setup

The setup has been completed for you:
1. `compile_commands.json` has been generated from the Template project
2. `.vscode/c_cpp_properties.json` has been configured to use it
3. A helper script `regenerate_compile_commands.sh` has been created

## Regenerating compile_commands.json

When you add new files, change include paths, or modify the build configuration, regenerate the database:

```bash
# For the Template project (default)
./regenerate_compile_commands.sh

# For a specific project
./regenerate_compile_commands.sh USART/08_USART_Byte_Poll_TX
```

Then reload VS Code:
- Press `Ctrl+Shift+P`
- Type "Developer: Reload Window"
- Press Enter

## Manual Regeneration

If you prefer to do it manually:

```bash
cd Projects/YourProject/build
cmake ..
cp compile_commands.json /home/shrey_shah/STM32F103C8T6/compile_commands.json
```

## Troubleshooting

**IntelliSense still not working after reload:**
1. Check that `compile_commands.json` exists at the workspace root
2. Open VS Code settings (Ctrl+,) and search for "C/C++: IntelliSense Engine"
3. Ensure it's set to "Default" (not "Tag Parser")
4. Check the C/C++ output panel for errors

**Missing headers for a specific project:**
- Regenerate using that project: `./regenerate_compile_commands.sh YourProjectName`
- The generated database includes all headers referenced by that project

**Go-to-Definition still not working:**
- Ensure the file you're trying to navigate from is included in the compile database
- Try holding `Ctrl` and clicking the identifier
- If it fails, regenerate the database for a project that includes that file
