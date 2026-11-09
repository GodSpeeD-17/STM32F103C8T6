# CMake Build Architecture

## Purpose

This document explains how the STM32F103C8T6 build system is organized, why it
was reorganized, how a project uses it, and what has been validated so far. It
is written for readers who know how to compile a C project but do not need prior
knowledge of this repository or advanced CMake.

The central design rule is:

> An application declares what it needs; the shared CMake pipeline decides how
> those capabilities are compiled, connected, linked, inspected, and deployed.

Consequently, a project `CMakeLists.txt` owns application-specific choices such
as its name, enabled services, direct Driver capabilities, optimization level,
and Flash origin. It does not duplicate compiler flags, Core paths, Driver
source lists, linker behavior, artifact generation, or development-tool setup.

## What Changed

The earlier build model treated application, Core, and Driver sources as one
flattened source collection. Projects had to know more implementation details,
include paths could leak across unrelated modules, and editor configuration
could disagree with the real compiler configuration.

The current model introduces the following boundaries:

- one public project-facing CMake interface;
- one canonical Arm GNU toolchain file;
- one shared build-policy target;
- one target for Core;
- one target per selected Driver module;
- automatic resolution of direct Driver dependencies;
- one final application ELF target;
- generated deployment artifacts, memory reporting, VS Code metadata, and
  hardware utility targets derived from the same configuration.

This is target-oriented CMake: sources, include directories, compile options,
and dependencies belong to the component that owns them.

## Newcomer Mental Model

The complete architecture can be understood as four stages:

```text
Project CMakeLists.txt
  declares application identity and requested capabilities
            |
            v
STM32F103Project.cmake
  resolves project policy and exposes the public CMake API
            |
            v
CMake_Template.cmake + Modules/*.cmake
  validates tools/paths, creates component targets, and wires the build
            |
            v
<Project>.elf -> <Project>.bin / <Project>.hex / <Project>.map
  plus memory report, flash/debug targets, and optional VS Code files
```

An application should normally interact with only two shared functions:

```cmake
stm32f103_setup_toolchain()
project(...)
stm32f103_configure_project(...)
```

Everything below those calls is shared platform implementation.

## Repository Layout

```text
BareMetal/CMake
├── STM32F103Project.cmake             Public project-facing API
├── CMake_Template.cmake               Ordered shared pipeline
├── STM32F103Diagnostics.cmake         Configure-time summaries and validation helpers
├── STM32F103Flash.cmake               ST-Link flash transaction and recovery
├── STM32F103EraseFlash.cmake          ST-Link erase transaction and recovery
├── STM32F103MemoryReport.cmake        Post-link Flash/RAM report
├── Modules
│   ├── STM32F103Validation.cmake      Required-input validation and defaults
│   ├── STM32F103Tooling.cmake         Tool discovery and path validation
│   ├── STM32F103Flags.cmake           Target-owned compile/link policy
│   ├── STM32F103Paths.cmake           Output and generated-file paths
│   ├── STM32F103Sources.cmake         Core/Driver graph and source ownership
│   ├── STM32F103Targets.cmake         Final firmware target and artifact generation
│   └── STM32F103UtilityTargets.cmake  VS Code, flash, debug, and maintenance targets
├── Templates
│   ├── c_cpp_properties.json.in
│   ├── launch.json.in
│   ├── settings.json.in
│   └── tasks.json.in
└── Toolchains
    └── arm-none-eabi-gcc.cmake        Cross-compiler selection
```

## Public Project Contract

### Repository discovery

A project inside this repository walks upward until it finds:

```text
BareMetal/CMake/STM32F103Project.cmake
```

This permits projects to be nested at different depths without hard-coding a
fixed number of parent directories.

A project copied outside the repository must identify the shared repository
root explicitly:

```bash
cmake -S . -B Build \
    -DSTM32F103_REPO_ROOT=/absolute/path/to/STM32F103C8T6
```

The supplied path must be the directory containing `BareMetal`.

### Toolchain setup

`stm32f103_setup_toolchain()` must execute before `project(...)`. It:

- selects `Toolchains/arm-none-eabi-gcc.cmake`;
- establishes the `arm-none-eabi` compiler and binutils family;
- defaults the toolchain directory to
  `/opt/arm-gnu-toolchain-14.3/bin`;
- permits a `TOOLCHAIN_PATH` cache override or function argument; and
- enables `compile_commands.json` generation for editor tooling.

The toolchain file selects tools only. MCU flags and language policy remain
target-owned in `STM32F103Flags.cmake`.

### Project configuration

`stm32f103_configure_project()` executes after `project(...)`. Its most
important inputs are:

| Input | Meaning | Current default |
|---|---|---|
| `DRIVER_MODULES` | Direct capabilities required by the application | Required |
| `BUILD_OUTPUT_DIR` | Destination for deployable artifacts | `Build/Artifacts` |
| `DRIVER_MODULE_TREE` | Filename-only or full-path configure summary | `SHOW_FILENAME_ONLY` |
| `OPTIMIZATION_LEVEL` | Value appended to `-O` | `1` |
| `FLASH_ADDRESS` | Address used by the flash target | `0x08000000` |
| `LINKER_FILE` | Linker script for the firmware image | Core STM32F103C8T6 script |
| `SAVE_TEMPS` | Enables compiler intermediate-file retention | `OFF` |
| `ST_FLASH_PATH` | Path to the ST-Link flash utility | `/usr/bin/st-flash` |
| `ST_UTIL_PATH` | Path to the ST-Link GDB server | `/usr/bin/st-util` |
| `OPENOCD_PATH` | Path to OpenOCD | `/usr/bin/openocd` |
| `GDB_PATH_MULTIARCH_PATH` | Path to the debugger client | `/usr/bin/gdb-multiarch` |
| `ST_UART_FLASH_PORT` | UART bootloader device | `/dev/ttyUSB0` |

The function resolves canonical project, repository, Core, Driver, CMake, and
linker paths before entering the shared pipeline.

## Ordered Configure Pipeline

`CMake_Template.cmake` deliberately keeps configuration order visible:

```text
1. Validate required project inputs
2. Apply defaults and derive binutils names
3. Validate and print repository/tool paths
4. Configure host-side utilities
5. Create target-owned compile and link policy
6. Configure artifact and VS Code paths
7. Print the resolved build configuration
8. Discover application/Core/Driver sources
9. Resolve and register the Driver dependency graph
10. Create the final firmware target
11. Register VS Code targets
12. Register flash, debug, erase, and maintenance targets
```

Failure therefore occurs early for a missing path, tool, linker script, Driver
module, or application source directory instead of appearing as an obscure
compiler or linker error later.

## CMake Target Graph

The generated target relationships are:

```text
stm32::build_options (INTERFACE)
          |
          v
stm32::core (INTERFACE today; OBJECT if Core gains sources)
          |
          +----------------------+----------------------+
          v                      v                      v
stm32_driver_RCC         stm32_driver_GPIO      other selected Drivers
   (OBJECT)                  (OBJECT)            (OBJECT or INTERFACE)
          \                      |                      /
           \______ resolved Driver relationships _____/
                              |
                              v
                     <Project_Name>.elf
                              |
                              v
                    ELF / BIN / HEX / MAP
```

### Shared build-policy target

`stm32::build_options` is an alias for one `INTERFACE` target containing the
requirements shared by every compiled component:

- Cortex-M3, Thumb, and software floating-point architecture flags;
- project-selected optimization;
- warnings and debug information;
- function/data section isolation;
- C `gnu11`, C++17, and assembly-specific options; and
- device compile definitions.

No directory-wide compiler flags are required.

### Core target

`stm32::core` owns `BareMetal/Core/Inc` and consumes the shared build-policy
target. Core is currently header-only, so it is represented by an `INTERFACE`
library. If Core source files are added later, the same registration function
automatically creates an `OBJECT` library without changing project consumers.

### Driver targets

Every resolved Driver becomes `stm32_driver_<Module>` with a public alias of
`stm32::driver::<Module>`.

- A Driver containing sources is an `OBJECT` library.
- A header-only Driver is an `INTERFACE` library.
- The Driver owns its `Inc` directory and its own source files.
- Every Driver consumes Core.
- Direct Driver-to-Driver relationships are attached after all targets exist.
- The final ELF links every resolved Driver target so every required object is
  retained exactly once.

### Application target

`<Project_Name>.elf` owns only application sources under `Src`, application
header lookup under `Inc`, final link options, and project-specific compile
definitions. Project headers receive lookup priority over shared headers.

Every project now depends only on its own `app_startup.[ch]` and the
component interfaces it selects; the legacy shared `startup.h` and its
compatibility include path have been removed.

## Driver Capability Resolution

Projects request capabilities, not the full implementation chain. The current
direct dependency map is:

| Requested Driver | Automatically required Drivers |
|---|---|
| `ADC` | `GPIO` |
| `BSP` | `GPIO` |
| `DMA` | None |
| `GPIO` | `RCC` |
| `I2C` | `RCC` |
| `NVIC` | None |
| `RCC` | None |
| `Ring_Buffer` | None |
| `SSD1306` | `I2C`, `Ring_Buffer` |
| `SysTick` | None |
| `Timer` | `RCC` |
| `USART` | `RCC`, `GPIO` |

Dependencies are resolved transitively and duplicates are suppressed while
preserving the first requested order.

For example, Project 01 declares only:

```cmake
DRIVER_MODULES
    BSP
    Timer
```

The build resolves:

```text
BSP, Timer, GPIO, RCC
```

`BSP` supplies the board LED capability, `Timer` supplies its current blocking
delay implementation, while `GPIO` and `RCC` remain implementation dependencies
owned by the shared graph.

## Source Discovery

The pipeline searches these locations with `CONFIGURE_DEPENDS`:

```text
<Project>/Src/**/*.{c,cpp,cxx,s,S}
BareMetal/Core/Src/**/*.{c,cpp,cxx,s,S}
BareMetal/Driver/<Resolved_Module>/Src/*.{c,cpp,cxx,s,S}
```

Headers are not compiled as sources. They become visible through the owning
target's include requirements. Adding or removing a matching source causes
CMake to re-evaluate the glob on the next build.

## Compile and Link Policy

The current platform policy targets an STM32F103C8T6 Cortex-M3:

| Domain | Policy |
|---|---|
| Architecture | `-mcpu=cortex-m3 -mthumb -mfloat-abi=soft` |
| C | GNU C11 with strict/missing-prototype warnings |
| C++ | C++17 without exceptions, RTTI, or thread-safe statics |
| Debug | `-g -gdwarf-2` |
| Sectioning | `-fdata-sections -ffunction-sections` |
| Stack analysis | `-fstack-usage` |
| Linker | Project linker script, Nano specs, static libraries |
| Dead-code removal | `-Wl,--gc-sections` |
| Map/cross-reference | Generated for every final ELF |

Only the final application target owns the linker script, memory map, standard
libraries, and link transaction. Driver targets never link firmware images.

## Generated Outputs

A normal out-of-source build uses this shape:

```text
<Project>
├── Build
│   ├── Artifacts
│   │   ├── <Project>.elf
│   │   ├── <Project>.bin
│   │   ├── <Project>.hex
│   │   └── <Project>.map
│   ├── CMakeFiles
│   ├── VSCode
│   ├── CMakeCache.txt
│   └── compile_commands.json
└── .vscode                       Created automatically at configure time
```

After linking, `objcopy` derives binary and Intel HEX images from the ELF. The
memory-report script runs `arm-none-eabi-size`, calculates Flash as
`.text + .data`, calculates RAM as `.data + .bss`, and prints byte, KiB, and
utilization summaries against 64 KiB Flash and 20 KiB RAM.

## VS Code and IntelliSense

The configure step renders reviewed JSON templates into `Build/VSCode` and
copies them into the project's `.vscode` directory in the same pass, so every
project is debuggable from VS Code (F5 → "Debug: <Project>") immediately after
its first configure — no manual copy step. A fresh directory (e.g. after
`clean_all`) is materialized again by the next reconfigure.

The explicit build targets remain available for refresh-only workflows:

```bash
cmake --build Build --target vscode_files
```

Both `settings.json` and `c_cpp_properties.json` point IntelliSense at
`Build/compile_commands.json`. Editor analysis therefore receives the same
macros, include paths, language flags, and feature selections as the compiler.
The configuration no longer maintains a second hand-written approximation of
the build.

The `debug` target builds the ELF and generates these editor files together.

## Firmware and Utility Targets

| Target | Action |
|---|---|
| `<Project>.elf` | Compile, link, convert artifacts, and print memory usage |
| `flash` | Build and flash through ST-Link/SWD |
| `debug_flash` | Print the resolved ST-Link command without touching hardware |
| `flash_uart` | Build and flash through the UART bootloader |
| `info` | Probe the connected ST-Link |
| `erase_flash` | Erase device Flash through ST-Link |
| `erase_flash_uart` | Erase through the UART bootloader |
| `vscode_files` | Generate all project `.vscode` metadata |
| `debug` | Build firmware and prepare VS Code debugging metadata |
| `clean_all` | Remove configured artifacts and generated `.vscode` files |

The ST-Link flash and erase scripts first attempt a normal connection. If that
fails, they retry with connect-under-reset. A second failure terminates the
target with a clear CMake error.

Repository-wide `replace_*` maintenance targets also exist for intentionally
synchronized Template-style files. They overwrite matching files across
`Projects`; inspect their scope before invoking them. They are not part of a
normal application build.

## Building a Project

The generator-independent workflow is:

```bash
cd Projects/GPIO/01_OB_LED
cmake -S . -B Build
cmake --build Build
```

If the local Bash convenience functions are installed, the equivalent normal
workflow is:

```bash
stm32-build
stm32-flash
```

`stm32-build` is a local shell wrapper, not a dependency of the CMake
architecture. CI, another developer, or another editor can always use the
standard CMake commands.

## Creating a New Project

The intended workflow is:

1. Copy `Projects/Template` to the desired application directory.
2. Rename the directory; its name becomes the project, ELF, BIN, HEX, and MAP
   basename automatically.
3. Select application feature options in its `CMakeLists.txt`.
4. List only the Driver capabilities used directly by the application.
5. Add application headers under `Inc` and sources under `Src`.
6. Configure and build out of source in `Build`.

The Template currently maps its feature options to both source selection and
compile definitions:

| Feature option | Enabled Driver/service behavior |
|---|---|
| `APP_ENABLE_SYSTICK_TIMEBASE` | Adds the shared `SysTick` Driver and enables the application millisecond timebase |
| `APP_ENABLE_TIMER_US_DELAY` | Adds the `Timer` Driver and enables the optional polling microsecond-delay service |

This keeps conditional application APIs synchronized with the Driver modules
actually linked into the firmware.

## Adding a Driver to the Build Graph

When a new Driver directory is introduced:

1. create `BareMetal/Driver/<Module>/Inc` and, when required, `Src`;
2. add `STM32_DRIVER_<Module>_DEPENDENCIES` to
   `STM32F103Sources.cmake`;
3. list only its immediate downward Driver dependencies;
4. request the module from one project; and
5. verify the configure summary, compile database, object graph, and final ELF.

Source and include discovery is otherwise automatic.

## Current Validation State

The centralized pipeline was preserved in Git as fallback checkpoint
`a865508` (`BKCP #5`). Project 01 is the first expanded consumer intended for
hardware validation before the same detailed project entrypoint is propagated
further.

The Project 01 pilot currently demonstrates:

- `stm32-build` performs configuration and compilation successfully;
- two requested Driver capabilities resolve to four required modules;
- 3 project sources and 11 Driver sources build into one ELF;
- `.elf`, `.bin`, `.hex`, and `.map` appear under `Build/Artifacts`;
- Flash, debug, erase, information, and VS Code targets are registered; and
- the resulting image reports 8,940 bytes of Flash code/data and 2,080 bytes
  of RAM usage in the current configuration.

Project 01 still uses its legacy startup/application sources. Those sources
currently emit a missing-prototype warning for the legacy `Default_Handler`
and a discarded-qualifier warning for its heap pointer. They do not block the
CMake integration or firmware generation, but they remain application cleanup
work rather than shared build-pipeline defects.

## Deliberate Boundaries

The architecture intentionally does not:

- make a project list every transitive Driver dependency;
- expose every Driver include directory globally;
- place final-link policy on Driver targets;
- generate `.vscode` files during an ordinary configure;
- require the Bash helper functions;
- copy shared `BareMetal` infrastructure into every project; or
- make CMake responsible for application runtime initialization order.

CMake decides what is compiled and how components are connected. Application
code still owns runtime policy such as clock enablement, peripheral
configuration, IRQ-source selection, NVIC delivery, and operation start.

## Known Operational Notes

- CMake 3.28 or newer is required.
- The default absolute tool paths describe the current development machine;
  other installations should pass the corresponding overrides.
- `st-info` and `stm32flash` are optional at configure time, but their targets
  fail if the commands remain unavailable when invoked.
- The primary compiler, linker, debugger, OpenOCD, and ST-Link paths are
  validated during configuration.
- Changing a cached output or toolchain path in an existing `Build` directory
  may require `cmake --fresh -S . -B Build` so old cache state cannot win.
- `flash`, `erase_flash`, UART targets, and repository-wide `replace_*` targets
  mutate hardware or files; ordinary builds do not invoke them.

## Quick Diagnostic Checklist

When a project does not build, check the configure summary in this order:

1. Did repository discovery resolve the intended `STM32F103C8T6` root?
2. Are the Arm compiler, binutils, linker script, and required host tools found?
3. Does `Requested` contain only the application's direct capabilities?
4. Does `Resolved` contain their expected transitive dependencies?
5. Are the expected application and Driver source counts reported?
6. Does `Build Dir` point to the intended `Build/Artifacts` directory?
7. Does `compile_commands.json` reflect the chosen features and flags?
8. Does the final memory report fit within device Flash and RAM?

This sequence follows the same order in which the shared pipeline establishes
the build, making failures easier to localize.
