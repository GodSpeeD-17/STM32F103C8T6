# Repository Agent Instructions (AGENTS.md)

> **This file is the single authority for repository-wide agent instructions.**
> It consolidates the former standalone `CODING_PREFERENCES.md` and
> `BareMetal/Driver/DRIVER_STACK_TOP_DOWN_AUDIT.md` into one document. Those
> two files have been removed; their content now lives in Part 1 and Part 2
> below. Peripheral-specific planning documents (e.g. `I2C_ARCHITECTURE.md`,
> `TIMER_ARCHITECTURE.md`) remain separate and reference this file.

## Contents

- Part 0 - Repository Agent Instructions
- Part 1 - Coding Preferences
- Part 2 - Driver Stack Top-Down Audit Method (Driver -> Codec -> LL)

---

# Part 0 - Repository Agent Instructions

Before modifying source code or documentation, read and follow the Coding
Preferences in **Part 1** of this file.

A requested "Doxygen pass" includes both documentation maintenance and the
source-formatting/comment conventions recorded in Part 1.

Before planning, extending, or auditing a layered STM32 peripheral driver, use
the Driver -> Codec -> LL planning method, scratchpad template, traceability
gate, and Timer reference implementation plan recorded in **Part 2** of this
file.

---

# Part 1: Coding Preferences

# Coding Preferences

This file records the repository owner's coding and documentation preferences.
Update it whenever a new preference is established, and consult it before every
code change, review, or Doxygen pass.

Sections are grouped by concern (Documentation, Naming, Formatting,
Architecture, Application Template) so a specific rule can be found without
reading the whole file. The **Preference Log** at the end is the only
chronological section — it exists to show *when* and *why* each rule was
adopted, not to be read top-to-bottom for daily reference.

---

# 1. Documentation

## 1.1 What Counts as a Doxygen Pass

A Doxygen pass is not limited to editing `/** ... */` blocks. It includes:

- Correct, complete file/group/function/parameter/return-value/note/
  precondition/warning documentation, wherever applicable.
- An explicit `@def MACRO_NAME` in every documented macro's block, including
  short one-line constant macros.
- Valid Doxygen command syntax with resolvable references.
- Concise `//!` comments inside functions explaining logic, sequencing,
  preservation rules, hardware constraints, and side effects.
- Source formatting consistent with the [Call Layout](#32-function--macro-call-layout) rules.
- A Doxygen validation run plus relevant compile, test, and diff checks.

A pass is incomplete until every `#define` in scope has an exact matching
`@def` name and the audit reports zero omissions. This applies to public,
private, object-like, function-like, and one-line macros alike; header
guards without Doxygen blocks are excluded.

## 1.2 Doxygen Reference & Retval Syntax

- Every macro or symbolic constant used as a Doxygen `@ref` must place the
  target inside backticks — in accepted/expected-value lists, parameter
  descriptions, return descriptions, notes, warnings, and ordinary prose:

  ```c
   * - @ref `PERIPH_EVENT_UPDATE`: Update event
   * - Any non-empty combination contained by @ref `PERIPH_EVENT_ALL`
   * @note Start operation with @ref `DRIVER_STATUS_ON`
  ```

  Do not add a second quoted display label after the `@ref` target. Types,
  structures, members, groups, and functions stay unwrapped unless the
  surrounding style explicitly requires code styling.

- Use this exact status-return layout — list marker, backtick-wrapped `@ref`,
  one colon, no trailing period:

  ```c
   * @retval - @ref `DRIVER_STATUS_SUCCESS`: Register image was captured
   * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: An input pointer is `NULL`
  ```

- Use the same list/colon layout for non-status return values:

  ```c
   * @retval - `0x00U`: The selector is not supported
   * @retval - `0x01U`: The selector is supported
  ```

- Every Doxygen `@p` reference to a function parameter must place the
  parameter name inside backticks too, the same as `@ref`:

  ```c
   * @retval - @ref `DRIVER_STATUS_ERROR_NULL_PTR`: @p `pConfig` was a null pointer
   * @note Reading `SR` through this API participates in @p `events` clearing
  ```

  The parameter itself is already code on the other end of the reference
  (a function argument), so the doc-side mention stays code-styled too.

- Two Doxygen 1.9 parser quirks are accepted, not bugs to work around:
  - Repeated return-value warnings because the parser reads the list marker
    (`-`) as the return-value key — do not rewrite the house style to
    silence these.
  - `unexpected token TK_EOF as the argument of ref` for the required
    backtick-wrapped `@ref \`SYMBOL\`` form, when the symbol actually exists.
  - Missing targets, undocumented symbols, and malformed commands remain
    real validation failures — only the two exact warnings above are exempt.

## 1.3 Function Doxygen Layout

Document every public and private function in this order:

1. `@brief`, then `@details` when behavior, ownership, sequencing, or side
   effects need explanation.
2. One `@param[in]` / `@param[out]` / `@param[in,out]` entry per parameter.
3. An `Accepted values:` (input) or `Expected values:` (output) list right
   after each parameter, whenever its vocabulary can be stated.
4. A function-specific `@returns` line naming the returned status type.
5. Complete `@retval` entries for every status the implementation can return.
6. Applicable `@pre`, `@note`, and `@warning` entries.

```c
 * @param[in] operationState Requested Timer operation state
 * Accepted values:
 * - @ref `DRIVER_STATUS_OFF`: Stop Timer counter operation
 * - @ref `DRIVER_STATUS_ON`: Start Timer counter operation
 * @returns @ref driver_status_t "Operation-state operation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Timer counter operation state was updated
 * @retval - @ref `DRIVER_STATUS_ERROR_INVALID_ARG`: @p `TIMx` / @p `operationState` was invalid
```

Validation helpers use an action-oriented `@brief` starting with
`Validates`, plus the same return layout:

```c
 * @brief Validates that the Timer counter is stopped
 * @param[in] TIMx Timer peripheral instance
 * @returns @ref driver_status_t "Counter-state validation status"
 * @retval - @ref `DRIVER_STATUS_SUCCESS`: Timer clock is enabled and the counter is stopped
 * @retval - @ref `DRIVER_STATUS_ERROR_BUSY`: Timer counter is running
```

- Document each parameter on its own — never combine several parameters
  into one Accepted Values block.
- Never omit a finite selector/range list when the API defines one.
- The `@retval` list must match the implementation exactly, including
  distinct null-pointer, invalid-argument, state, busy, and timeout paths.

## 1.4 Structure Doxygen Layout

Document every public and private structure with `@struct` using the public
typedef name. Every member needs one complete block containing:

1. A focused `@brief` explaining the member's semantic role.
2. An `Accepted Values:` list (writable/config members) or `Expected Values:`
   list (observation/output members).
3. Explicit symbolic values when the vocabulary is finite — never hide the
   valid set behind vague prose.
4. `@memberof` naming the owning public typedef.

```c
/**
 * @brief Describes one coherent peripheral configuration domain
 * @struct peripheral_config_t
 */
typedef struct _peripheral_config_t
{
	/**
	 * @brief Operation-mode selector
	 * Accepted Values:
	 * - @ref `PERIPHERAL_MODE_A`
	 * - @ref `PERIPHERAL_MODE_B`
	 * @memberof peripheral_config_t
	 */
	peripheral_mode_t	mode;
} peripheral_config_t;
```

- Do not use a trailing `/**< ... */` member comment when a complete member
  contract can be documented instead.
- Keep accepted/expected lists inside the owning member block so the
  structure can be audited without following its consumers.

## 1.5 Function-Local `//!` Logic Comments

- Place a concise `//!` comment immediately before the operation/block it
  explains:

  ```c
  //! Reuse the shared apply path to keep batching and dirty writes consistent.
  ASSERT_DRIVER_STATUS(_TIM_ValidateClockEnabled(TIMx));
  ```

- A comment must explain *why* the operation exists or *what invariant* it
  protects — never just translate the following C statement into English.
- Every function body needs at least one meaningful `//!` comment, including
  one-line adapters, getters, setters, startup handlers, and minimal
  interrupt handlers. For a thin function, explain why direct delegation, an
  atomic read, or a bounded interrupt-side operation is the correct
  ownership choice. Function-level Doxygen does not substitute for this.
- **A comment is incomplete if a reader would need to ask a follow-up
  question to understand why the line exists.** A label for *what* the
  operation does is not the same as an explanation of *why* it's written
  that way. This applies most to non-obvious arithmetic, bit-manipulation,
  rounding, or hardware-quirk lines — state the actual mechanism, and
  include a concrete before/after example whenever the comment describes a
  numeric trick (rounding, scaling, masking).

  Insufficient — names the operation, not the mechanism:
  ```c
  //! Convert the x100 remainder into one of BRR's 16 fractional steps, rounding to nearest.
  uint32_t fraction = ((fractionRemainder * USART_CODEC_BRR_OVERSAMPLING) + USART_CODEC_BRR_DIV_SCALE_HALF) / USART_CODEC_BRR_DIV_SCALE_FACTOR;
  ```

  Complete — states the mechanism with a worked example:
  ```c
  //! DIV_FRACTION expresses the fraction in sixteenths, not hundredths, so rescale by
  //! OVERSAMPLING (x16) before dividing back out by the x100 scale. Plain integer
  //! division always truncates toward zero (e.g. 1.6 -> 1), so DIV_SCALE_HALF (half of
  //! the x100 divisor) is added first to turn that truncation into round-to-nearest
  //! (e.g. 1.6 -> 2), matching RM0008's own rounded BRR worked examples.
  uint32_t fraction = ((fractionRemainder * USART_CODEC_BRR_OVERSAMPLING) + USART_CODEC_BRR_DIV_SCALE_HALF) / USART_CODEC_BRR_DIV_SCALE_FACTOR;
  ```

- This same standard applies to macro `@brief`s: a brief that only restates
  where a bit lives (its register/field name) instead of what it does to
  hardware behavior fails the same test.
- Interrupt-source and interrupt-status macro documentation must describe the
  request mechanism, not merely call the symbol an "interrupt enable" or
  restate the flag name. The documented contract must identify:
  - the complete Boolean condition that asserts the peripheral interrupt
    request, including every gating enable/mode bit and every source flag;
  - whether setting the enable while its source flag is already set requests
    service immediately;
  - the hardware event that sets the source flag and the software action that
    clears it, so the reader can determine when the request deasserts and why
    an uncleared source re-enters the handler; and
  - that the peripheral source enable controls request generation only, while
    NVIC enablement independently controls delivery to the processor.

  Put source-specific behavior on the public mask alias itself so opening that
  symbol's generated page answers these questions without requiring the reader
  to infer semantics from the macro expansion or navigate to another symbol.

## 1.6 Peripheral `.md` Two-Part Documentation Model

Every canonical peripheral `.md` page must be self-explanatory and contain
exactly two teaching parts, in this order:

1. **Theory with Register Mapping Bridge** — explains the peripheral without
   requiring source-code knowledge:
   - Purpose, terminology, state model, hardware flow, peer interactions,
     and the significance of every register in scope.
   - Register mapping states: owner, address/offset, width, implemented
     range, access class, special read/write semantics, side effects, and
     relationships between registers.
2. **Implementation with Theory Bridge** — maps that hardware model onto the
   repository:
   - Traces each theoretical state/action through its Core C member, LL
     access, Codec transformation (or documented no-Codec reason), Driver
     transaction, public API, volatile access width, ordering, validation,
     preservation, synchronization, concurrency, and shared-resource rules.
   - Explains deliberately omitted access directions or lower-layer
     functions.

Part 1 answers *what the hardware means and which registers express it*;
part 2 answers *how this project implements that theory and why*. Do not
interleave them in a way that forces a reader to understand source symbols
before learning the hardware model.

- The peripheral `.md` file is the single canonical long-form page for that
  peripheral's theory, architecture, register model, layer ownership, and
  implementation mapping — give its top-level heading a stable Doxygen page
  identifier.
- The peripheral's public Driver/LL Doxygen groups must `@ref` that page.
  Source Doxygen must not duplicate the theory: file/group comments
  summarize scope and ownership; individual API comments document only
  their local parameters, results, preconditions, side effects, exact
  hardware transaction, and safety warnings.
- The generated project documentation uses a concise Markdown main page as a
  navigation index, referencing each documentation-ready peripheral page
  without duplicating it. `Doxyfile` must list canonical peripheral pages in
  `INPUT` and configure the index through `USE_MDFILE_AS_MAINPAGE`.
- Only current, reviewed peripheral guides belong in the generated docs.
  Scratchpads, TODO files, superseded architecture snapshots, and internal
  planning records stay excluded unless explicitly promoted. A peripheral
  isn't documentation-complete until its page is linked from both the
  navigation index *and* the relevant generated Doxygen groups.
- For special registers, explicitly distinguish stored state from an action
  port: explain write-one-to-set, write-one-to-clear, write-zero-to-clear,
  read-clear, FIFO, data-port, keyed-write, and write-only behavior *before*
  describing the corresponding C access. Source Doxygen repeats only the
  operation-specific safety warning; a `volatile` declaration alone does not
  document these semantics.

## 1.7 Application Template File Documentation

- Use the Timer module as the concrete Doxygen/file-layout reference for
  application Template files, subject to later explicit overrides (e.g.
  left-aligned section titles). Every file starts with the standard `@file`,
  `@author`, `@brief`, `@version`, `@date`, `@details` metadata.
- The `@details` block must make the file's architectural position
  understandable standalone, using uniquely identified `@section` blocks for
  hierarchy, responsibility, and dependency boundary: the file's layer or
  cross-cutting role, its direct users, its direct dependencies, and its
  prohibited ownership. Never use an unexplained arrow instead of a named
  `includes`/`calls`/`uses`/`is invoked by` relationship.
- Number application layers from hardware upward. Layer 1 is always the
  hardware-nearest project/Driver access layer; increasing numbers own
  progressively more software-only service, policy, behavior, and
  orchestration. A higher layer may depend downward; a lower layer must not
  depend upward.
- Every application configuration macro's Doxygen must explain more than its
  literal value: its architectural significance, the capability/behavior it
  controls, its direct consumers, every accepted value, relevant cross-macro
  or hardware constraints, and what defining it does *not* do. Feature flags
  document both enabled and disabled behavior; hardware-selection macros
  document exclusive ownership and every companion mapping that must stay
  consistent.

## 1.8 Standard Header File Metadata Block

Every new header file starts with a Doxygen metadata block followed by the
header guard and an Includes banner, in this order:

```c
/**
 * @file <filename>.h
 * @author Shrey Shah
 * @brief <concise purpose>
 * @version v1.0
 * @date <DD-MM-YYYY>
 * @details
 * <one or more lines describing the file's architectural position, its
 * direct consumers, its direct dependencies, and the responsibilities that
 * deliberately remain outside this file.>
 */
// Header Guards
#ifndef <FILENAME>_H_
#define <FILENAME>_H_

// ==================================================================================================== //
// Includes
// ==================================================================================================== //
#include "<required header>.h"
```

- The `@file` name must match the actual filename exactly.
- `@author` is `Shrey Shah` for repository-owned files.
- `@brief` is a concise noun phrase naming the file's purpose.
- `@version` starts at `v1.0` for a new file and increments on breaking
  contract changes.
- `@date` uses `DD-MM-YYYY`.
- `@details` must make the file's architectural position understandable
  standalone: its layer/cross-cutting role, direct users, direct dependencies,
  and prohibited ownership. Never use an unexplained arrow.
- The `// Header Guards` marker line precedes `#ifndef`/`#define`, and the
  file closes directly with `#endif /* <GUARD> */` (no redundant end banners).
- The Includes banner uses the standard `=` border and `// Includes` title.
- Capability/aggregate headers include only the headers required by their
  public macros, types, and declarations; implementation-only dependencies
  stay in the `.c` file.
- C++ linkage guards (`extern "C"`) wrap externally linked declarations per
  section 5.

---

# 2. Naming Conventions

## 2.1 Selector Macros vs. Typedef Stem

- Every value macro for a selector typedef must carry that typedef's own
  stem (its name minus the trailing `_t`) — not merely live in the right
  Doxygen group or file section. Singular/plural drift is fine
  (`usart_stop_bits_t` pairing with `USART_STOP_BIT_1`/`_0_5`/`_2`/`_1_5`
  keeps the shared `STOP_BIT(S)` stem despite the plural/singular mismatch),
  but dropping the stem entirely is not.
- The typedef's `_IS_VALID` validation macro is normally already named with
  the full stem. If that validation macro's stem doesn't match the plain
  value macros, the *value macros* drifted and must be renamed to match —
  not the validator. For example:
  - `usart_data_bits_t` had values `USART_8_BITS`/`USART_9_BITS` beside an
    already-correct `USART_DATA_BITS_IS_VALID` → renamed to
    `USART_DATA_BITS_8`/`USART_DATA_BITS_9`.
  - `usart_hardware_enable_t` had values `USART_TX_ENABLE`/`USART_RX_ENABLE`/
    `USART_RTS_ENABLE`/`USART_CTS_ENABLE`/`USART_TX_RX_ENABLE`/
    `USART_RTS_CTS_ENABLE`/`USART_HARDWARE_ALL`/`USART_HARDWARE_NONE` beside
    an already-correct `USART_HARDWARE_ENABLE_IS_VALID` → every value
    renamed onto the shared `USART_HARDWARE_ENABLE_*` stem.
- A consistent stem lets IDE autocomplete surface a selector's full value
  set the moment its typedef-derived prefix is typed, without the author
  needing to already know an unrelated shorthand name.

## 2.2 Register Field Macro Naming

- Use the owning block, register, and field in every field-macro root:
  `BLOCK_REGISTER_FIELD`.
- A single-bit field exposes `_Pos`, `_Msk`, and the unsuffixed mask alias.
- A multi-bit field exposes `_Pos`, `_Width`, `_Msk`, and the unsuffixed
  mask alias, in that order.
- `_Pos`/`_Width` use `reg_bit_pos_t`/`reg_field_width_t`; `_Msk` uses
  `REG_BIT_MASK()` or `REG_FIELD_MASK()`.
- Group raw register-field macros under one banner per owning register;
  order fields least-significant-bit first. When two documented views begin
  at the same bit, keep each internally complete and place the broader
  register/subregister view before its contained fields.
- Keep register-mask macros native to the 32-bit register width; prefer the
  direct `0x01UL` shift expression. Support field widths `0U..31U`; use
  `0xFFFFFFFFUL` explicitly when all 32 bits are required. Document that
  positioned bits beyond bit 31 are discarded instead of complicating the
  macro to preserve out-of-range bits.

## 2.3 Scalar & Register Types

- Use `reg` for register images, register masks, register field values, and
  pointers to caller-owned register images. Reserve fixed-width integer
  types for values that are not register representations, and use
  `uintptr_t` for address arithmetic.
- Use the Core-owned `frequency_t` typedef for every hertz-valued API
  return, output, structure field, constant, and intermediate value. Do not
  introduce peripheral-specific aliases (`rcc_freq_t`, `tim_frequency_t`)
  for the same physical quantity. Prescaler selectors and dimensionless
  divider values keep their own semantic types instead of using
  `frequency_t`.

## 2.4 Variable Naming in Shared Helpers

- Prefer role-specific names — `registerImage`, `positionedFieldMask`,
  `fieldPosition` — over shortened generic names — `regImage`, `mask`,
  `value`, `pos` — inside shared register helpers.
- Express register-field replacement as explicit copy, clear, and set
  operations, with the final return kept separate so each preservation step
  stays easy to inspect.

## 2.5 Application Template File & Symbol Naming

- Startup declarations, linker-symbol declarations, the vector table, and
  application-service policy live inside each project — the shared Driver
  root must not own or inject a project's startup implementation.
- Name project-owned Template service modules `app_*.*`, including
  `app_startup.[ch]`. Keep conventional names only for `main.[ch]` and
  `syscalls.c`.
- Reusable SysTick hardware access is the shared `BareMetal/Driver/SysTick`
  stack, keeping `systick.[ch]` and the `SysTick_` prefix as a naming
  exception. Application time state and the strong `SysTick_Handler()`
  definition belong to the project-owned `app_time` service; the handler
  performs only the minimum state publication needed by main context.
- Symbols called by the processor, linker, or C library keep their required
  ABI names; every other project-owned public service function uses the
  `App_` prefix.
- Project include directories take precedence over shared Driver include
  directories.

---

# 3. Source Formatting & Layout

## 3.1 Control Flow

- Prefer explicit `if`/`else` over ternary expressions — the longer form
  keeps state mapping and error behavior easy to scan.
- Use fully braced, multi-line `switch` cases, with assignments, `break`,
  and returns on separate lines:

  ```c
  //! Match by peripheral base address because instance macros are raw memory-mapped pointers.
  switch ((uintptr_t) TIMx)
  {
  	case TIM2_BASE_ADDRESS:
  	{
  		*pClockMask = RCC_APB1ENR_TIM2EN;
  		break;
  	}
  	default:
  	{
  		return DRIVER_STATUS_ERROR_INVALID_ARG;
  	}
  }
  ```

  Never place a case assignment and `break`, or a default label and return,
  on the same line.

- The same fully braced, multi-line requirement applies to **every**
  `if`/`else` body, including single-statement bodies. Never collapse a
  condition and its body onto one line, and never pack multiple single-line
  `if` statements into an aligned column of `{ statement; }` blocks — even
  when a run of similar short conditions looks compact and readable that
  way:

  ```c
  // Not this:
  if ((cr1RegImage & USART_CR1_IDLEIE) != 0x00000000UL)	{ sources |= USART_IRQ_SOURCE_IDLE; }
  if ((cr1RegImage & USART_CR1_RXNEIE) != 0x00000000UL)	{ sources |= USART_IRQ_SOURCE_RXNE; }

  // This:
  if ((cr1RegImage & USART_CR1_IDLEIE) != 0x00000000UL)
  {
  	sources |= USART_IRQ_SOURCE_IDLE;
  }
  if ((cr1RegImage & USART_CR1_RXNEIE) != 0x00000000UL)
  {
  	sources |= USART_IRQ_SOURCE_RXNE;
  }
  ```

  This stays consistent with the `switch` layout above and remains easy to
  extend with a second statement or a breakpoint later.

- Do not introduce a local variable solely to avoid a ternary, or to
  pre-compute a value that is already directly available at each use site.
  If the input itself can be tested inline at every branch, test it there
  instead of caching it in a redundant intermediate:

  ```c
  // Not this — setBits only ever restates sourceState, adding a variable that
  // tracks no additional information:
  const uint8_t setBits = (sourceState == DRIVER_STATUS_ON) ? 0x01U : 0x00U;
  if (setBits) { updatedCr1RegImage |= USART_CR1_IDLEIE; } else { updatedCr1RegImage &= ~USART_CR1_IDLEIE; }

  // This — test the real input directly at each site:
  if (sourceState == DRIVER_STATUS_ON)
  {
  	updatedCr1RegImage |= USART_CR1_IDLEIE;
  }
  else
  {
  	updatedCr1RegImage &= ~USART_CR1_IDLEIE;
  }
  ```

## 3.2 Function & Macro Call Layout

- Keep a call on one line when it stays readable:

  ```c
  ASSERT_DRIVER_STATUS(_TIM_ValidateClockEnabled(TIMx));
  ```

- When a nested or argument-heavy call needs multiple lines, expand *every*
  nesting level: each function/macro name on its own line, its opening
  parenthesis on the next line, one argument per line, closing parentheses
  aligned to their call depth:

  ```c
  ASSERT_DRIVER_STATUS
  (
  	Codec_TIM_ExtractTimeBaseConfig
  	(
  		LL_TIM_ReadPSC(TIMx),
  		LL_TIM_ReadARR(TIMx),
  		LL_TIM_ReadCNT(TIMx),
  		&timeBase
  	)
  );
  ```

- Do not expand short calls mechanically — choose compact vs. expanded based
  on readability, and keep the chosen form consistent within a module.

## 3.3 Macro Alignment & Banner Grouping

- Align related `#define` values across their complete major banner domain:
  find the longest macro name in that domain, place exactly two tabs after
  it, and add enough tabs after every shorter name so all values start in the
  same column. Register sub-banners do not reset alignment; for example,
  every macro beneath one peripheral's `Register Defines` banner shares one
  column across CR1, CR2, SR1, SR2, and the remaining register sections.
  A single peripheral Core register header also keeps its device constants
  and register-field macros in that shared column. Recalculate only when
  entering a logically independent module or file domain.

  ```c
  /** @brief Microsecond delay chunk used by the millisecond blocking helper @def TIM_DRIVER_BLOCKING_DELAY_MS_CHUNK_US */
  #define TIM_DRIVER_BLOCKING_DELAY_MS_CHUNK_US			((uint16_t) 1000U)
  /** @brief Conservative polling-loop budget per requested microsecond @def TIM_DRIVER_BLOCKING_DELAY_POLL_BUDGET_PER_US */
  #define TIM_DRIVER_BLOCKING_DELAY_POLL_BUDGET_PER_US		((uint32_t) 1024UL)
  /** @brief Fixed setup allowance added to the blocking polling-loop budget @def TIM_DRIVER_BLOCKING_DELAY_POLL_BUDGET_BASE */
  #define TIM_DRIVER_BLOCKING_DELAY_POLL_BUDGET_BASE		((uint32_t) 1024UL)
  ```

- Use tab characters, not runs of spaces, for this alignment and for
  multi-line macro continuation indentation. The single separator after
  `#define` and the conventional ` *` inside Doxygen blocks are not
  indentation and remain spaces. Evaluate visual alignment with a four-column
  tab stop; macro values must begin in the same rendered column under that
  setting.

- Keep every register-field macro family visually atomic: place no blank
  lines between its `_Pos`, optional `_Width`, `_Msk`, and unsuffixed alias,
  then place exactly one blank line before the next field family. Treat a
  standalone raw field-value macro as its own family and separate it the same
  way. This whitespace is a semantic boundary that makes each complete field
  definition immediately distinguishable from the next.

- Within one register-map structure, align every sibling bitfield's trailing
  `/**< ... */` comment to one shared column across all register unions. Use
  the longest declaration as the anchor, place exactly two tabs after it, and
  add tabs after shorter declarations as required. Apply the same rule to
  documented closing register members such as `} CR1;` and `} SR2;`.

(See [2.2 Register Field Macro Naming](#22-register-field-macro-naming) for
the register-banner ordering and `_Pos`/`_Width`/`_Msk` family rules.)

## 3.4 Section Banner Formatting

- Write section-banner titles left-aligned, without decorative padding,
  between the existing border lines — never centered with space/tab runs:

  ```c
  // ==================================================================================================== //
  // Public API
  // ==================================================================================================== //
  ```

- Follow the Timer-header structural style: one `// Header Guard` line
  immediately before `#ifndef`/`#define`, `// --- C++ Compatibility ---` at
  both linkage boundaries, and close the file directly with
  `#endif /* HEADER_GUARD */`. Do not add redundant `Header Guard End`,
  `C++ Compatibility End`, or `Documentation Group` banners.
- Use banners to express logical API hierarchy, not just to divide a long
  file. A major `=` banner owns one coherent domain (configuration,
  operation control, state observation, tick storage, interrupt handling);
  a domain with distinct pairs/subdomains gets a left-aligned `-`
  sub-banner beneath it. Don't club unrelated control/state/data APIs under
  one generic banner. Header and source files use the same banner names and
  API order.
- Within every lifecycle/operation-control conjugate pair, place the
  teardown/release operation before the setup/acquire operation: `Destroy`
  before `Create`, `DeConfig` before `Config`, `Disable` before `Enable`,
  `Stop` before `Start`. Apply this order consistently in declarations,
  definitions, and documentation.

---

# 4. Driver Architecture

## 4.1 Layer Ownership & Reuse

- Place peripheral-independent behavior in the lowest shared layer that can
  express it safely. Peripheral layers keep only their own validation,
  register selection, sequencing, and hardware policy.
- Place every raw register-field macro in the Core header of the hardware
  block that *physically* owns the register, even when another Driver
  consumes that field (e.g. `SCB_AIRCR_xxx` belongs in `stm32f1xx_scb.h`;
  NVIC may consume `AIRCR.PRIGROUP` without owning or duplicating it).
  Compatibility headers may include the owner header to preserve macro
  visibility, but must not redeclare the macros.
- Do not centralize peripheral instance/capability predicates or
  peripheral-specific operating limits in `stm32f1xx_defines.h` — keep them
  in the respective peripheral stack (e.g. oscillator/clock-frequency
  limits belong to RCC).
- Peripheral-independent register *behavior* (e.g. compare-and-write) lives
  in a shared primitive like `RegOps_WriteIfChanged()`; a peripheral (Timer)
  wraps it to validate its own instance/register selection, and other
  peripherals (GPIO, RCC) can reuse the same primitive while keeping their
  own rules.
- Reserve a static LUT for genuine table-shaped per-instance data — the
  kind of mapping that would otherwise need real metadata storage (e.g. a
  GPIO pin-routing table with port/pin/mode per instance). When such a LUT
  is needed, index it by the driver's stable instance index using the
  smallest element type that represents the mapping directly (an array of
  `rcc_bus_t`, not an array of metadata structures).
- For a small, fixed dispatch over 2-3 known instances (such as resolving
  one RCC bus selector or one clock/reset mask per instance), prefer a
  `switch` on the peripheral base address over a LUT. A LUT trades memory
  for faster access, and that trade only pays for itself when the access is
  either genuinely hot or the data is too rich to express as `switch` cases;
  neither applies to a one-time-per-configuration-call constant lookup. Use
  the same base-address `switch` shape already established for register-field
  and reset-mask decoding (see `_TIM_DecodeAPB1ClockEnableMask()`), not a
  parallel array, so the lookup mechanism stays consistent regardless of
  which instance property is being resolved.

## 4.2 Register Access Pattern (`.REG` / `.BIT`)

- When a Core register structure models named fields, expose the hardware
  word as a register-member union with a full-width `.REG` image and a
  named `.BIT` field view (the Timer register-map pattern). Order fields
  least-significant-bit first, and represent every gap explicitly with a
  reserved field so the view totals exactly 32 bits. If one physical bit
  range has different read and write meanings, add named `.BIT.READ` /
  `.BIT.WRITE` views inside the union instead of picking one misleading
  interpretation.
- `.REG` is the canonical LL transaction path. A `.BIT` view documents and
  exposes field placement, but does not authorize a compiler-generated
  read-modify-write where the hardware contract requires a staged
  full-register transfer — keyed, action, write-only, and
  write-one-to-clear transactions must use the full-register operation.
- Centralize each peripheral LL's ordinary full-width volatile access in one
  generic pointer-based read primitive and one generic pointer-based write
  primitive. Named register accessors select the right `.REG` pointer and
  delegate to those primitives — never repeat direct dereference or
  `RegOps_Read()`/`RegOps_Write()` mechanics. Keep the named surface
  symmetric where hardware permits; preserve truthful asymmetry for
  read-only, write-only, and action-port registers, documenting *why* the
  missing direction/conjugate doesn't exist. A semantic write-to-clear
  helper may delegate through the corresponding named writer.

## 4.3 Validation & Fallible Results (`driver_status_t`)

- Every validation function returns `driver_status_t`:
  `DRIVER_STATUS_SUCCESS` when valid, the most specific
  `DRIVER_STATUS_ERROR_*` otherwise. Never return `uint8_t`, `bool`,
  `0x00U`, or `0x01U` as a validation result.
- Fallible predicates and state queries must preserve the distinction
  between a legitimate negative state and an operation failure — never
  collapse a null pointer, invalid argument, unavailable clock, invalid
  hardware state, or other error into `0`, `false`, or an apparent
  OFF/not-pending result.
- Use one of these three contracts:
  - **Validation-only helper** — returns `DRIVER_STATUS_SUCCESS` or a
    specific `DRIVER_STATUS_ERROR_*`.
  - **Truly binary state getter** — returns `DRIVER_STATUS_OFF`/`_ON`
    directly, while still preserving distinct error statuses.
  - **Fallible data/mask/extensible-state query** — returns operation
    status, publishing the result through an output pointer only after the
    operation succeeds.
- Compatibility wrappers must not collapse a status-returning API into a
  raw Boolean result — remove or migrate such wrappers instead of treating
  an error as a valid negative result.
- Driver and Codec orchestration/staging/commit/validation/cleanup helpers
  that participate in a status-returning call chain must also return
  `driver_status_t` — never `void` merely because current LL operations
  can't fail. Return `DRIVER_STATUS_SUCCESS` and preserve status
  propagation so later validation or hardware-error reporting doesn't need
  an API-contract change.
- Reserve `void` for interfaces whose contract is inherently void (an
  interrupt-handler ABI) and for deliberately dumb LL write primitives that
  own no validation, sequencing, or fallible policy.

## 4.4 Conjugate API Naming & Scope

- Conjugate API pairs must be symmetric in both naming and semantic scope —
  `TIM_Config()`/`TIM_DeConfig()`, `TIM_GetIRQEvents()`/`TIM_AckIRQEvents()`.
  Never pair a function that owns only a narrow subdomain with one that
  resets or mutates the complete peripheral.
- Every public getter for a mutable operation state needs its symmetric
  setter (`PERIPH_GetOperationState()` / `PERIPH_SetOperationState(state)`).
  When a state domain accepts both `DRIVER_STATUS_OFF` and `_ON`, prefer
  that single Get/Set pair over separate public `Disable()`/`Enable()`
  functions. Separate action verbs remain appropriate only when the
  hardware actions are genuinely asymmetric and can't be one truthful state
  setter — document that exception explicitly.
- A root configuration type (`tim_config_t`) represents every currently
  admitted base-configuration domain intentionally included in it. Its
  root config API applies the whole object while preserving explicitly
  separate operational domains (e.g. IRQ-source enables). Its
  deconfiguration conjugate restores the documented reset state. Conjugate
  lifecycle entry points stay independent: a configuration function must
  not call its deconfiguration conjugate, and vice versa — the application
  owns their ordering and explicitly requests a reset when needed.
- Do not expose general-purpose convenience functions that configure a
  peripheral to an arbitrary requested frequency — callers provide explicit
  register-semantic values. A narrowly named service-bootstrap helper is
  acceptable when a concrete admitted service needs one fixed
  configuration, validates its documented clock assumption, and delegates
  the canonical root configuration API (e.g. `TIM_ConfigForBlockingDelay()`
  for a validated 72 MHz Timer kernel clock) — it must never become a
  general frequency setter.
- Calculated-frequency *getters* are fine — they observe/report programmed
  state without mutating configuration.

## 4.5 Structured Configuration & Transaction Decomposition

**Configuration structures**

- A root configuration structure is a structure of independently coherent
  configuration domains. Do not create a configuration structure merely
  because one function accepts more than one value — prefer explicit typed
  parameters when a small set of single-use scalar values has no
  independently reusable domain, lifecycle, or cross-field invariant
  (SysTick clock-source + reload is the canonical example). A structure is
  appropriate only when its members form a stable domain that's passed,
  stored, returned, nested, or validated as one reusable object.
- Every nested configuration structure representing an independently
  useful, admitted operation normally needs a symmetric public grouped
  `Get`/`Set` pair. Deliberately asymmetric hardware actions are exceptions
  and must document why no conjugate exists. The root function and the
  grouped setter reuse the same narrowly scoped private staging helper; the
  grouped getter uses the matching extraction path.

  ```c
  typedef struct _periph_config_t
  {
  	periph_timebase_config_t	timebase;
  	periph_counter_config_t		counter;

  } periph_config_t;
  ```

- Do not place interrupt-request source enables inside a root configuration
  structure — IRQ-source enablement stays an explicit application action so
  the call site proves interrupt generation was intentional. Root
  configuration preserves the peripheral IRQ-source register; the
  application clears stale peripheral and NVIC pending state, enables NVIC
  delivery, and only then enables the peripheral IRQ source:

  ```c
  ASSERT_DRIVER_STATUS(RCC_EnablePeripheralClock(PERIPH1_CLOCK_MASK));
  ASSERT_DRIVER_STATUS(PERIPH_Config(PERIPH1, &config));
  ASSERT_DRIVER_STATUS(PERIPH_AckIRQEvents(PERIPH1, PERIPH_IRQ_EVENT_UPDATE));
  ASSERT_DRIVER_STATUS(NVIC_ClearPendingIRQ(PERIPH1_IRQn));
  ASSERT_DRIVER_STATUS(NVIC_EnableIRQ(PERIPH1_IRQn));
  ASSERT_DRIVER_STATUS
  (
  	PERIPH_SetIRQSources
  	(
  		PERIPH1,
  		PERIPH_IRQ_SOURCE_UPDATE,
  		DRIVER_STATUS_ON
  	)
  );
  ASSERT_DRIVER_STATUS(PERIPH_SetOperationState(PERIPH1, DRIVER_STATUS_ON));
  ```

  Omitting `PERIPH_SetIRQSources()` leaves existing IRQ-source state
  unchanged; disabling sources is equally explicit, passing the owned
  source mask with `DRIVER_STATUS_OFF`.

**Clock, reset, and NVIC ownership**

- Clock-gate ownership stays explicit and outside the peripheral Driver.
  The application uses the RCC Driver to enable the required gate *before*
  configuration. A peripheral Driver may query RCC to validate that
  precondition, but must not expose duplicate clock APIs or mutate the gate
  inside `Config()`, `DeConfig()`, grouped configuration, operation-state,
  IRQ, or action APIs.
- `DeConfig()` restores the peripheral register bank through the peer RCC
  reset service when that's the hardware-defined mechanism, but leaves the
  application-owned clock gate and NVIC delivery state unchanged. The
  application explicitly decides when either external resource is
  disabled:

  ```text
  RCC clock enable
    -> peripheral base configuration
    -> stale peripheral-event acknowledgement
    -> NVIC pending-state cleanup and delivery enablement
    -> peripheral IRQ-source enablement
    -> peripheral operation enablement
  ```

- Root peripheral configuration must not read, validate, clear, disable, or
  enable NVIC delivery state — the application owns the explicit ordering
  between base configuration, stale-event cleanup, pending-line cleanup, NVIC
  delivery enablement, peripheral IRQ-source enablement, and the final
  transition to active state. For external peripheral IRQs, enable the NVIC
  mask before enabling the peripheral source so every subsequently generated
  request has an already-established delivery path. SysTick is exempt because
  it does not use an external NVIC IRQ line.

**Staging helpers**

- Staging helpers each accept only one configuration domain plus the
  caller-owned register images that domain can modify:

  ```c
  static driver_status_t _PERIPH_StageTimeBaseConfig
  (
  	const periph_timebase_config_t* const	pTimeBaseConfig,
  	reg* const								pPrescalerRegisterImage,
  	reg* const								pAutoReloadRegisterImage,
  	reg* const								pCounterRegisterImage
  );

  static driver_status_t _PERIPH_StageCounterConfig
  (
  	const periph_counter_config_t* const	pCounterConfig,
  	reg* const								pControlRegisterImage
  );
  ```

- A staging helper must:
  - perform only the transformation named by its signature;
  - validate its own pointer/domain contract and return `driver_status_t`;
  - transform caller-owned, non-volatile images with no MMIO, clock, reset,
    NVIC, operation-state, or cleanup work;
  - use Codec functions for individual field encoding instead of
    duplicating register placement in the Driver;
  - stage through local working images and publish output only after every
    fallible transformation succeeds; and
  - preserve every register field outside its declared domain.
- Every structure inside a public root configuration object has one
  corresponding private `_PERIPH_Stage<Domain>Config()` helper, owning the
  atomic local-copy/publication boundary for its structure and invoking the
  grouped Codec transformation. One reusable staging path per configuration
  member — no exceptions.

**Public API ownership**

- Public configuration APIs own the full Read/Modify/Write transaction:

  ```text
  PERIPH_GetTimeBaseConfig()
    -> validate public output and live preconditions
    -> read every required register image once
    -> extract the complete timebase structure without changing hardware

  PERIPH_SetTimeBaseConfig()
    -> validate public input and live preconditions
    -> read every required current register image once
    -> call _PERIPH_StageTimeBaseConfig(...)
    -> commit changed images in the required hardware order
    -> run the documented cleanup path

  PERIPH_Config()
    -> validate the complete root request and lifecycle preconditions
    -> read every image required by all represented domains
    -> call _PERIPH_StageCounterConfig(...)
    -> call _PERIPH_StageTimeBaseConfig(...)
    -> commit only after all fallible staging succeeds
    -> preserve peripheral IRQ-source state
    -> run the documented cleanup path
  ```

- The root function must not duplicate the grouped staging logic, and must
  not call public grouped setters when that would create multiple partially
  committed transactions — it reuses their private staging helpers, stages
  the whole root request before the first write, then owns one ordered
  commit. A grouped public setter reads and commits only its own domain's
  registers.
- Private helpers stage cached values but never commit configuration MMIO
  themselves — the public configuration API directly compares and writes
  its cached images in the required hardware order, including any
  temporary policy, action-register, or restoration sequence. This keeps
  the public function's maximum authority and real hardware cost visible.
  Only deliberately mechanical LL register-write primitives stay `void`.
- Timer example: `TIM_Config()` visibly composes the staging paths for
  `tim_config_counter_t` and `tim_config_timebase_t` while preserving
  `TIMx_DIER`. `TIM_GetCounterConfig()`/`SetCounterConfig()` and
  `TIM_GetTimeBaseConfig()`/`SetTimeBaseConfig()` are the grouped domain
  pairs; their setters reuse the staging paths while the root transaction
  performs one all-domain ordered commit. No private helper shadows all of
  `TIM_Config()`, and no configuration path calls `TIM_DeConfig()`.
  Applications configure Timer interrupt generation separately through
  `TIM_SetIRQSources()`.

## 4.6 MMIO Access Minimization

- Modularity must not multiply volatile register accesses. Helper
  boundaries separate validation, staging, and commit responsibilities —
  they never grant a helper permission to reread or rewrite the same
  hardware register.
- For each coherent Driver transaction:
  1. Validate the complete request and every live precondition that can
     fail.
  2. Snapshot each required volatile register exactly once.
  3. Pass caller-owned `reg` images through narrow, MMIO-free staging
     helpers.
  4. Coalesce every compatible domain/field change into the final register
     image.
  5. Dirty-write each changed register exactly once when hardware allows.
  6. Perform additional reads/writes only when a named hardware
     requirement demands it — an unlock sequence, mode transition,
     write-zero/write-one-to-clear behavior, read-to-clear behavior, or a
     preload/update-event commit.
- Hold the declared exclusive ownership or transaction guard across the
  whole snapshot/stage/commit interval. Do not reread a cached
  configuration register just to repeat a precondition check right before
  commit — if concurrency must be supported, define an explicit guard
  instead of weakening the cached-image model.
- Do not reread a register merely because multiple staging helpers consume
  its fields, and do not let separate helpers commit partial images when
  one public API owns the coherent transaction. Every additional MMIO
  access must be justified by the peripheral contract and documented
  beside the sequence requiring it — correct hardware ordering takes
  precedence over forcing an unsafe single-write implementation.
- Do not introduce a transaction structure merely to shorten a helper
  signature or hide several temporal images of the same register — pass
  independently required `reg` images explicitly so inputs, outputs, and
  authority stay visible. A structure is appropriate only for a stable,
  reusable domain with its own invariant, never as a bare argument
  container.
- When hardware makes a requested field read-only/non-writable in the
  current mode, return the appropriate `driver_status_t` error *before* the
  first write. Do not hide a multi-step mode transition inside one
  configuration call to force the request to succeed — the application
  explicitly calls the public APIs that leave the restrictive mode, apply
  the field, and restore the desired mode:

  ```c
  periph_domain_config_t currentConfig;

  ASSERT_DRIVER_STATUS
  (
  	Codec_PERIPH_ExtractDomainConfig(currentRegisterImage, &currentConfig)
  );

  //! Reject a field transition that hardware cannot accept in the current mode.
  if ((currentConfig.mode == PERIPH_MODE_RESTRICTIVE) &&
  	(currentConfig.field != pConfig->field))
  {
  	return DRIVER_STATUS_ERROR_STATE;
  }
  ```

  The Driver never synthesizes those mode-change calls or intermediate MMIO
  writes internally.
- The generic single-register transaction shape:

  ```c
  driver_status_t PERIPH_SetDomainConfig
  (
  	PERIPH_TypeDef* const				pPeripheral,
  	const periph_domain_config_t* const	pConfig
  )
  {
  	// Local Variables
  	reg currentRegisterImage = 0x00000000UL;
  	reg targetRegisterImage = 0x00000000UL;

  	// Validate Input and State
  	ASSERT_DRIVER_STATUS(_PERIPH_ValidateDomainTransaction(pPeripheral, pConfig));

  	//! Snapshot once, then let modular helpers transform only local images.
  	currentRegisterImage = LL_PERIPH_ReadControl(pPeripheral);
  	targetRegisterImage = currentRegisterImage;
  	ASSERT_DRIVER_STATUS(_PERIPH_StageDomainConfig(pConfig, &targetRegisterImage));

  	//! Coalesce compatible changes into one dirty write.
  	if (currentRegisterImage != targetRegisterImage)
  	{
  		LL_PERIPH_WriteControl(pPeripheral, targetRegisterImage);
  	}

  	return DRIVER_STATUS_SUCCESS;
  }
  ```

  If hardware requires an intermediate image, the public transaction
  performs the minimum ordered multi-write sequence directly, with Doxygen
  and `//!` comments naming the hardware rule that makes each additional
  access mandatory.

## 4.7 BSP Capability Packaging and Initialization

- Split board support by cohesive hardware capability. The Blue Pill board
  package uses `bsp_gpio.[ch]` for board GPIO conveniences and
  `bsp_usart.[ch]` for the fixed board USART transport, and `bsp_timer.[ch]`
  for the configuration-locked TIM4 microsecond-delay service; do not place
  any capability implementation back into a monolithic `bsp.c`.
- Each capability implementation includes its own matching public header
  first. Each public capability header is self-contained and includes only
  the headers required by its public macros, types, and declarations.
  Implementation-only dependencies remain in the `.c` file.
- `bsp.h` is an aggregate compatibility header. It includes capability headers
  only under BSP-owned build definitions such as
  `BSP_GPIO_CAPABILITY_ENABLED`, `BSP_USART_CAPABILITY_ENABLED`, and
  `BSP_TIMER_CAPABILITY_ENABLED`; shared BSP code never consumes project-owned
  `APP_ENABLE_*` policy.
- New and minimal consumers include `bsp_gpio.h`, `bsp_usart.h`, or
  `bsp_timer.h` directly and request the matching `BSP_GPIO`, `BSP_USART`, or
  `BSP_TIMER` CMake component. The aggregate `BSP` component deliberately
  resolves every board capability.
- Expose one canonical public initialization transaction per capability:
  `BSP_InitOBLED()`, `BSP_InitUSART()`, and `BSP_InitTimerUSDelay()`. Each
  initializer owns every RCC, GPIO, and peripheral operation needed to make
  its board capability ready and deterministic. Application boot code
  conditionally orchestrates those calls; it does not reproduce board clock
  masks or configuration sequences.
- Expose `BSP_TimerDelayUs()` as the sole steady-state operation of the TIM4
  delay capability. It accepts the application-facing `uint32_t` duration,
  owns decomposition into the Timer Driver's 16-bit transactions, and returns
  the first exact Driver failure without adding another board Timer API.
- Divide each complete BSP initializer into private `static inline` helpers
  for its cohesive clock, GPIO, and peripheral subtransactions. Do not expose
  those helpers or implementation-only RCC masks in public BSP headers.
- Keep steady-state board operations public only when a current consumer needs
  them. Legacy debug-USART wrappers may remain in the USART capability as
  explicitly documented compatibility APIs, but they delegate canonical BSP
  behavior and do not create a second initialization policy.
- `BSP_TIMER` exclusively reserves TIM4 as a polling-only 1 MHz counter for
  microsecond delays. Its initialization owns the TIM4 RCC gate and complete
  fixed delay configuration; its delay operation owns bounded chunking and
  Driver-status propagation. It never enables a Timer IRQ source, requests
  NVIC, publishes application time, or consumes the application's interrupt-
  driven timebase selection. CMake selects this capability when the application
  requests microsecond delay support, and that configuration must reject TIM4
  as the simultaneous application timebase.

---

# 5. Application Template Rules

(For file/symbol naming specifics, see
[2.5 Application Template File & Symbol Naming](#25-application-template-file--symbol-naming).)

- Treat `Projects/Template` as the canonical seed for every new application;
  never create a new project by copying another numbered example. A policy,
  structure, service, CMake default, or documentation change intended for all
  current and future projects must be implemented and verified in the Template
  first, then propagated to existing projects. Project-specific behavior stays
  in its owning project and does not flow back into the Template.
- Every application Template header that declares an externally linked
  function or object must wrap those declarations in an `extern "C"` guard
  for C++ consumers. Macro-only configuration headers don't need this.
- Application Template files use `stm32f1xx_data_types.h` as their direct
  source of fixed-width integers, shared physical quantities, register
  vocabulary, and driver statuses — never include `<stdint.h>` or another
  standard scalar-type header directly from a Template file. Standard
  headers providing a distinct service (`<errno.h>`) remain direct
  dependencies where that service is actually used.
- Keep `app_time.h` and `app_time.c` project-local. A new project copies their
  canonical baseline from `Projects/Template` and thereafter owns both files;
  do not replace them with one shared implementation or include a shared `.c`
  file. This is deliberate policy-layer duplication, not reusable Driver/BSP
  duplication.
- The project-local timing service owns the complete application timebase
  policy: the selected SysTick or general-purpose Timer base, its requested
  tick frequency, the monotonic software tick, peripheral IRQ-source and NVIC
  sequencing where applicable, and the selected strong IRQ-handler ABI symbol.
  Every application timebase is interrupt-driven.
- Strong timing handlers remain project-local so each application can use the
  selected IRQ's full capabilities: event inspection and service ordering,
  selective acknowledgement, scheduling publication, diagnostics, profiling,
  shared-event handling, and project-specific error policy. A shared strong
  handler, weak hook, or callback layer must not seize or narrow that ownership
  merely to remove identical starter code.
- Consolidate application timing and delay behavior into the project-local
  `app_time.[ch]` pair; do not retain a separate `app_delay.[ch]` pair. The
  millisecond/tick delays consume the application-owned interrupt-driven
  timebase. Optional microsecond delay APIs delegate to the independently
  selected polling-only `BSP_TIMER` capability and do not reproduce its TIM4
  clock or fixed configuration sequence.

---

# 6. Shared Build Reporting

- The shared CMake configure report must enumerate project-local headers and
  sources under a dedicated `[Local]` section whose tree starts directly with
  `Inc/` and `Src/`, using the same filename/full-path mode as selected Driver
  modules. Summary counts distinguish headers, compiled sources, and include
  directories; an include-directory count must never be labeled as a count of
  included files.
- The post-link memory report must present exact bytes, human-readable KiB,
  used/free/capacity totals, utilization percentages, and the GNU Berkeley-size
  category composition used to derive Flash (`text + data`) and RAM
  (`data + bss`) consumption. Keep this in one compact aligned region table;
  use exactly one row per region and make every numeric cell self-contained as
  `bytes (KiB)`. Do not split one region across multiple unit rows, print a
  separate detailed category table, add decorative graphs or duplicated
  visualizations, or repeat the CMake status prefix on every report line.
  Prioritize auditable numeric detail.

---

# Preference Log

- 2026-08-28: Standardized the new-header metadata block: every new header
  starts with `@file`/`@author`/`@brief`/`@version`/`@date`/`@details`, a
  `// Header Guards` marker, `#ifndef`/`#define`, an `=`-border `// Includes`
  banner, and closes with `#endif /* GUARD */`; `@details` states the file's
  layer, users, dependencies, and prohibited ownership.
- 2026-08-28: Required shared CMake diagnostics to list project-local C/header
  files in a dedicated `[Local]` `Inc/`/`Src/` tree alongside selected Driver
  files, report semantically correct file and include-directory counts, and
  render one compact aligned Flash/RAM table with free space, utilization
  percentages, and explicit GNU-size composition formulas, without a separate
  category table or decorative graphics.
- 2026-08-28: Required interrupt enable/status macro documentation to state
  the full request condition, already-set behavior, flag set/clear mechanism,
  repeated-entry consequence, and separate NVIC-delivery requirement on the
  public mask alias rather than merely restating a register bit name.
- 2026-08-28: Established `Projects/Template` as the mandatory canonical seed
  for new applications and required cross-project policy/structure changes to
  be implemented and verified there before propagation; numbered examples may
  not serve as project-creation baselines.
- 2026-08-26: Kept `app_time.[ch]` intentionally project-local so each
  application retains unrestricted strong SysTick/Timer IRQ ownership, merged
  application delay behavior into that pair, and assigned the optional
  polling-only 1 MHz TIM4 microsecond-delay configuration to the independently
  selected `BSP_TIMER` capability.
- 2026-08-26: Split BSP into independently selected GPIO and USART capability
  headers/sources and CMake components, retained `bsp.h` as a guarded aggregate,
  required capability headers to be self-contained and minimal, and assigned
  complete RCC/GPIO/peripheral initialization ownership to `BSP_InitOBLED()`
  and `BSP_InitUSART()` with private static-inline subtransaction helpers.
- 2026-08-23: Extended peripheral-header alignment across device constants,
  all register-field macros, sibling bitfield comments, and documented closing
  register members so the complete header uses coherent shared columns.
- 2026-08-23: Restricted static LUTs to genuine table-shaped per-instance
  data; required a base-address `switch` instead of a parallel array for
  small fixed dispatch such as an RCC bus or clock/reset mask lookup, since
  the memory-for-speed LUT trade-off doesn't pay for itself on a one-time
  configuration-path lookup.
- 2026-08-23: Clarified that one peripheral `Register Defines` domain uses a
  single macro-value column across every register sub-banner; sub-banners do
  not restart alignment.
- 2026-08-23: Fixed the repository's visual-alignment basis at four-column
  tab stops so tab-only macro alignment renders consistently in the owner's
  editor.
- 2026-08-23: Required exactly one blank line between complete register-field
  macro families, with no blank lines inside a `_Pos`/optional `_Width`/`_Msk`/
  alias family; standalone raw field values follow the same separation rule.
- 2026-08-23: Extended the fully braced, multi-line requirement from `switch`
  cases to every `if`/`else` body, prohibiting collapsed single-line
  `{ statement; }` forms even for runs of similar short conditions.
- 2026-08-23: Required `//!` logic comments (and macro `@brief`s) to state the
  actual mechanism behind non-obvious arithmetic/bit-manipulation/rounding
  lines, with a worked example for numeric tricks, rather than only labeling
  what the operation does.
- 2026-08-23: Required every selector-typedef value macro to carry that
  typedef's own stem, matching an already-correctly-named `_IS_VALID`
  validation macro rather than the other way around.
- 2026-08-23: Required Doxygen `@p` parameter references to backtick-wrap the
  parameter name, the same as `@ref`, since the referenced parameter is
  already code on the declaration side.
- 2026-08-22: Prohibited configuration structures that only bundle a small
  set of single-use scalar function arguments; required explicit typed
  parameters unless the values form a reusable domain object or invariant.
- 2026-08-22: Centralized peripheral LL register mechanics in generic
  pointer-based read/write primitives and required named accessors to delegate
  register selection while preserving hardware-defined access asymmetry.
- 2026-08-22: Required at least one meaningful implementation-local `//!`
  comment inside every function body, including thin wrappers, state accessors,
  startup handlers, and minimal IRQ handlers.
- 2026-08-22: Required every public mutable operation-state getter to have a
  symmetric setter and preferred one OFF/ON Get/Set pair over redundant public
  Disable/Enable functions.
- 2026-08-22: Required hierarchical logical API banners mirrored between
  headers and sources, and ordered lifecycle/control conjugates teardown-first
  (`Destroy`/`DeConfig`/`Disable`/`Stop` before their constructive counterparts).
- 2026-08-22: Fixed application layer numbering from hardware upward, with
  Layer 1 always hardware-nearest and higher layers owning progressively more
  software orchestration; required capability-oriented configuration-macro
  Doxygen with accepted values, consumers, constraints, and non-effects.
- 2026-08-22: Made `stm32f1xx_data_types.h` the sole Template-facing gateway
  for standard fixed-width and repository-wide scalar types; prohibited direct
  Template `<stdint.h>` includes.
- 2026-08-22: Adopted Timer-style Template file metadata and structural guard
  markers, prohibited redundant end/group banners, and required explicit
  per-file hierarchy, responsibility, and dependency-boundary documentation.
- 2026-08-22: Required left-aligned, unpadded ordinary section-banner titles
  and an explicit `Header Guard` marker before every header guard.
- 2026-08-22: Assigned startup ownership to each project, standardized
  reusable Template filenames on `app_*.*`, retained conventional platform and
  ABI entry filenames, and required project-local headers to take include-path
  precedence.
- 2026-08-22: Kept project-owned SysTick as an explicit `systick.[ch]` and
  `SysTick_` naming exception beneath the application time service.
- 2026-08-22: Superseded project-local SysTick ownership: moved reusable
  SysTick hardware state into a shared Driver/LL stack while keeping the
  software tick and minimal strong `SysTick_Handler()` in `app_time`.
- 2026-08-22: Required C++ linkage guards in every Template header that
  declares externally linked functions or objects.
- 2026-08-22: Named the project-owned startup module `app_startup.[ch]` while
  retaining processor-mandated reset and exception ABI symbol names.
- 2026-08-16: Restored the repository-owner Doxygen style using
  backtick-wrapped `@ref` targets and list-style referenced `@retval` status
  entries. This later decision supersedes the earlier same-day native Doxygen
  return/reference syntax preference.
- 2026-08-16: Replaced invalid backtick-wrapped Doxygen `@ref` targets with
  bare targets plus quoted backtick-styled display labels, and replaced the
  invalid `@retval - @ref ...` list form with Doxygen's native
  `@retval VALUE Description` form.
- 2026-08-16: Required Timer-style `.REG` and `.BIT` views for Core register
  structures with named fields, explicit reserved gaps, nested views for
  overlapping read/write meanings, and full-register LL transactions for
  keyed, action, write-only, or write-one-to-clear semantics.
- 2026-08-16: Required register-scoped macro banners, least-significant-bit
  first field order, `BLOCK_REGISTER_FIELD` roots, and complete
  `_Pos`/`_Width`/`_Msk`/mask-alias families matching the Timer register
  layer.
- 2026-08-16: Assigned raw register-field macros to the Core header of the
  physical register owner, prohibited peer Driver headers from duplicating
  those definitions, and permitted compatibility includes to preserve macro
  visibility.
- 2026-08-16: Assigned peripheral instance/capability predicates and
  peripheral-specific operating limits to their respective driver stacks
  instead of the shared Core defines header.
- 2026-08-16: Required macro value alignment and multi-line macro continuation
  indentation to use tabs rather than runs of spaces.
- 2026-08-16: Established the initial theory-to-implementation documentation
  bridge; the later two-part single-source rule below supersedes duplication of
  theory in function-local LL Doxygen.
- 2026-08-16: Made reviewed peripheral `.md` files the canonical long-form
  Doxygen pages, required public Driver/LL groups and the generated project
  navigation page to reference them, and kept planning/TODO material excluded.
- 2026-08-16: Required each canonical peripheral guide to contain Theory with
  Register Mapping Bridge followed by Implementation with Theory Bridge, and
  prohibited duplicating that theory in source Doxygen.
- 2026-08-15: Required every macro or symbolic constant used as a Doxygen
  `@ref` target to be enclosed in backticks in all documentation contexts.
- 2026-08-15: Assigned peripheral clock-gate query/mutation to RCC/application,
  prohibited peripheral Drivers from hiding gate transitions inside lifecycle
  or narrow APIs, and required `DeConfig()` to leave application-owned RCC and
  NVIC state unchanged.
- 2026-08-15: Required coherent Driver transactions to snapshot each required
  register once, stage modularly in local images, coalesce compatible changes,
  dirty-write each register once where legal, and document every additional
  hardware-mandated MMIO access.
- 2026-08-15: Required one MMIO-free `_PERIPH_Stage<Domain>Config()` helper per
  root-configuration structure member and required the public configuration API
  to commit all staged register images directly; private configuration commit
  helpers may not hide the public transaction's hardware writes.
- 2026-08-15: Required requests for fields that are not writable in the current
  hardware mode to fail before MMIO; applications explicitly own the public
  calls needed to leave and later restore that mode instead of one Driver call
  hiding intermediate mode writes.
- 2026-08-15: Prohibited private transaction structures that exist only to
  shorten signatures; independently required temporal register images remain
  explicit unless they form a stable reusable domain with their own invariant.
- 2026-08-15: Prohibited root configuration from reading or mutating NVIC
  delivery state; applications explicitly sequence IRQ sources and NVIC before
  transitioning the peripheral to its active operation state.
- 2026-08-15: Excluded IRQ-source enables from root configuration structures;
  applications must configure peripheral interrupt generation explicitly
  before independently enabling NVIC delivery.
- 2026-08-15: Defined root configuration as visible composition of reusable
  domain-staging helpers, assigned Read/Modify/Write and ordered commit
  ownership to public Driver APIs, and prohibited private helpers that shadow
  an entire public root transaction.
- 2026-08-15: Permitted narrowly named fixed-configuration service helpers
  when they validate their clock assumptions and delegate canonical root
  configuration, while retaining the prohibition on general frequency setters.
- 2026-08-15: Extended the retval list/value/colon layout to non-status return
  values and prohibited legacy entries without the list marker, code
  formatting, and colon.
- 2026-08-15: Required conjugate configuration/deconfiguration entry points to
  remain independent, prohibited either lifecycle function from calling its
  conjugate, and assigned lifecycle ordering and explicit reset decisions to
  the application.
- 2026-08-15: Established Core-owned `frequency_t` as the universal
  hertz-valued type and prohibited duplicate peripheral frequency aliases.
- 2026-08-15: Required conjugate APIs to have symmetric names and semantic
  scope, established root `Config` / `DeConfig` ownership, and prohibited
  frequency-targeting configuration functions while retaining observational
  frequency calculations.
- 2026-08-15: Required Driver and Codec transaction helpers to return
  `driver_status_t`; reserved `void` for inherently void interfaces and dumb,
  non-validating LL write primitives.
- 2026-08-15: Extended the exact function Doxygen layout to private helpers,
  required validation-specific return descriptions, and prohibited tab-aligned
  or period-terminated driver-status descriptions.
- 2026-08-15: Required validation functions to return `driver_status_t` and
  prohibited fallible predicates or compatibility wrappers from collapsing
  errors into raw Boolean results.
- 2026-08-15: Added compact-versus-expanded call layout rules.
- 2026-08-15: Required meaningful `//!` logic comments inside functions.
- 2026-08-15: Defined a Doxygen pass as documentation, source-style, and
  validation work rather than comment-only editing.
- 2026-08-15: Added banner-local macro alignment with two tabs after the
  section's longest macro name.
- 2026-08-15: Required fully braced multi-line switch cases.
- 2026-08-15: Preferred explicit if/else control flow over ternary expressions.
- 2026-08-15: Required peripheral-independent register behavior to live in
  RegOps, with peripheral layers retaining validation and hardware policy.
- 2026-08-15: Preferred simple 32-bit `UL` register-mask expressions, with a
  separate explicit full-register mask instead of special-case macro logic.
- 2026-08-15: Required `reg` for register representations, semantic variable
  names in shared helpers, and explicit copy-clear-set field staging.
- 2026-08-15: Required the exact status-reference layout shown above for
  documented driver statuses.
- 2026-08-15: Required per-instance hardware topology, such as Timer-to-RCC-bus
  ownership, to live in an indexed static LUT.
- 2026-08-15: Required LUTs to use the smallest direct element type and avoid
  metadata structures when the table stores only one value.
- 2026-08-15: Required every documented macro, including short constant
  macros, to carry an explicit `@def MACRO_NAME` tag.
- 2026-08-15: Defined the complete public-function Doxygen order, per-parameter
  Accepted/Expected Values blocks, referenced return type, exhaustive status
  list, and consistently formatted `@retval` descriptions.


# Part 2: Driver Stack Top-Down Audit Method

# STM32 Driver Stack Top-Down Planning and Audit

## Document Control

| Field | Value |
| --- | --- |
| Purpose | Reusable planning, scratch, decision, and completion reference for STM32 peripheral-driver stacks |
| Planning direction | Driver Layer -> Codec Layer -> LL Layer |
| Worked example | STM32F103 TIM2, TIM3, and TIM4 Timer stack |
| Audit type | Architecture-to-implementation gap analysis; no implementation is authorized by this document alone |
| Generic method status | Reviewed and accepted as the normative repository audit method |
| Reusable worksheet status | Self-contained gate/legend template; independently validated |
| Timer evidence status | Reviewed snapshot at commit `91635a4` on 2026-08-15 |
| Timer planning status | Architecture decisions and mitigation order reviewed; open implementation gates are recorded below |
| Timer implementation status | Not closed; Waves 1-3 contain P0 safety work |
| Repository style authority | [`AGENTS.md` (Part 1)](#part-1-coding-preferences) |
| Normative precedence | This file governs future planning, admission, and mitigation where it conflicts with older peripheral notes |
| Timer implementation snapshot | [`BareMetal/Driver/Timer/TIMER_ARCHITECTURE.md`](BareMetal/Driver/Timer/TIMER_ARCHITECTURE.md) is historical/current-state context, not a competing planning authority |
| Doxygen processing | Excluded from generated API documentation; this is a mutable planning and evidence artifact |

### Later Timer PWM decision authority

The subsequently approved
[`BareMetal/Driver/Timer/TIMER_PWM_ARCHITECTURE.md`](BareMetal/Driver/Timer/TIMER_PWM_ARCHITECTURE.md) supersedes
this worked example's earlier PWM ownership proposals. Timer PWM is now a
cohesive Timer-driver subdomain in `BareMetal/Driver/Timer/Inc/timer_pwm.h` and
`BareMetal/Driver/Timer/Src/timer_pwm.c`: Timer base APIs own frequency, PWM owns channel mode,
channel polarity, duty cycle, and channel output-enable state, and the
application owns GPIO/AFIO. The
approved scalar API deliberately has no PWM config structure, heap handle,
endpoint registry, or separate `Driver/PWM` module. Historical findings below
remain useful evidence but are not competing PWM API authority. The PWM
subdomain also avoids the worked example's open cross-feature `EGR.UG` hazard:
stopped configuration/duty transactions load only the selected active CCR
through a temporary selected-lane preload bypass while CEN and CCxE are clear.
Unlike the Timer root lifecycle and service APIs described later in this
worked example, PWM APIs do not query the RCC gate: successful `TIM_Config()`
completion is their lifecycle prerequisite. They validate `TIMx` identity and
the Timer/PWM register state owned by their transaction only. Open Timer-base
and future-feature findings remain open.

## Start Here

1. Read **Architectural Invariants** and **Repeatable Top-Down Audit
   Procedure** for the reusable method.
2. Read **Driver**, **Codec**, and **LL Layer Audit** for the Timer worked
   example.
3. Read **Finding and Hazard Ledger**, **Canonical Closure Gate Registry**,
   and **Implementation Waves** before changing Timer code.
4. Copy **Reusable Audit Worksheet** for another STM32 peripheral.
5. Use **Final Planning Conclusions** as the decision summary; do not treat it
   as proof that implementation gates have passed.

## Why This Document Exists

This document makes the reasoning between an STM32 peripheral's hardware model
and its implementation explicit and repeatable. It answers, in order:

1. Which user intents deserve stable public Driver APIs?
2. Which private Driver helpers are required to implement those contracts
   safely and without duplication?
3. Which pure Codec transformations are demanded by the frozen Driver design?
4. Which LL register accesses are demanded by the Driver and Codec design?
5. Which types, metadata, tests, and documentation prove the complete stack is
   coherent?

The ordering is binding. A register field does not justify a public function,
and an existing LL accessor or Codec function does not by itself justify a
Driver API. Public intent drives private orchestration; private orchestration
drives register-image transformations and hardware access.

## How To Use This File

Use this file in three roles:

- **Scratchpad:** record evidence and unresolved questions in the Working
  Ledger before editing source code.
- **Planning contract:** freeze each layer at its completion gate before using
  it to derive the next layer.
- **Final reference:** retain accepted decisions, dependency matrices, and
  implementation waves after the audit is complete.

Every ledger entry uses one of these labels:

| Label | Meaning |
| --- | --- |
| `[Observed]` | Directly verified in source, tests, device headers, or the reference manual |
| `[Decision]` | Accepted architecture rule or API ownership decision |
| `[Gap]` | Required behavior or evidence is absent or incomplete |
| `[Deferred]` | Valid scope intentionally assigned to a later wave or another module |
| `[Question]` | Decision cannot be frozen until stated evidence is collected |
| `[Conclusion]` | Reviewed planning conclusion; implementation still requires its named closure evidence |

Do not silently convert a question into a decision. Add the evidence, record
the decision, and update every affected traceability row.

Disposition lifecycle is tracked separately from those labels:

```text
Observed -> Candidate -> Admitted -> Contract-frozen -> Implemented -> Verified
```

`[Gap]`, `[Deferred]`, and the other evidence/decision labels describe the
state of knowledge; they do not imply a lifecycle state. A deferred candidate
is not admitted, and a reviewed contract is not implemented or verified.

## Architectural Invariants

These rules apply to every peripheral stack unless a documented hardware
exception requires a narrower rule.

1. **Driver APIs express stable user intent.** They accept domain types and
   configuration structures, validate public contracts, own peripheral state,
   and sequence cross-register or cross-driver operations.
2. **Private Driver helpers express repeated policy.** They own topology
   lookup, capability checks, precondition guards, snapshot/stage/commit
   transactions, dirty writes, event latching, and cleanup or rollback.
3. **Codec APIs are pure transformations.** They decode caller-owned register
   images into domain values and stage domain values into caller-owned images.
   They never dereference a peripheral instance or perform volatile I/O.
4. **LL APIs are mechanical register access.** They read or write one register
   image and contain no public validation, clock policy, state transitions,
   event policy, or domain interpretation.
5. **Register mechanics shared by multiple peripherals belong in RegOps.** A
   Timer-specific wrapper may bind a Timer register, but it must not duplicate
   generic read, write, field-stage, or dirty-write mechanics.
6. **Feature drivers compose peripheral drivers.** PWM, encoder, capture
   measurement, and similar application-facing modules may own calculations,
   GPIO routing, and multi-peripheral composition while using stable Timer
   primitives underneath.
7. **One layer does not repair another layer's contract.** LL does not validate
   Driver inputs; Codec does not compensate for an unsafe Driver sequence; the
   Driver does not expose raw bit positions to avoid defining domain types.
8. **Every public mutator has a written transition contract.** The contract
   states whether the clock must be enabled, whether operation must be stopped,
   which registers may change, whether preload requires an event, and what is
   left unchanged on failure.
9. **No public API is accepted without traceability.** Each accepted function
   maps to a user intent, private orchestration path, Codec transformation or
   documented no-Codec case, LL access, hardware resource, statuses, and tests.
10. **Documentation is part of completion.** Public Doxygen follows the exact
    repository preferences, and implementation logic uses focused `//!`
    comments where reasoning is not self-evident.
11. **Root configuration visibly composes reusable domains.** A root Driver
    function stages each nested configuration domain through the same narrow
    helper reused by that domain's public grouped API. It does not delegate its
    entire authority to a private root-shadow helper, call grouped public
    setters as separately committed transactions, or write hardware before all
    fallible root staging succeeds.

## Dependency Direction

```text
Application or feature driver (for example PWM)
                    |
                    v
             Public Driver API
                    |
                    v
         Private Driver orchestration
           /          |           \
          v           v            v
   Codec (pure)   LL (volatile)   peer services
      |               |          RCC/NVIC/GPIO/DMA
      v               v
   RegOps field    RegOps I/O
   operations      operations
```

The Driver may call Codec, LL, and peer services. Codec and LL must not call the
Driver. Codec must not call LL. A higher-level feature driver may call the
public Timer Driver but must not bypass it through Timer Codec or Timer LL.

## Repeatable Top-Down Audit Procedure

### Phase 0: Establish Scope and Evidence

1. Name the supported MCU family, concrete part, peripheral instances, and
   hardware capability variants.
2. Collect the device header, reference-manual register semantics, errata,
   startup/clock topology, existing API, tests, examples, and higher-level
   consumers.
3. Build an instance capability table before designing APIs. Record bus,
   reset domain, IRQ lines, channel count, counter width, optional registers,
   DMA routes, GPIO/remap ownership, and hardware quirks.
4. State explicit non-goals and separate current-target support from future MCU
   portability.
5. Inventory existing symbols without treating them as accepted architecture.

**Phase 0 completion gate:** every supported instance can be described without
conditional guesswork, and every hardware claim has a named source.

### Phase 1: Freeze the Driver Layer

1. Enumerate user intents and higher-level consumers.
2. Apply the public API admission test to each intent.
3. Group related fields into coherent configuration transactions.
4. Define public types, accepted values, return statuses, preconditions,
   postconditions, side effects, and concurrency expectations.
5. Derive one reusable private staging path for each coherent configuration
   domain, plus separate helpers only for repeated validation, topology,
   hazardous commit sequencing, or cleanup—not merely to shorten a function.
6. Write each public Driver function as the owner of its complete
   Read/Modify/Write transaction. Root configuration visibly composes the
   domain staging paths and performs one ordered all-domain commit.
7. Reject, defer, or move every remaining candidate to a feature driver.

**Driver completion gate:** the public contract is sufficient for all in-scope
user intents, contains no register-leakage API, and every function has a
complete symbolic implementation path.

### Phase 2: Derive and Freeze the Codec Layer

1. Read only the frozen Driver transactions and public domain types.
2. List every required decode and stage transformation by register image.
3. Prefer symmetric `Extract`/`Stage` pairs for persistent configuration and
   state; document deliberate asymmetry for action and acknowledge semantics.
4. Factor repeated bit/field mechanics into generic RegOps or private Codec
   helpers.
5. Prove that staging preserves unrelated bits and that multi-image staging is
   all-or-nothing from the caller's perspective.
6. Reject peripheral pointers, volatile access, clock checks, and sequencing
   from Codec.

**Codec completion gate:** every Driver transformation is implemented or
planned exactly once, every selector has encode/decode coverage, and pure tests
cover valid values, invalid values, null destinations, boundary images, and
unrelated-bit preservation.

### Phase 3: Derive and Freeze the LL Layer

1. Read the frozen Driver transactions and Codec register-image requirements.
2. List the registers that must be read, written, or both.
3. Classify each register's access semantics: read-only, read/write,
   write-only/action, write-zero-to-clear, write-one-to-clear, reserved-bit
   constrained, preload/shadowed, or data-port.
4. Provide thin named accessors where they improve type-safe call sites and a
   generic register operation only where the project deliberately supports a
   raw mechanical primitive.
5. Keep special write semantics visible; do not disguise an action or flag
   acknowledge as an ordinary read-modify-write operation.
6. Mark complete-register-map accessors that are not currently demanded as raw
   foundation, not as evidence for upper-layer scope.

**LL completion gate:** every demanded register access exists with correct
semantics, no LL function owns policy or domain validation, and unused raw
coverage is explicitly classified rather than accidentally driving the API.

### Phase 4: Cross-Layer Closure

1. Complete the public API traceability matrix.
2. Complete the register ownership and write-semantics matrix.
3. Verify error propagation and failure atomicity at every boundary.
4. Verify lifecycle, clock, reset, IRQ, DMA, GPIO/remap, and concurrency
   ownership.
5. Map tests to contracts and negative paths.
6. Run compile, unit, documentation, style, and example-consumer checks.
7. Perform an independent adversarial review against the completion gates.

**Audit completion gate:** there are no unclassified symbols, untraced public
APIs, missing transformations, ambiguous write semantics, or unresolved
questions inside the declared scope.

## Working Evidence and Decision Ledger

| ID | State | Decision or evidence | Consequence |
| --- | --- | --- | --- |
| A-001 | `[Implemented]` | The Timer root configuration contains only timebase and counter domains. | `TIM_Config` neither reads nor mutates IRQ-source or NVIC delivery state; applications explicitly configure both before starting the counter. |
| A-002 | `[Observed]` | Timer Codec already contains master/slave, output-compare, channel-state/polarity, IRQ, and DMA transformations beyond the current public Driver surface. | Existing lower-layer breadth must be classified as planned foundation or excess; it cannot create public scope by itself. |
| A-003 | `[Observed]` | Timer LL provides named accessors for the complete TIM2-TIM4 register map. | Required access and raw foundation must be distinguished in the LL matrix. |
| D-001 | `[Decision]` | Timer public APIs will represent Timer primitives; PWM frequency/duty and GPIO-route composition remain above Timer. | PWM mode is a valid output-compare selector, but percentage duty-cycle policy does not belong in Timer. |
| D-002 | `[Decision]` | Persistent readable state should normally have paired `Get` and `Set` APIs; actions and acknowledge operations use explicit verbs instead. | Avoid fake getters for action registers and avoid fake setters for pending flags. |
| D-003 | `[Decision]` | Group and scalar setters reuse the same narrow staging and hazardous commit primitives when fields share constraints or latch behavior. | Each public API owns one visible Read/Modify/Write transaction without calling another setter and repeating its hardware snapshot. |
| D-004 | `[Decision]` | Every hertz-valued quantity uses the Core-owned `frequency_t` alias. | RCC, Timer, and peer consumers must not introduce duplicate peripheral-specific frequency aliases. |
| D-005 | `[Decision]` | A coherent Driver transaction snapshots each required register once, stages modular changes in local images, and commits the minimum hardware-legal MMIO sequence. | Helper boundaries never justify redundant accesses. When a field is not writable in the current hardware mode, reject the transaction and require explicit application-owned mode transitions instead of hiding intermediate mode writes. Every remaining additional access must trace to a documented indivisible hardware commit requirement. |
| C-001 | `[Decision]` | Codec transformations operate on `reg` images and domain types only. | Peripheral pointers, RCC/NVIC access, and volatile reads/writes are forbidden in Codec. |
| L-001 | `[Decision]` | LL is mechanically complete only when access semantics are correct, not merely when every register has a read/write wrapper. | `SR` acknowledge and `EGR` action writes require semantic scrutiny even if generic accessors exist. |
| X-001 | `[Decision]` | Safety Waves 1-3 precede every new Timer feature; the narrow PWM-output foundation is the first expansion after the P0 gates. | Current defects are mitigated before channel code reuses the same transaction, LL, and IRQ patterns. |

The detailed layer audits, traceability matrices, implementation waves, and
final planning conclusions follow in the remaining sections.

### Verified Evidence Snapshot

| Evidence | Repository source | Audit use |
| --- | --- | --- |
| Public Timer contracts | [`BareMetal/Driver/Timer/Inc/timer.h`](BareMetal/Driver/Timer/Inc/timer.h) | Current API families, compatibility surface, documented side effects |
| Public types/selectors/config | [`stm32f1xx_data_types.h`](BareMetal/Core/Inc/stm32f1xx_data_types.h), [`BareMetal/Driver/Timer/Inc/timer_data_types.h`](BareMetal/Driver/Timer/Inc/timer_data_types.h), [`timer_defines.h`](BareMetal/Driver/Timer/Inc/timer_defines.h), [`timer_config.h`](BareMetal/Driver/Timer/Inc/timer_config.h) | Shared frequency type, current Timer semantic vocabulary, and grouped configuration coverage |
| Driver transactions/helpers | [`BareMetal/Driver/Timer/Src/timer.c`](BareMetal/Driver/Timer/Src/timer.c) | Validation order, MMIO order, preload commit, IRQ/NVIC, delay cleanup |
| Codec declarations/implementation | [`BareMetal/Driver/Timer/Inc/timer_codec.h`](BareMetal/Driver/Timer/Inc/timer_codec.h), [`timer_codec.c`](BareMetal/Driver/Timer/Src/timer_codec.c) | Current Extract/Stage symmetry, dormant coverage, W0C behavior, gaps |
| LL register access | [`BareMetal/Driver/Timer/Inc/timer_ll.h`](BareMetal/Driver/Timer/Inc/timer_ll.h) | Complete-map inventory and access-direction audit |
| Raw Timer register map | [`stm32f1xx_timer.h`](BareMetal/Core/Inc/stm32f1xx_timer.h) | Raw fields, aliases, access qualifiers, register layout |
| Generic register operations | [`stm32f1xx_utils.h`](BareMetal/Core/Inc/stm32f1xx_utils.h) | Image staging, MMIO transfer, dirty/RMW semantic restrictions |
| Historical verification record — non-gating | Timer Codec host-test result recorded in [`BareMetal/Driver/Timer/TODO.md`](BareMetal/Driver/Timer/TODO.md) | Eight unique exported Codec entry points were invoked directly, with additional transitive scalar execution; no retained repository-local executable test evidence |
| Removed legacy migration target | Historical `Driver/PWM` heap/handle implementation | Superseded by the Timer-owned scalar API in `BareMetal/Driver/Timer/TIMER_PWM_ARCHITECTURE.md`; legacy sources were removed after consumer migration |
| Concrete part/package | [`STM32F103C8T6 Datasheet`](Reference_Docs/STM32F103C8T6_Datasheet.pdf), DS5319 ordering and LQFP48 pinout | Package-bonded Timer remap routes and SWJ pin conflicts |
| Hardware semantics | [`STM32F103C Reference Manual`](Reference_Docs/STM32F103C_Reference_Manual.pdf), especially RM0008 Chapter 15 | CEN/CMS/DIR, preload/UG, CCMR/CCER, CCR read-clear, SR W0C, EGR action, DCR/DMAR |

The repository does not retain an STM32F103 silicon errata document. Obtain and
review the errata matching the deployed device revision before a Timer wave can
claim hardware implementation closure; absence of that evidence is not treated
as proof that no Timer erratum applies.

### Worked-Example Capability Snapshot

This compact table records what is proven for the current target and leaves
unadmitted routes visibly unfrozen. It satisfies evidence inventory, not every
future feature gate.

| Index/instance | Clock and reset | IRQ line | Counter/channels | DMA routes from RM0008 | Pin/remap ownership | Internal trigger inputs / remaining evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 0 / `TIM2` | APB1; `TIM2EN` / `TIM2RST` | Dedicated `TIM2_IRQn` (28) | 16-bit, four channels | CH3: DMA1 Ch1; UP: Ch2; CH1: Ch5; CH2/CH4: Ch7 | AFIO owns 2-bit remap; all four routes are package-available | ITR0=TIM1, ITR1=TIM8 unavailable, ITR2=TIM3, ITR3=TIM4; silicon errata remains open |
| 1 / `TIM3` | APB1; `TIM3EN` / `TIM3RST` | Dedicated `TIM3_IRQn` (29) | 16-bit, four channels | CH3: DMA1 Ch2; CH4/UP: Ch3; CH1/TRIG: Ch6 | AFIO owns 2-bit remap; C8T6 exposes no-remap and partial-remap only, one encoding is reserved and full-remap pins are not bonded | ITR0=TIM1, ITR1=TIM2, ITR2=TIM5 unavailable, ITR3=TIM4; silicon errata remains open |
| 2 / `TIM4` | APB1; `TIM4EN` / `TIM4RST` | Dedicated `TIM4_IRQn` (30) | 16-bit, four channels | CH1: DMA1 Ch1; CH2: Ch4; CH3: Ch5; UP: Ch7 | AFIO owns a raw full-remap bit, but the C8T6 package exposes only no-remap pins | ITR0=TIM1, ITR1=TIM2, ITR2=TIM3, ITR3=TIM8 unavailable; silicon errata remains open |

Sources are the Timer instance/index definitions, current Timer topology
helpers, RCC and NVIC definitions, AFIO raw map, datasheet package tables, and
RM0008. RM0008 Table 78 is authoritative for the DMA routes because current
Core DMA-channel comments contain conflicting TIM assignments. DMA routes are
shared DMA1 channels, not exclusive Timer resources; a DMA peer contract
remains deferred and the Core metadata conflict requires repair before use.
The Timer kernel is PCLK1 when the APB1 prescaler is 1 and twice PCLK1
otherwise. TIM2 remaps using PA15/PB3 and TIM3 partial remap using PB4 conflict
with default JTAG/SWJ ownership, so the AFIO/board route must deliberately
release those pins (for example by retaining SWD while disabling JTAG).
Hardware-common capability includes up/down/center counting, a 16-bit
prescaler, four input-capture/output-compare/PWM/one-pulse lanes,
synchronization/encoder/Hall support, and Timer DMA requests. That hardware
breadth is not current public scope: base/IRQ/delay is implemented,
channel/PWM is accepted next, and capture/synchronization/DMA remains deferred.

### Reproducible Inventory Snapshot

Counts and classifications below were taken from baseline `91635a4` on
2026-08-15. Re-run these commands whenever a wave changes the surface:

```bash
rg -n "^driver_status_t TIM_|^__STATIC_INLINE (driver_status_t|uint8_t) TIM_" BareMetal/Driver/BareMetal/Driver/Timer/Inc/timer.h
rg -n "^__STATIC_FORCEINLINE .* _TIM_" BareMetal/Driver/BareMetal/Driver/Timer/Src/timer.c
rg -n "^driver_status_t Codec_TIM_" BareMetal/Driver/BareMetal/Driver/Timer/Inc/timer_codec.h
rg -n "^__STATIC_FORCEINLINE .* Codec_TIM_" BareMetal/Driver/BareMetal/Driver/Timer/Src/timer_codec.c
rg -n "^__STATIC_FORCEINLINE .*LL_TIM_(Read|Write)" BareMetal/Driver/BareMetal/Driver/Timer/Inc/timer_ll.h
rg -n "^#define LL_TIM_(REG|SCALAR_REG)" BareMetal/Driver/BareMetal/Driver/Timer/Inc/timer_ll.h
rg -n "TIM_|Codec_TIM_|LL_TIM" Projects/PWM BareMetal/Driver/Timer -g "*.[ch]"
```

Record the new command output, commit/date, and reviewer in the canonical gate
evidence table; a remembered count is not closure evidence.

## Driver Layer Audit

### Driver Responsibility

The Driver is the only Timer layer allowed to know all of the following at the
same time:

- the public user intent and domain types;
- the selected Timer instance and its capabilities;
- live clock, operation, channel, trigger, DMA, and IRQ state;
- which register images must be captured;
- which Codec transformations must be performed;
- which LL writes are legal and in which order;
- which peer drivers own RCC, NVIC, GPIO/AFIO, and DMA resources;
- what must be restored when a transaction cannot complete.

This makes the Driver an orchestration layer, not a second Codec and not an
expanded LL.

### Structured Configuration Composition Rule

A root configuration object is a structure of coherent configuration domains.
Each nested structure that represents an independently admitted operation
normally has a symmetric public grouped `Get` / `Set` pair. The grouped setter
and root API reuse the same private domain-staging helper, while the getter uses
the corresponding extraction path. Deliberately asymmetric hardware actions
must document why no conjugate exists.

Use this generic shape:

```c
typedef struct _periph_config_t
{
	periph_timebase_config_t	timebase;
	periph_counter_config_t		counter;

} periph_config_t;

static driver_status_t _PERIPH_StageTimeBaseConfig
(
	const periph_timebase_config_t* const	pTimeBaseConfig,
	reg* const								pPrescalerRegisterImage,
	reg* const								pAutoReloadRegisterImage,
	reg* const								pCounterRegisterImage
);

static driver_status_t _PERIPH_StageCounterConfig
(
	const periph_counter_config_t* const	pCounterConfig,
	reg* const								pControlRegisterImage
);
```

IRQ-source enablement is intentionally absent from the generic root object.
Root configuration preserves peripheral interrupt-source state. Applications
must clear stale peripheral and NVIC pending state and enable NVIC delivery
before calling the peripheral's explicit IRQ-source setter:

```c
ASSERT_DRIVER_STATUS(PERIPH_Config(PERIPH1, &config));
ASSERT_DRIVER_STATUS(PERIPH_AckIRQEvents(PERIPH1, PERIPH_IRQ_EVENT_UPDATE));
ASSERT_DRIVER_STATUS(NVIC_ClearPendingIRQ(PERIPH1_IRQn));
ASSERT_DRIVER_STATUS(NVIC_EnableIRQ(PERIPH1_IRQn));
ASSERT_DRIVER_STATUS
(
	PERIPH_SetIRQSources
	(
		PERIPH1,
		PERIPH_IRQ_SOURCE_UPDATE,
		DRIVER_STATUS_ON
	)
);
ASSERT_DRIVER_STATUS(PERIPH_SetOperationState(PERIPH1, DRIVER_STATUS_ON));
```

This sequence makes interrupt intent visible at the call site. Omitting the
source setter preserves existing source state; disabling sources likewise
requires an explicit `DRIVER_STATUS_OFF` request.

Root configuration does not query NVIC delivery as a hidden precondition.
The application owns the sequence from peripheral configuration through stale
event cleanup, NVIC delivery enablement, peripheral IRQ-source enablement, and
the final active-state transition. External peripheral IRQs establish the
NVIC delivery path before the peripheral is permitted to generate requests;
SysTick has no external NVIC line and is therefore exempt.

The staging helpers operate only on non-volatile caller-owned images, use
Codec transformations for field placement, preserve unrelated fields, return
`driver_status_t`, and publish their output images only after every fallible
step succeeds. They do not receive a peripheral pointer merely to read or
write MMIO, and they do not own clock, reset, NVIC, operation-state, or cleanup
policy.

MMIO access count is part of the transaction contract. The public Driver API
snapshots each required volatile register once, passes those images through
modular staging helpers, coalesces compatible changes, and dirty-writes each
changed register once when hardware semantics permit. Helper boundaries do not
justify repeated reads or partial writes. Every additional access must trace to
a named hardware rule, such as an unlock sequence, mandatory mode transition,
special flag-clear semantic, read-clear behavior, or preload/update commit;
correct hardware ordering remains authoritative when one write is not legal.
Do not replace explicit temporal `reg` images with a private structure that
exists only to shorten a signature. Such a structure is admitted only when its
members form a stable reusable domain with an independently documented
invariant.

Each structure contained by a root configuration object has one matching
private Driver `_PERIPH_Stage<Domain>Config()` helper. It owns atomic local-copy
publication for exactly that structure and delegates field placement to the
grouped Codec transformation. Policy or action images outside that structure
must be staged separately.

The public grouped function owns its narrow Read/Modify/Write transaction:

```text
PERIPH_GetTimeBaseConfig()
  -> validate output and live preconditions
  -> snapshot required images
  -> extract and publish the complete domain without changing hardware

PERIPH_SetTimeBaseConfig()
  -> validate input and live preconditions
  -> snapshot required images
  -> _PERIPH_StageTimeBaseConfig(...)
  -> commit changed images in hardware order
  -> cleanup
```

The root function owns the maximum admitted configuration transaction:

```text
PERIPH_Config()
  -> validate the complete root request and lifecycle preconditions
  -> snapshot all required images
  -> _PERIPH_StageCounterConfig(...)
  -> _PERIPH_StageTimeBaseConfig(...)
  -> commit only after every domain stages successfully
  -> preserve peripheral IRQ-source state
  -> cleanup
```

The root function must not call public grouped setters when that would split
one root request into partially committed sub-transactions. It reuses their
private staging logic instead, then directly performs every required register
write. A private configuration commit helper must not hide the public API's
MMIO sequence or maximum authority.

### Public API Admission Test

A candidate becomes a canonical public Driver API only when all applicable
questions have satisfactory answers.

| Test | Acceptance question | Reject, defer, or relocate when... |
| --- | --- | --- |
| Consumer intent | Does a named application or peer driver need this as an independent operation? | The only justification is that a bit or LL function exists. |
| Semantic vocabulary | Can inputs and outputs be expressed without raw positions, masks, encodings, or register pointers? | The proposed contract exposes register mechanics. |
| Stable ownership | Is this peripheral the unambiguous owner of the state or action? | GPIO/AFIO, DMA-controller, NVIC, clock-tree, board-route, or feature-service ownership is stronger. |
| Coherent granularity | Do the grouped fields share preconditions, commit timing, and failure behavior? | A scalar would split an inseparable hardware transition, or a root structure would collect unrelated features. |
| Capability behavior | Is supported/unsupported behavior defined per instance? | The API silently assumes every STM32 variant has the same width, channels, bus, or feature set. |
| Complete contract | Can preconditions, side effects, postconditions, statuses, concurrency, and rollback be documented? | Failure can leave unspecified partial state. |
| Testability | Can success, invalid input, invalid state, ordering, no-write-on-error, and side effects be observed? | The behavior cannot be verified below an application demo. |
| Canonicality | Is this the single preferred semantic path or a clearly labeled convenience/compatibility wrapper? | It duplicates another path with different validation or hidden side effects. |

Public linkage and application-facing API are not synonyms. Every exported
function must also declare one audience:

- **Application semantic API** — normal peripheral use.
- **Peer-service API** — infrastructure used by another driver.
- **Compatibility wrapper** — temporary migration surface with a removal plan.
- **Raw/diagnostic API** — deliberately mechanical and never presented as the
  normal safe path.

### Current Timer Public Inventory

The application header `timer.h` exposes 35 callable symbols, all through the
primary public surface; the six legacy IRQ compatibility wrappers are removed.
`timer_defines.h` additionally exposes three pure callable channel utilities:
`TIM_ChannelMaskToIndex`, `TIM_ChannelMaskExtractLowestChannel`, and
`TIM_ChannelMaskRemoveChannel`. The unconsumed instance-index API and its LUT
were removed in favor of private base-address switch decoding.

| Family | Current surface | Current verdict |
| --- | --- | --- |
| Clock gate | none | RCC/application owns Timer APB1 clock-gate query and mutation; Timer APIs only validate that the required gate is already enabled. |
| Operation state | `TIM_GetOperationState`, `TIM_SetOperationState` | Coherent ownership of `CR1.CEN`. |
| Lifecycle | `TIM_Config`, `TIM_DeConfig` | Independent lifecycle entry points requiring an enabled application-owned gate. Configuration covers the timebase and counter domains represented by `tim_config_t`; both APIs leave RCC gate and NVIC state unchanged. |
| General frequency-setting presets | none | General frequency-targeting configuration functions were removed; applications provide explicit prescaler/configuration data. |
| Input-clock observation | `TIM_GetInputClockFrequency` | Public RCC-derived Timer kernel-clock query; requires a stable configured clock tree but no Timer clock gate or Timer MMIO access. |
| Blocking-delay service configuration | `TIM_ConfigForBlockingDelay` | Narrow fixed service bootstrap; validates a 72 MHz Timer kernel clock, delegates the canonical root configuration, and establishes the stable OPM/ARPE/UDIS/UIF policy required by blocking polling. |
| Grouped base configuration | timebase and counter `Get`/`Set` pairs | Domain structures, grouped staging, and shared apply paths; timebase validation/staging now precedes the MMIO-only commit phase. |
| Timebase scalar access | programmed frequency, PSC, ARR, and CNT functions | PSC/ARR/CNT pairs and `TIM_GetProgrammedTickFrequency` are present. A narrow symmetric programmed-frequency setter is absent; active shadow state remains unobservable. |
| Counter behavior scalar access | DIR, CMS, OPM, ARPE, URS, and digital-filter clock-division pairs | Complete for represented CR1 fields; cross-field compatibility and live-state constraints need strengthening. |
| IRQ/event handling | `TIM_GetIRQSources`, `TIM_SetIRQSources`, `TIM_GetIRQEvents`, `TIM_AckIRQEvents` | Separate DIER-source and SR-event types cover trigger and overcapture vocabulary; NVIC delivery is independent. ISR event Get/Ack paths rely on the initialized gate-lifetime invariant instead of querying RCC per interrupt. Generic acknowledgement rejects input-capture lanes until capture consumption is admitted. |
| Former IRQ compatibility | six `TIM_IRQ_*` wrappers | Removed after consumer migration; no Boolean error-collapsing wrapper remains. |
| Blocking delay | `TIM_BlockingDelayUs`, `TIM_BlockingDelayMs` | Existing consumers justify preservation, but this is a dedicated-Timer service, not a generic peripheral primitive. Applications must allocate the instance through `TIM_ConfigForBlockingDelay` and preserve its configuration; delay calls do not revalidate base configuration. |
| Channel/output compare | none | Accepted-next primitives for the dormant PWM migration are absent despite partial Codec/LL groundwork. |
| Input capture | none | Deferred candidate; consuming-read and overcapture semantics would be required if a named consumer admits it. |
| Master/slave/encoder | none | Deferred candidates; partial selector groundwork does not constitute admitted public scope. |
| Timer-side DMA | none | Deferred peer candidate; partial request Codecs and raw LL access do not establish ownership. |

### Existing Public Symbol Disposition

Closure classifies exported functions plus public semantic type/selector
families; raw register bit macros remain Core evidence rather than individual
Driver API candidates.

| Existing public family | Lifecycle state | Disposition |
| --- | --- | --- |
| Base clock, operation, timebase, counter functions | Implemented; not fully verified | Retain while closing atomicity, transition, concurrency, and documentation gates. |
| `TIM_GetInputClockFrequency` | Implemented; retained | Public observation for policy-layer prescaler calculation after RCC clock-tree stabilization; performs no Timer MMIO and has no Timer clock-gate precondition. |
| `TIM_Config` | Implemented; retained trace evidence open | Independent conjugate of `TIM_DeConfig`; it applies timebase/counter state, leaves IRQ-source and NVIC delivery state unobserved and unchanged, never invokes deconfiguration, and reuses both grouped staging paths before one root-owned ordered commit. |
| Former frequency-setting configuration functions | Removed | `TIM_ConfigTickFrequency`, `TIM_Config1MHz`, `TIM_ConfigBaseTickFrequency`, and `TIM_ConfigBase1MHz` have no canonical replacement; use explicit `tim_config_t` data. |
| `TIM_ConfigForBlockingDelay` | Dedicated service bootstrap | Retain only for the admitted blocking polling-delay service; it validates the fixed 72 MHz kernel-clock contract before delegating `TIM_Config`. |
| Timer IRQ source/event functions | Implemented; evidence open | Retain the four canonical functions and separate source/event types; add retained Codec/MMIO traces before closing the evidence gate. |
| `TIM_BlockingDelayUs`, `TIM_BlockingDelayMs` | Compatibility/service | Move behind dedicated-Timer ownership while retaining temporary wrappers; application-owned allocation through `TIM_ConfigForBlockingDelay` is a precondition rather than runtime configuration discovery. |
| Channel/output selectors and Codecs | Candidate/partly admitted | Admit only the subset required by the dormant PWM migration; keep other selectors dormant. |
| `tim_remap_t` and TIM1/TIM2/TIM3/TIM4 remap selectors | Misowned compatibility debt | Move to AFIO/pin-routing ownership. TIM1 selectors are outside the current Timer instance scope. |
| Frequency-specific default constants | Explicit configuration data | Retain only where a project establishes the documented Timer kernel clock before using the constant; they do not justify frequency-setting functions. |
| Former `tim_irq_enable_t` | Removed | IRQ source staging now uses `DRIVER_STATUS_OFF/ON`. |
| Former `TIM_Get/SetClockDivision` and `TIMx_CKD_CLK_*` selectors | Migrated | Replaced by digital-filter sampling-clock vocabulary around CR1.CKD (`tDTS`). |
| Former `TIMx_UPDATE_SOURCE_OVF_DMA` | Migrated | Replaced by `TIMx_UPDATE_SOURCE_OVERFLOW_UNDERFLOW_ONLY`; URS does not make DMA an update source. |
| Three public pure `timer_defines.h` channel utilities | Raw/public utility candidates | Keep channel-mask iteration only if an admitted public/peer path consumes it; the unused instance-index utility was removed. |
| Master/slave, capture, Hall/XOR, and DMA selector families | Deferred raw/Codec groundwork | Not a public guarantee; reconsider only with named consumers and completed capability evidence. |

### Current Private Helper Inventory

The current private functions form useful categories.

| Category | Existing examples | Assessment |
| --- | --- | --- |
| Topology and peer integration | `_TIM_DecodeRCCBus`, `_TIM_DecodeAPB1ClockEnableMask`, `_TIM_DecodeAPB1PeripheralResetMask`, `_TIM_DecodeIRQ` | Correct ownership; the current TIM2-TIM4 mappings are explicit and use instance-safe address decoding. Public `TIM_GetInputClockFrequency` composes the private bus mapping with RCC observations without Timer MMIO. |
| Validation and preconditions | instance, selector, state, clock-enabled, counter-stopped, IRQ-source/event, and channel-event acknowledge validators | Correct category; broader grouped compatibility, capability, channel, and transaction-wide validation remain incomplete. |
| Register-bound dirty write | `_TIM_WriteCR1IfChanged`, `_TIM_WriteCNTIfChanged` | Correctly delegates comparison/write mechanics to `RegOps_WriteIfChanged` for independent scalar transactions; the ordered base commits compare their already validated images directly. |
| Base-domain staging | `_TIM_StageCounterConfig`, `_TIM_StageTimeBaseConfig`, `_TIM_StageTimeBaseUpdate` | All are MMIO-free, reuse Codec stages, keep independently required images explicit, and publish outputs only after every fallible stage succeeds. Counter staging produces one final CR1 image and rejects direction changes that hardware cannot accept in the current center-aligned mode. |
| Ordered base commit | Public `TIM_Config` and timebase setters | Each public API directly owns the temporary update policy, `EGR.UG`, final CR1 application, and CNT restoration. Grouped/scalar counter setters dirty-write one final CR1 image; root configuration coalesces its counter image into the mandatory timebase sequence and performs no redundant standalone counter write. |
| Runtime state apply | `TIM_SetOperationState` | The public API visibly owns the narrow CR1.CEN Read/Modify/Write transaction; the former one-call private forwarding helper is removed. |

Large transaction helpers should ordinarily be `static` functions. Reserve
`__STATIC_FORCEINLINE` for small leaf helpers where forced inlining has a
measured or compelling reason; otherwise it increases code-size/debug risk
without improving the architecture.

### Immediate Contract Corrections

These corrections precede feature expansion.

#### 1. Enforce failure atomicity

`TIM_SetTimeBaseConfig()` now validates clock availability and stopped-counter
state from its cached CR1 image, stages every timebase/CR1/EGR image locally,
and only then enters its public MMIO commit. `TIM_SetCounterConfig()` follows the
same boundary for its single final CR1 image. It rejects a direction change
that is read-only in the current center-aligned mode and leaves the explicit
mode-transition sequence to the application. `TIM_Config()` reuses both
staging paths and completes every fallible domain transformation before its
first Timer-register write. Retained MMIO trace evidence remains required to
close the verification gate.

Binding rule:

1. Validate every pointer, selector, capability, compatibility rule, clock,
   and required stopped/disabled state.
2. Capture every required image.
3. Perform every fallible Codec stage operation on local images.
4. Hold the declared exclusive application ownership or transaction guard from
   snapshot through commit; do not reread cached configuration registers merely
   to repeat validation.
5. Begin MMIO commit only when no ordinary validation failure remains.
6. After the first write, use a single cleanup path for every possible failure.

No public function may return an ordinary validation/status failure after a
partial commit while claiming unrelated state was preserved.

#### 2. Model the complete update domain

`EGR.UG` can reinitialize CNT according to direction and clear prescaler phase.
When `UDIS=0`, the resulting update event reloads PSC and transfers ARR/CCRx
preloads when their preload controls are enabled. It can also create update-
domain side effects, drive `TRGO` through master mode, and affect DMA request
timing when request-source gating permits. RM0008 defines `URS=1` to suppress
UIF/update interrupt/DMA requests from software UG while still permitting the
update transfer; the current helper uses that rule but lacks a retained MMIO
trace and optional on-target confirmation. Consequently:

- treat ordinary dirty-write necessity and active-shadow commit necessity as
  separate decisions;
- never infer that active state is synchronized merely because the readable
  preload image equals the requested value—the active PSC/ARR/CCR shadow is not
  directly observable;
- define whether a setter updates programmed preload state only or guarantees
  an immediate active commit, and use explicit commit provenance/state when a
  safe skip decision is required;
- include channel preloads, master synchronization, and DMA routing in the
  transaction analysis;
- do not describe a forced update as timebase-local;
- name a generalized private helper `_TIM_CommitPreloadedState` only after its
  full side-effect contract is defined.

#### 3. Separate event, request, and delivery domains

The former `tim_irq_t` was used for both DIER enables and SR flags even though
the hardware sets are not isomorphic. The implemented model now uses distinct
types:

- `tim_irq_source_t` — enable-capable Timer interrupt requests: update,
  capture/compare 1-4, and trigger;
- `tim_event_flag_t` — latched status: update, capture/compare 1-4, trigger,
  and capture overrun 1-4;
- `tim_dma_source_t` — Timer DMA request sources: update,
  capture/compare 1-4, and trigger;
- NVIC enable, pending, and active state — owned by NVIC and IRQ topology, not
  represented as Timer event flags.

Canonical Timer functions are:

```c
TIM_GetIRQSources(...)
TIM_SetIRQSources(...)
TIM_GetIRQEvents(...)
TIM_AckIRQEvents(...)
```

The former `TIM_GetPendingIRQMask()` incorrectly named an SR event-flag getter
as IRQ pending state, and the former `TIM_AcknowledgeIRQ()` unconditionally
cleared NVIC pending state. Their replacements expose SR events explicitly and
never mutate NVIC state. An optional coordinated delivery helper remains
admissible only when the IRQ mapping proves exclusive ownership and aggregates
every source.

A generic event acknowledge must inspect each selected channel's CCMR mode.
By target Driver policy, it rejects `CCxIF` acknowledgement for an input-
capture lane because writing that flag to zero—although hardware-permitted—
discards the unread capture notification.
`TIM_ReadCapturedValue()` owns normal capture consumption; an explicit
`TIM_DiscardCapturedValue()` is admissible only for a named consumer. Read the
captured value before acknowledging its overcapture flag.

#### 4. Keep calculated frequency observation separate from configuration

The hardware exposes the programmed PSC preload, not the active prescaler
shadow. Retain a calculated getter whose name and contract state that limitation:

```c
TIM_GetProgrammedTickFrequency(...)
```

The getter calculates frequency from the live Timer kernel clock and programmed
PSC state. General frequency-targeting setters and presets are rejected;
callers select explicit prescaler and timebase values through @ref tim_config_t
or narrow register-semantic setters. `TIM_ConfigForBlockingDelay()` is the sole
service-specific fixed-frequency exception: it validates the documented
72 MHz input clock and configures only the admitted blocking polling-delay service.

#### 5. Define state-return convention deliberately

Returning `DRIVER_STATUS_OFF`, `DRIVER_STATUS_ON`, or an error directly is an
accepted local convenience only for truly binary state. Use an output pointer
when the result can expand, when multiple observations are returned, or when a
status value and data value should not share one domain. Do not create Boolean
wrappers that turn invalid access into a legitimate OFF result.

#### 6. Move dedicated-Timer conveniences above the core surface

Preserve `TIM_BlockingDelayUs()` and `TIM_BlockingDelayMs()` until consumers migrate, but plan a
`timer_delay` service whose contract explicitly owns a dedicated Timer. The
core Timer API must not imply that a blocking delay preserves an arbitrary
caller's channel, trigger, DMA, or synchronization configuration.

#### 7. Keep clock-gate transitions outside Timer ownership

RCC/application owns every Timer APB1 clock-gate query and mutation. Timer
configuration, deconfiguration, operation, IRQ, and delay APIs validate that
the required gate is already enabled and return `DRIVER_STATUS_ERROR_STATE`
without Timer MMIO when it is not. The application explicitly owns the safe
stop, IRQ/NVIC quiescence, deconfiguration, and final clock-disable sequence.

### Accepted and Candidate Public Families

Only the base corrections plus channel/PWM-output expansion have a
current repository consumer or TODO. Other native Timer families are planned
candidate shapes so their lower-layer requirements and ownership are known,
but remain deferred until a named consumer passes public API admission. Exact
function signatures require per-function contract rows before code is written.

#### Base Timer primitives — retain and correct

- Operation state, lifecycle, grouped timebase, grouped counter, scalar base
  access, and programmed tick-frequency query. Clock-gate control remains an
  RCC/application operation outside the Timer public surface.
- Group configuration remains canonical; scalar setters reuse the same narrow
  staging and hazardous commit primitives without calling a public grouped
  setter and repeating its register snapshots.
- Update-event policy should be a coherent update configuration rather than a
  public function for every CR1 bit. Existing scalar functions may remain as
  documented conveniences.

#### Common channel primitives — accepted next for PWM migration

```c
TIM_GetChannelEnableMask(...)
TIM_SetChannelState(...)
TIM_GetChannelPolarity(...)
TIM_SetChannelPolarity(...)
```

- Configuration and polarity functions accept exactly one validated channel.
- A state mutator may accept a non-empty channel mask so several `CCxE` bits
  can be staged and committed in one CCER write.
- The contract must state that disabling an input channel resets its capture-
  prescaler phase. Existing CCR data and SR flags remain governed by their own
  read/acknowledge semantics.

#### PWM-output primitives — accepted next for PWM migration

```c
TIM_GetPWMOutputConfig(...)
TIM_SetPWMOutputConfig(...)
TIM_GetCompareValue(...)
TIM_SetCompareValue(...)
```

Use `tim_config_pwm_output_t` for PWM1/PWM2 mode and preload.
Stage fast and clear to safe disabled defaults; do not expose those selectors
until a named consumer needs independent control. The getter returns
`DRIVER_STATUS_ERROR_STATE` when the lane is not configured in this admitted
PWM representation; it does not pretend to decode every output-compare mode.
Polarity and runtime enable state remain separately owned common channel
properties unless a full feature transaction explicitly includes them. A mode
configuration setter requires the selected channel to be disabled; it does not
silently glitch an active output by disabling and re-enabling it.

PWM percentage, frequency, GPIO route, and multi-channel sharing policy belong
in the PWM feature driver. PWM uses Timer timebase and PWM-output primitives;
the channel-local Timer configuration must not silently rewrite the shared
timebase. The first migration supports TIM2/TIM3/TIM4 routes only; dormant
legacy TIM1-backed PWM routes remain deferred to advanced-Timer admission.

#### Input capture primitives — candidate, deferred pending consumer

```c
TIM_GetInputCaptureConfig(...)
TIM_SetInputCaptureConfig(...)
TIM_ReadCapturedValue(...)
```

Use `tim_config_input_capture_t` for input selection, input prescaler, digital
filter, and capture polarity. Runtime channel enable remains separate. The
captured-value operation deliberately uses `Read`, not a side-effect-free
`Get`: reading CCR in input mode consumes/clears `CCxIF`. Its result must report
the captured value and overcapture state using the manual-recommended order—
read captured data before observing overcapture—which this Driver adopts as a
contract. No public mode-neutral `SetCaptureCompareValue()` is accepted because
CCR is read-only in capture mode.

#### Master/slave and encoder primitives — candidate, deferred pending consumer

```c
TIM_GetMasterMode(...)
TIM_SetMasterMode(...)
TIM_GetSlaveConfig(...)
TIM_SetSlaveConfig(...)
TIM_GetEncoderConfig(...)
TIM_SetEncoderConfig(...)
```

A complete `tim_config_slave_t` covers slave mode, trigger selection,
master/slave synchronization, and applicable external-trigger polarity,
prescaler, filter, and clock selection. It must represent all eight `SMS`
encodings, not only disabled and encoder modes. Internal trigger availability
is instance-dependent and belongs in Driver topology/capability validation.

Encoder configuration is a Driver transaction across SMCR and channels 1/2;
the “encoder requires two coherent input lanes” rule must not leak into Codec.

#### Timer-side DMA primitives — candidate, deferred pending peer contract

```c
TIM_GetDMARequestEnableMask(...)
TIM_SetDMARequestState(...)
TIM_GetDMABurstConfig(...)
TIM_SetDMABurstConfig(...)
```

Timer owns DIER request generation, CR2 capture/compare DMA selection, DCR
layout encoding, and knowledge of which register-window elements are legal for
read or write. The DMA driver owns controller channel selection, transfer
direction, addresses, count, priority, and operation state. A safe burst setup
therefore needs a coordinated peer contract that supplies transfer direction
to Timer-side window validation; DCR base/count alone is not sufficient. DMAR
is a live DMA portal and is never ordinary Timer configuration state.

#### Semantic actions — admit only with an independent consumer

A broad public `TIM_GenerateEvent()` is valid domain vocabulary for update,
capture/compare, or trigger generation, but current private commit use alone
does not prove an application-facing need. Keep arbitrary action generation
private until a named application or peer consumer passes admission.

The admitted PWM migration does prove a narrower peer need: glitch-free
startup must stage PSC/ARR/CCR preloads and commit them coherently before CEN.
Admit either a Timer-owned feature transaction that performs that sequence or
a peer-service `TIM_CommitPreloadedState()` with an explicit whole-update-domain
contract, exclusive ownership precondition, and no claim of channel-local side
effects. PWM must not abuse a base setter merely to force `UG`.

### Explicit Deferrals and Ownership Boundaries

| Feature | Disposition |
| --- | --- |
| PWM frequency/duty calculations and GPIO routing | PWM feature driver over Timer and GPIO/AFIO services |
| Timer pin remap mutation | AFIO/pin-routing owner; Timer exposes signal/channel identity only |
| DMA controller configuration | DMA driver |
| NVIC priority and generic line management | NVIC/application IRQ integration |
| Basic timers, advanced-control timers, 32-bit counters, reduced channel counts | Capability-gated future variants; do not force into TIM2-TIM4 structures |
| Break/dead-time, repetition counter, complementary outputs, main output enable | Advanced-Timer-specific Driver/Codec/LL extension |
| Raw register-image APIs | LL/diagnostic only; rejected from application Timer API |

Use `tim_config_t` as the root structure of structures for admitted Timer base
configuration. Preserve one nested structure per coherent base transaction,
currently timebase and counter. IRQ-source enablement remains outside the root
object so application intent is explicit through `TIM_SetIRQSources()`.
Deferred domains are absent from the public structure and are preserved by
`TIM_Config()` until admitted. Applications explicitly invoke
`TIM_DeConfig()` when complete reset state is required.

### Private Helper Families by Admission State

| Category | Planned helpers or pattern | Why it belongs in Driver |
| --- | --- | --- |
| Instance topology | bus, gate mask, reset mask, IRQ-source routing | Selects peer resources from the live instance. |
| Capability | supported channel mask, counter mask/width, feature mask | Rejects valid vocabulary unsupported by one instance. |
| Validation | channel/single-channel/mask, grouped config, cross-field compatibility | Owns public and hardware-state policy. |
| Preconditions | clock enabled, counter stopped, channel disabled, DMA/trigger quiescent | Reads live state before a hazardous transition. |
| Register routing | channel-to-CCMR read/write, channel-to-CCR read/compare-write | Driver selects physical register; Codec only interprets its supplied image. |
| Snapshot | capture every image/resource required by one transaction | Prevents repeated volatile reads and defines rollback evidence. |
| Apply/commit | accepted base/channel/output paths; capture/slave/encoder/DMA only if later admitted | Centralizes canonical staging and commit ordering without turning deferred candidates into scope. |
| Hazard isolation | preload commit, W0C acknowledge, capture consume | Makes special semantics explicit even if used by one public API. |
| Cleanup/rollback | restore temporary CR1, IRQ, clock, channel, and ownership state | Prevents assertion-style early returns from skipping restoration. |
| Calculation | input clock, exact/rounded prescaler, frequency/period conversion | Keeps numerical policy above Codec and LL. |

Accepted-next names include the relevant topology/channel/output helpers below;
capture, slave, encoder, and DMA names are placeholders only if their families
later pass admission:

- `_TIM_GetClockEnableMask`, `_TIM_GetResetMask`,
  `_TIM_GetIRQLineForSource`;
- `_TIM_GetSupportedChannelMask`, `_TIM_GetCounterValueMask`,
  `_TIM_ValidateFeatureSupported`;
- `_TIM_ValidateSingleChannel`, `_TIM_ValidateChannelMask`,
  `_TIM_ValidateChannelDisabled`;
- `_TIM_ValidatePWMOutputConfig`,
  `_TIM_ValidateInputCaptureConfig`, `_TIM_ValidateSlaveConfig`,
  `_TIM_ValidateDMABurstConfig`;
- `_TIM_ReadCCMRForChannel`, `_TIM_WriteCCMRForChannelIfChanged`,
  `_TIM_ReadCCRForChannel`, `_TIM_WriteCompareValueIfChanged`;
- `_TIM_ApplyPWMOutputConfig`, `_TIM_ApplyInputCaptureConfig`,
  `_TIM_ApplyChannelState`, `_TIM_ApplySlaveConfig`,
  `_TIM_ApplyEncoderConfig`, `_TIM_ApplyDMARequestState`,
  `_TIM_ApplyDMABurstConfig`, `_TIM_CommitPreloadedState`.

A helper earns existence when it centralizes repeated policy, isolates a
hazardous sequence, resolves topology/capability, implements a transaction or
cleanup path, or materially improves an otherwise unreadable operation. A
helper that merely renames one safe call should not exist.

### Metadata Representation Rule

Convert a validated instance to one stable index, then use the smallest direct
representation for each mapping:

| Mapping shape | Preferred representation |
| --- | --- |
| One property per dense instance index | Direct typed LUT with designated index initializers |
| Several independently consumed properties | Separate typed LUTs |
| Several inseparable values always consumed together | Struct LUT, with the cohesion reason documented |
| Sparse or irregular selection | Fully braced `switch` or explicit mapping table |
| Formulaic bit position | Computation, not stored metadata |
| Capability set | Typed capability-mask LUT |
| Register image or mask | `reg` |
| Address | Typed pointer or `uintptr_t`, not `reg` |

Do not introduce a one-field metadata struct or a “god metadata” record. Assert
each LUT length against `*_INSTANCE_COUNT`, reject the invalid index sentinel
before lookup, and document whether the data describes topology, capability,
encoding, or policy.

### Generic Driver Transaction Templates

#### Side-effect-free getter

```text
validate output -> validate instance/capability -> validate clock
-> snapshot each required register exactly once -> Codec Extract into locals
-> publish outputs only after all extraction succeeds
```

#### Persistent setter

```text
validate all public values -> validate live preconditions -> acquire guard
-> snapshot -> stage every local image
-> dirty-write ordinary state in documented order -> commit preload if needed
-> cleanup/restore -> release guard -> return final status
```

#### Action or acknowledge

```text
validate semantic action -> acquire required guard -> construct action image
from a safe constant or Codec action/ack transform -> unconditional semantic
write -> perform required observation/cleanup -> release guard
```

Never use a read-modify-write or dirty-write template for W0C/W1C, write-only
action, read-clear, FIFO, or data-window registers.

### Driver Completion Gate

- [ ] Every in-scope intent has one canonical public path and named audience.
- [ ] Every public API passes the admission test or is labeled compatibility.
- [ ] Every public API documents accepted/expected values, preconditions,
      side effects, success/failure postconditions, statuses, and concurrency.
- [ ] Every grouped structure represents one coherent hardware transaction.
- [ ] Every scalar mutator preserves unrelated state through a canonical apply
      path.
- [ ] All validation and fallible Codec staging finish before the first write.
- [ ] Every post-write failure uses a guaranteed cleanup path.
- [ ] Programmed/preload state is distinguished from active/effective state.
- [ ] Update generation accounts for every preload, TRGO, DMA, flag, and
      downstream synchronization effect.
- [ ] Event flags, peripheral request enables, and NVIC line state use distinct
      vocabulary and ownership.
- [ ] Channel-disabled, counter-stopped, DMA/trigger-quiescent, and capability
      constraints are enforced where required.
- [ ] Reentrancy, task/ISR races, read-stage-write races, and ownership guards
      are explicit.
- [ ] Every private helper satisfies the helper-admission rule.
- [ ] Peer-driver calls preserve the peer's cache, reference count, lock, and
      shared-resource invariants.
- [ ] No application-facing Timer API exposes a live register pointer,
      register image, bit position, or raw hardware encoding. A declared DMA
      peer service may expose a typed/`uintptr_t` DMAR endpoint only after DMA
      admission; it never exposes a `reg` image or raw field encoding.

## Codec Layer Audit

### Codec Responsibility and Applicability

Codec owns peripheral-specific representation knowledge while remaining a pure
function layer. For Timer, that means selector-to-field encoding, field-to-
selector decoding, channel-lane positioning, reserved encoding rejection,
unrelated-bit preservation, and transactional mutation of caller-owned `reg`
images.

Every Driver transaction must perform a Codec applicability pass, but not every
transaction is required to call Codec.

| Operation shape | Codec expectation |
| --- | --- |
| Persistent semantic configuration | Normally paired `Extract` and `Stage` |
| Read-only semantic status | `Extract` only |
| Write-only action with semantic selector mapping | `Stage<Action>` only, starting from a safe constant image |
| W0C/W1C acknowledgement | `Extract` plus explicit `Stage<Ack>` or `Stage<Clear>`, never an ordinary setter |
| Validated public payload already equals the exact action payload | Codec may be unnecessary; trace row must state why |
| Derived frequency/period/duty arithmetic | Driver/service calculation, not register Codec |
| Composite feature implemented only through peer public APIs | No private Codec or LL required |
| Raw diagnostic operation | No Codec only when raw audience and responsibility are explicit |

Codec is required when public values differ from raw encoding, field placement
is peripheral-specific, a concept spans images, variants require adaptation,
reverse extraction is required, or peripheral masks are needed to preserve
unrelated bits.

### Generic Derivation Rule

For every contract-frozen Driver function:

1. List the semantic values accepted or returned.
2. List the register images the Driver snapshots or commits.
3. For each semantic-to-image conversion required by `GetX`, derive
   `Codec_<PERIPH>_ExtractX`.
4. For each image mutation required by `SetX` or `ConfigX`, derive
   `Codec_<PERIPH>_StageX`.
5. For a coherent grouped Driver transaction, prefer a grouped Codec pair over
   an exported function for every bit.
6. Export a scalar Codec only when a Driver scalar API consumes it or multiple
   grouped Codecs genuinely share it; otherwise keep it private.
7. Make persistent concepts symmetric. Record deliberate asymmetry for
   actions, status, acknowledgements, FIFO/data-window access, and consuming
   reads.
8. Validate all inputs and stage into local copies before publishing any caller
   output or image.
9. Do not export a Codec merely because the raw register field exists.

### Current Timer Codec Inventory

The current Timer Codec declares 47 external `Codec_TIM_*` functions and uses
30 private static helpers.

- 31 APIs are called directly by the current Driver and 2 more participate
  transitively through grouped timebase staging: timebase, CR1/update handling,
  and DIER/SR event/IRQ handling.
- 14 APIs are forward groundwork with no current Driver consumer: master/slave,
  channel/output-compare groundwork, channel state/polarity, and DMA request
  staging.

The boundary itself is sound: Codec operates on caller-owned images and has no
MMIO, Timer instance, RCC, or NVIC access. Its current surface covers the
transformations used by the existing Driver, but grouped failure publication,
accepted-next CCR/PWM-output coverage, and status vocabulary are incomplete;
retained executable Codec test evidence is absent. Deferred feature breadth
remains dormant by design.

### Current Symmetry and Coverage Matrix

| Register or concept | Current Codec surface | Driver demand | Verdict |
| --- | --- | --- | --- |
| `PSC` | prescaler Extract/Stage | Current getter/group | Complete |
| `ARR` | auto-reload Extract/Stage | Current getter/group | Complete |
| `CNT` | counter value Extract/Stage | Current getter/setter/group | Complete |
| `PSC+ARR+CNT` | grouped timebase Extract/Stage | Current root configuration | Complete surface |
| `CR1.DIR/CMS/OPM/ARPE/URS/CKD` | scalar pairs plus grouped counter pair | Current counter functions | Complete surface; grouped extraction publication must be fixed |
| `CR1.CEN` | state Extract/Stage | Current operation state | Complete |
| `CR1.UDIS` | inverted update-event state pair | Private commit/delay | Complete but not proof of a public bit API |
| `CR2.MMS` | master-mode pair | No Driver consumer | Dormant, complete for MMS |
| `CR2.CCDS` | none | Deferred DMA candidate | Missing if a DMA peer contract is accepted |
| `CR2.TI1S` | none | Deferred Hall/XOR-input candidate | Missing only if a named Hall/XOR consumer is accepted |
| `SMCR.SMS` | slave-mode pair | No Driver consumer | Dormant and only four of eight hardware modes |
| Remaining `SMCR` | none | Future trigger/external-clock/sync | Missing |
| `EGR.UG` | update generation Stage | Private preload commit | Correct Stage-only action |
| `EGR.CCxG/TG` | none | No approved consumer | Defer until an action API passes admission |
| `CCMRx.CCxS` | selection pair | Internal output decode/stage composition | Representation exists; standalone staging is unsafe across mode interpretation |
| `CCMRx` output view | output-compare Extract/Stage with four scalar outputs/inputs | Accepted-next PWM migration | Dormant generic foundation; add a narrow typed PWM-output pair without claiming all modes are admitted |
| `CCMRx` input view | none | Deferred input-capture candidate | Missing if that candidate is admitted |
| `CCER.CCxE` | single-channel state pair | Accepted channel family | Present, but mask-level Driver operation needs one-image staging support |
| `CCER.CCxP` | single-channel polarity pair | Accepted channel family | Present |
| `CCR1..CCR4` | none | Accepted-next PWM compare value; deferred capture value | Blocking the accepted-next channel/PWM slice |
| DIER IRQ sources | typed mask Extract/Stage | `TIM_GetIRQSources` / `TIM_SetIRQSources` | Complete for update, CC1-4, and trigger sources |
| DIER DMA sources | mask Extract/Stage | Deferred DMA candidate | Partial: trigger DMA absent |
| SR event flags | typed event Extract/Ack | `TIM_GetIRQEvents` / `TIM_AckIRQEvents` | Complete vocabulary for update, CC1-4, trigger, and overcapture; generic input-capture acknowledgement is intentionally rejected |
| `SR.UIF` | dedicated Extract/Clear pair | Private update/delay | Redundant with canonical event-mask semantics |
| `DCR` | none | Deferred DMA-burst candidate | Missing if a DMA peer contract is admitted |
| `DMAR` | none | Live DMA endpoint | Correct unless an explicit CPU data API is approved |

### Codec Derived for Accepted and Deferred Driver Families

#### Common channel and CCR value

Add a single semantic CCR value type, for example
`tim_capture_compare_value_t`, and one reusable pair:

```c
Codec_TIM_ExtractCaptureCompareValue(...)
Codec_TIM_StageCaptureCompareValue(...)
```

All four CCR registers have the same 16-bit representation. Driver selects the
physical CCR register; Codec must not duplicate four channel-specific value
pairs.

If `TIM_SetChannelState()` accepts a channel mask, add a mask-level CCER stage
operation so all selected `CCxE` bits are changed in one local image and one
commit. Keep the single-channel state pair only if an accepted Driver or grouped
Codec still consumes it.

#### PWM output

Replace the four optional output-pointer/input-parameter contract with:

```c
Codec_TIM_ExtractPWMOutputConfig
(
	const reg ccmrRegImage,
	const tim_channel_t channel,
	tim_config_pwm_output_t* const pConfig
);

Codec_TIM_StagePWMOutputConfig
(
	reg* const pCcmrRegImage,
	const tim_channel_t channel,
	const tim_config_pwm_output_t* const pConfig
);
```

For the admitted PWM migration, the structure owns PWM1/PWM2 mode and preload;
fast and clear are staged to disabled defaults. General output-compare modes
and independently configurable fast/clear fields remain dormant until a named
consumer admits them. The Extract operation rejects other live OC modes rather
than fabricating a partial round trip. CCR value and CCER state/polarity use
their respective Codecs.

#### Input capture

Add `tim_input_capture_prescaler_t`, `tim_input_capture_filter_t`, and
`tim_config_input_capture_t`, then derive:

```c
Codec_TIM_ExtractInputCaptureConfig
(
	const reg ccmrRegImage,
	const reg ccerRegImage,
	const tim_channel_t channel,
	tim_config_input_capture_t* const pConfig
);

Codec_TIM_StageInputCaptureConfig
(
	reg* const pCcmrRegImage,
	reg* const pCcerRegImage,
	const tim_channel_t channel,
	const tim_config_input_capture_t* const pConfig
);
```

The grouped pair owns `CCxS`, `ICxPSC`, `ICxF`, and capture polarity across
caller-owned CCMR and CCER images and publishes both staged images only after
all validation succeeds. Runtime enable remains separate. The first admitted
capture slice accepts direct and indirect inputs only; `CCxS=TRC` remains
deferred until a trigger configuration/consumer is admitted. The generic CCR
extractor decodes the consumed captured value; read-clear ordering and
overcapture handling stay in Driver.

#### Master, slave, trigger, and encoder support

- Retain the complete `CR2.MMS` pair when the public master-mode pair is added.
- Extend `tim_slave_mode_t` to all eight SMS modes before exposing a generic
  slave configuration.
- Add transformations for `SMCR.TS`, `MSM`, `ETF`, `ETPS`, `ECE`, and `ETP`
  only after a slave/trigger consumer is admitted.
- Add `CR2.TI1S` only for a named Hall/XOR-input consumer; it is not required
  by ordinary quadrature encoder mode.
- Prefer grouped trigger/external-trigger structures where fields have
  compatibility constraints.
- Keep the rule “encoder uses two coherent input channels plus SMS” in Driver;
  Codec only stages each supplied image.

#### IRQ, event, and DMA vocabularies

- Change IRQ enable Codecs to `tim_irq_source_t` and include `DIER.TIE`.
- Change event Extract/Ack Codecs to `tim_event_flag_t` and include `TIF` and
  `CC1OF..CC4OF`.
- Change DMA enable Codecs to `tim_dma_source_t` and include `DIER.TDE`.
- Use `DRIVER_STATUS_OFF/ON` consistently for binary stage state; the separate
  `tim_irq_enable_t` conversion has been retired.
- Keep one canonical SR event Extract/Ack implementation. UIF convenience
  helpers may be private wrappers over it.

For W0C acknowledgement, stage write-ones for every unselected writable flag
and write-zero only for selected acknowledgements. This preserves unrelated
flags that appear after the snapshot. The same-source race remains inherent:
an event arriving on a selected source between observation and acknowledge can
be lost, so Driver must document quiescence or service-loop behavior.

#### DMA burst

If a DMA peer contract is admitted, add a grouped DCR pair whose semantic
layout contains:

- a semantic DMA burst base-register selector; and
- transfer count `1U..18U`.

Codec owns DBA translation and the hardware `DBL = transferCount - 1U`
encoding. Transfer direction is not stored in DCR and therefore is not part of
the Extract/Stage round trip; Driver receives it from the DMA peer and validates
every register in the selected window against read/write/action semantics
before staging DCR. `CR2.CCDS` gets a symmetric pair only if the accepted DMA
contract owns capture/compare request timing. DMAR remains a live LL endpoint/
address, not a staged configuration image.

### Current Codec Defects To Mitigate

1. **Grouped extraction is not consistently transactional.**
   `Codec_TIM_ExtractCounterConfig()` publishes members before the final CKD
   decode can reject a reserved encoding. Decode into a local structure and
   assign `*pCounter` only after every extraction succeeds.
2. **Standalone `CCxS` staging can reinterpret stale shared bits.** Switching
   a lane between output and input changes the meaning of the overlapping
   OC/IC fields. Keep raw selection staging private or require grouped output/
   input Codecs to establish every field in the selected interpretation.
3. **The current CCMR output extraction shape is fragile.** Four optional
   output pointers prevent simple transactional publication and scale poorly.
   Use one typed config destination.
4. **Repeated channel-position switches obscure the lane formula.** Derive one
   validated CCMR lane base plus field offsets and one CCER lane base plus
   offsets when the formula is clear; do not retain seven independent mapping
   helpers or replace them with an unnecessary LUT.
5. **Binary state type inconsistency is mitigated.** IRQ source staging uses
   the repository's ON/OFF state contract and `tim_irq_enable_t` is removed.
6. **IRQ/event decoded return types are corrected.** DIER/SR semantic decoders
   return `tim_irq_source_t` and `tim_event_flag_t`; the deferred DMA decoder
   still uses its existing type until DMA admission.
7. **SR semantics are duplicated.** Dedicated UIF and generic IRQ flag paths
   should share one event implementation.
8. **Exported surface is ahead of approved consumers.** Keep admitted-next
   groundwork explicitly dormant; make purely speculative scalar Codecs
   private or defer them.
9. **Retained host coverage is absent.** The prior host tests exercised only a
   small subset of exported Codec behavior, and their repository-local artifact
   is intentionally not retained. Future waves need fresh executable evidence
   for current and new contracts.
10. **Doxygen is not yet at repository preference.** Codec declarations still
    contain legacy `@returns` and `@retval` forms; run the exact documented pass
    when that layer is implemented.

### What Must Never Enter Codec

- `TIM_TypeDef*`, live register pointers, or MMIO.
- Timer instance, topology, or capability LUT validation.
- RCC clock, reset, bus frequency, or NVIC decisions.
- AFIO/GPIO route or DMA-controller configuration.
- Physical register selection such as CCMR1 versus CCMR2 or CCR1 versus CCR4.
- Counter-running, channel-enabled, trigger-active, or DMA-active checks.
- Busy/timeout policy, polling, locking, or critical sections.
- Preload/event commit sequencing or dirty-write decisions.
- Frequency, period, duty, rounding, approximation, or clamping policy.
- Cross-register feature policy such as encoder channel requirements.
- Compatibility wrappers, application presets, or silent selector fallback.

Codec may validate representation-level facts only: null caller storage,
semantic selector vocabulary, channel-mask shape needed for lane placement,
raw reserved encodings, and field/range fit.

### Codec Verification Matrix

For each Extract/Stage pair, test:

- every accepted selector and numeric boundary;
- every rejected selector, reserved raw encoding, and null pointer;
- Extract(Stage(value)) round trip;
- Stage(Extract(image)) canonical round trip where hardware aliases permit;
- preservation of every unrelated bit in random and boundary images;
- no caller-image mutation on failed Stage;
- no destination publication on failed grouped Extract;
- every channel lane and CCMR1/CCMR2-relative position;
- W0C/W1C preserve masks and action asymmetry;
- grouped multi-image all-or-nothing behavior;
- type-width behavior for every supported instance capability.

Codec tests are pure host tests. They must not require a fake peripheral or
clock tree.

### Codec Completion Gate

- [ ] Every contract-frozen Driver transformation has exactly one Codec path or a
      written no-Codec reason.
- [ ] Every exported Codec has a current or admitted-next Driver consumer.
- [ ] Persistent semantic concepts have Extract/Stage symmetry.
- [ ] Actions, status, acknowledgements, and consuming reads use deliberate
      asymmetric verbs.
- [ ] Codec contains no peripheral pointer, volatile I/O, topology, policy,
      sequencing, or peer-driver access.
- [ ] All validation finishes before caller-owned images or outputs change.
- [ ] Multi-field and multi-image operations publish atomically on success.
- [ ] Unrelated and reserved bits are preserved according to the register
      write contract.
- [ ] Raw register images never cross the public Driver API boundary;
      Driver-to-Codec calls use public domain values plus private caller-owned
      `reg` images.
- [ ] Channel interpretation is coherent; stale OC/IC fields cannot survive a
      mode switch unnoticed.
- [ ] Every function passes the pure verification matrix.
- [ ] Header/source Doxygen and logic comments follow
      `AGENTS.md` (Part 1) exactly.

## LL Layer Audit

### LL Responsibility

Timer LL binds a validated `TIM_TypeDef` instance to one physical register and
performs one mechanical read, write, or address formation. LL does not know why
the operation occurs, what a public selector means, whether the clock is on,
whether a channel is enabled, or whether several accesses form a transaction.

The correct derivation for each approved Driver operation is:

1. Identify every hardware observation and mutation in its frozen transaction.
2. Identify the register images supplied to or produced by Codec.
3. Classify each register's access semantics from the device header and
   reference manual.
4. Add a named `Read<REG>` only when a snapshot is required and reading is
   legal.
5. Add a named `Write<REG>` only when state/action commit is required and
   writing is legal.
6. Add an address accessor only for a genuine RegOps or peer-driver integration
   need.
7. Keep raw complete-map access explicitly classified as LL foundation; never
   let it create Driver scope.

### Current Timer LL Inventory and Verdict

The current LL exposes:

- `LL_TIM_REG` and `LL_TIM_SCALAR_REG` address adapters;
- generic `LL_TIM_ReadRegister` and `LL_TIM_WriteRegister` transfer aliases;
- named read/write pairs for CR1, CR2, SMCR, DIER, SR, EGR, CCMR1, CCMR2,
  CCER, CNT, PSC, ARR, CCR1-CCR4, DCR, and DMAR.

This covers the complete TIM2-TIM4 register block. No current base or
accepted-next channel/PWM family is blocked by a missing full-register route.
The gaps are access-direction accuracy, special-register semantics, and
classification—not register count. Dormant raw coverage does not prove that a
deferred family is admitted.

| LL surface | Classification | Required action |
| --- | --- | --- |
| CR1, DIER, SR, EGR, CNT, PSC, ARR | Current demand | Retain legal directions; correct semantic documentation and transaction usage. |
| CCMR1/2, CCER, CCR1-4 | Accepted-next channel/PWM foundation | Retain and correct for the admitted migration slice. |
| CR2 and SMCR | Dormant raw foundation | Retain as complete-map access; master/slave/capture-trigger demand remains deferred. |
| DCR and DMAR | Dormant raw DMA foundation | Retain but document DCR as state and DMAR as a live transfer portal; no DMA API is admitted. |
| `LL_TIM_ReadEGR` | Invalid | Remove; EGR is write-only/action-triggering. |
| Generic register aliases | Deliberate LL convention | Keep only with one consistent checked or unchecked contract; never silently turn invalid access into plausible data. |

### Current Driver Demand

| Register | Current use |
| --- | --- |
| `CR1` | Operation state, counter configuration, stopped checks, preload commit, delay |
| `DIER` | Timer interrupt-source state |
| `SR` | Event observation and W0C acknowledgement |
| `EGR` | Write-only software update action |
| `CNT` | Counter observation/mutation and delay |
| `PSC` | Timebase snapshot and buffered prescaler staging |
| `ARR` | Timebase snapshot/staging and delay reload |

CCMR1/2, CCER, and CCR1-4 are demanded by the accepted-next channel/PWM
migration. CR2/SMCR synchronization routes and DCR/DMAR DMA routes remain
dormant raw foundation until their candidates are admitted.

### Register Semantic Matrix

This matrix must be completed for every future peripheral, not only Timer.

| Timer register/class | Access and hardware behavior | Allowed LL/RegOps mechanics | Driver obligation |
| --- | --- | --- | --- |
| `CR1` | RW, but CEN may clear in OPM and DIR changes in center/encoder operation | Snapshot/full write; dirty write only under transaction ownership | Prevent stale read-stage-write from overwriting hardware/ISR changes; enforce CEN/CMS/DIR rules. |
| `CR2`, `SMCR`, `DIER`, `CCMR1/2`, `CCER`, `DCR` | Ordinary RW configuration with cross-field/runtime constraints | Snapshot, pure stage, full/dirty write | Validate capability, compatibility, stopped/disabled state, and shared-source ownership. |
| `CNT` | Live RW counter state | Read/write; dirty write only when live-counter race is excluded | Define whether mutation while running is supported and what snapshot coherence means. |
| `PSC` | Always buffered until update | Read programmed preload; full/dirty write | Distinguish programmed from effective division and own latch sequencing. |
| `ARR` | Immediate or buffered according to ARPE; ARR=0 blocks counting | Read programmed image; full/dirty write | Define active/preload semantics and update requirement. |
| output-mode `CCR1-4` | RW compare value; may be preloaded | Read/write; dirty write under output-mode contract | Validate output mode and define immediate versus update-latched effect. |
| input-mode `CCR1-4` | Read-only captured data; read clears `CCxIF` | One consuming read; never write or dirty-write | Provide mode-aware `ReadCapturedValue`; adopt the manual-recommended data-before-overcapture observation order as the Driver contract. |
| `SR` | W0C flags, including overcapture | Snapshot plus explicit Ack image and unconditional semantic write | Never generic RMW/dirty-write; protect unselected flags and document selected-source race. |
| `EGR` | Write-only action; bits self-clear | Construct image from zero and direct write only | Never read, RMW, toggle, or dirty-compare; account for all generated side effects. |
| `DMAR` | Indexed DMA transfer portal with a 16-bit transfer payload, not stable 32-bit state | Typed 16-bit data/address operation only | Never cache or dirty-write; DMA peer owns direction, PSIZE=16-bit, compatible MSIZE/alignment/conversion, and transfer sequencing. |

The local reference manual confirms that reading CCR in input mode clears
`CCxIF`, `CCxS` changes require `CCxE=0`, SR is W0C, and EGR is a write-only
action register. Those behaviors must appear in LL Doxygen and in the Driver
contract; a C qualifier alone cannot express the full policy.

### Generic Versus Named LL Access

Use a named LL accessor when it communicates the exact register, legal access
direction, raw member path, or alias:

```c
LL_TIM_ReadCR1(...)
LL_TIM_WriteEGR(...)
LL_TIM_ReadCCR3(...)
```

Use a generic shared operation for behavior that is truly
peripheral-independent:

- complete register transfer;
- pure image field staging/extraction;
- ordinary-state compare-and-write;
- raw set/clear/toggle only where the access semantics permit it.

The repository may retain `LL_TIM_ReadRegister` and
`LL_TIM_WriteRegister` as thin LL aliases over RegOps, consistent with the
coding preference. It must choose one honest contract:

1. **Checked:** return/propagate `driver_status_t` and use an output pointer; or
2. **Unchecked:** declare a validated non-null pointer precondition and perform
   exactly one direct volatile transfer.

Do not call a checked RegOps function, discard its error, and return zero from a
failed read as though it were a valid hardware image. Named LL functions must
follow the same chosen contract.

Do not add policy names such as `LL_TIM_EnableCounter`,
`LL_TIM_ClearIRQ`, `LL_TIM_SetChannelMode`, or a selector-aware
`LL_TIM_ReadCCR(TIMx, channel)`. Driver validates and routes the public channel;
Codec translates its lane; LL accesses the named register.

### Special RegOps Restrictions

`RegOps_WriteIfChanged`, `RegOps_Set`, `RegOps_Clear`, `RegOps_Toggle`, and
masked RMW helpers are valid only for ordinary state registers whose write has
no effect when repeated. Their contracts must explicitly forbid:

- W0C/W1C status registers;
- write-only action/command registers;
- read-clear registers;
- FIFO, data, and DMA portals;
- registers where writing the same value retriggers an action;
- stale images of hardware-modified state unless the caller holds ownership.

A compare-and-skip is wrong for an action whose equal-value write still matters
and can suppress a required acknowledgement or command.

### CCR Raw-Map Contract

The current named CCR reads use the output alias even though the LL function is
mode-neutral. Correct the raw map and LL contract so reads use a readable or
neutral view and writes use the writable output view. Suitable options are:

- `_I CCx_IN` plus `_IO CCx_OUT`; or
- a carefully documented neutral full-register view with mode-dependent
  permissions.

Do not create separate physical LL “capture” and “compare” registers; they are
the same address. The Driver provides semantic separation and prevents writes
in input mode.

Likewise, declare EGR with a write-only `_O` full-register view and remove every
read path.

### LL Verification

Add compile-time and fake-register evidence:

- `offsetof` and size assertions for every register member and reserved gap;
- compile-time width/alignment checks for the supported MMIO access size;
- fake-register tests showing that each named accessor touches the intended
  member exactly once;
- tests proving write-only registers are never read;
- tests proving W0C/action/data-window paths bypass generic dirty/RMW helpers;
- const-correctness checks for read and write directions;
- documentation checks for every access precondition and side effect.

### LL Completion Gate

- [ ] Every contract-frozen Driver API has a register-demand row.
- [ ] Every required observation/mutation has one legal named LL route.
- [ ] Every extra accessor is classified as accepted raw foundation or removed.
- [ ] Codec includes/calls no LL function.
- [ ] Driver does not directly dereference Timer members outside approved LL
      address binding.
- [ ] Each LL function performs exactly one read, write, or address formation.
- [ ] LL contains no public validation, selector mapping, state transition,
      batching, polling, clock/reset, NVIC, or peer-driver policy.
- [ ] Access direction matches the reference manual and raw C qualifier.
- [ ] Write-only/action registers have no read accessor.
- [ ] W0C/W1C/action/read-clear/FIFO/data-window registers never use generic
      RMW or dirty-write primitives.
- [ ] Buffered registers have Driver-owned commit semantics.
- [ ] Mode-dependent registers have mode-aware Driver contracts.
- [ ] Generic LL/RegOps error handling follows one documented model and never
      fabricates plausible data.
- [ ] Register layout assertions and one-access fake-register tests pass.
- [ ] Full images use `reg`, addresses use typed pointers/`uintptr_t`, and LL
      Doxygen follows the repository preference.

## Cross-Layer Traceability

### Resource Ownership Matrix

| Resource or decision | Owner | Allowed consumers | Forbidden shortcut |
| --- | --- | --- | --- |
| Raw register layout, offsets, masks, access qualifiers | Core/device header | LL and Codec masks/types | Driver inventing duplicate raw encodings |
| Domain selectors and config structures | Peripheral public defines/types/config | Driver and Codec | LL accepting public selector policy |
| Timer MMIO register selection | Timer LL; channel-to-register routing in private Timer Driver | Timer Driver | Feature driver calling Timer LL directly |
| Selector/image translation | Timer Codec | Timer Driver | Translation embedded in LL or repeated in Driver |
| Timer sequencing and validation | Timer Driver | Application and feature drivers through public APIs | Codec/LL deciding clock, stopped, enabled, busy, or rollback policy |
| Generic field/image and safe ordinary-register mechanics | RegOps | Codec, LL, and Driver under semantic restrictions | Per-peripheral copies of generic mechanics |
| Clock gate, reset, and bus/kernel frequency service | RCC Driver | Timer Driver through RCC public/service API | Timer LL changing RCC state or bypassing RCC cache/policy |
| NVIC enable, priority, pending, active, shared-vector aggregation | NVIC/application IRQ integration | Optional Timer integration through declared mapping | Treating Timer SR flags as NVIC pending state |
| Pin mode and AFIO remap | GPIO/AFIO or board pin-routing layer | PWM/capture/encoder feature integration | Timer Codec/LL modifying GPIO/AFIO |
| DMA controller/channel transfer | DMA Driver | Timer-side DMA request service | Timer Driver owning DMA memory address/count/priority |
| PWM duty/frequency and shared-channel policy | PWM feature driver | Application | Timer scalar API silently rewriting all-channel timebase |
| Timer topology and capability data | Private Timer Driver | Timer validation/routing helpers | One-field/god metadata structs or public raw topology leakage |

Driver-to-peer public/service APIs are preferred whenever the peer owns cached
state, reference counts, locks, shared-resource arbitration, or safety
sequencing. A direct foreign-register access requires an explicit ownership
decision; “no convenient peer API exists” is not sufficient justification.

### Timer Family Traceability Matrix

This family matrix proves the architecture. Implementation work must expand
each accepted family into one row per public function before code is written.

| Driver family / intent | Private Driver path | Codec dependency | LL / peer dependency | Current disposition |
| --- | --- | --- | --- | --- |
| Clock-gate precondition | instance-to-gate mapping and enabled-state validation only | None; exact peer-service observation | RCC gate-state query | Present; public mutation/query remain outside Timer and under RCC/application ownership |
| Base lifecycle configure/deconfigure | validate, independent application-ordered lifecycle, stopped-state guard, reusable grouped staging, ordered commit/reset | timebase/counter and update action | CR1, PSC, ARR, CNT, EGR, and RCC reset pulse | Both APIs require an enabled application-owned gate, leave gate/NVIC state unchanged, and never invoke each other; retained atomicity/failure traces remain open |
| Counter operation state | clock guard, apply state | CEN Extract/Stage | CR1 | Present |
| Timebase grouped/scalar | stopped/ownership guard, snapshot, stage, preload commit | PSC/ARR/CNT grouped/scalar | CR1, PSC, ARR, CNT, EGR, SR | Present; source ordering mitigates partial-write failure, while whole-update-domain scope and retained trace evidence remain open |
| Counter behavior grouped/scalar | whole-mode compatibility and stopped guard | CR1 grouped/scalar | CR1 and SMCR observation where mode-dependent | Present surface; cross-field validation incomplete |
| Programmed tick-frequency query/set | input-clock derivation, exact/rounded divider policy | PSC value | PSC; RCC frequency/prescaler service | Present under broader preset naming; active shadow is unobservable, so split preload state from commit behavior |
| IRQ source enable | validate source/capability, DIER transaction | IRQ-source mask Extract/Stage | DIER; NVIC only in explicit integration | Present under conflated type; trigger source missing |
| Event observation/ack | event validation, mode guard, W0C action | event flag Extract/Ack | CCMR1/2 and SR | Present under IRQ names; trigger/overcapture missing; target Driver policy rejects generic input-mode CCxIF discard |
| Channel enable mask | validate supported channel mask, one CCER transaction | channel enable mask Extract/Stage | CCER | Driver missing; Codec needs mask-level support |
| Channel polarity | validate one channel/disabled-state policy | polarity Extract/Stage | CCER | Codec/LL present; Driver missing |
| Output compare config | channel-disabled guard, CCMR routing, grouped apply | typed PWM1/PWM2 mode/preload config; fast/clear disabled | CCMR1/2 | Accepted-next PWM slice; broader OC selectors deferred |
| Output compare value | validate output mode, CCR routing, preload policy | capture/compare value Extract/Stage | CCR1-4, optional controlled update | CCR codec and Driver missing |
| Coherent preload commit | exclusive-owner/stopped guard, whole-update-domain sequence | update action staging | CR1/CR2/DIER/EGR/SR plus PSC/ARR/CCR context | Accepted peer need for glitch-free PWM startup; exact contract not frozen |
| Input capture config | channel-disabled guard, CCMR routing, grouped apply | typed input config | CCMR1/2, CCER | Deferred candidate; Driver/Codec missing, LL present |
| Captured value consume | validate input mode, ordered CCR/overcapture observation | CCR value extract plus event flag decode | CCR1-4 consuming read, SR | Deferred candidate; side-effect contract required |
| Master mode | validate capability and downstream transition policy | MMS Extract/Stage | CR2 | Deferred candidate; Codec/LL dormant |
| Slave/trigger config | topology/cross-field validation, quiescence, grouped apply | complete SMCR group | SMCR, optional CR2 | Deferred candidate; partial SMS Codec only |
| Encoder config | two-channel plus SMCR composite transaction | input-lane and SMCR transformations | CCMR1, CCER, SMCR, CR1/CNT | Deferred candidate; Driver composite and Codec coverage missing |
| DMA request state | source/capability validation | DMA-source mask Extract/Stage | DIER | Deferred candidate; partial Codec/LL groundwork |
| DMA burst integration | direction-aware window validation, DCR apply, DMA-peer address service | DCR layout Extract/Stage | DCR; DMAR address/data portal; DMA peer | Deferred candidate; requires coordinated peer contract before Driver/Codec |
| Dedicated blocking delay | dedicated-resource ownership, polling, cleanup | existing base/update transformations | CR1, SR, CNT, ARR; Timer public base APIs | Present; relocate to service and define full ownership |

### Finding and Hazard Ledger

`H-*` identifies a hardware/implementation finding. Severity describes current
impact only when the state is `[Gap]`; `[Deferred]` rows are admission
prerequisites, not present defects. `[Observed]` rows preserve verified good
behavior that later work must not regress.

| ID | Severity/state | Verified hazard | Mandatory mitigation |
| --- | --- | --- | --- |
| H-001 | Critical `[Mitigated]` | Grouped counter/timebase setters and root `TIM_Config` now validate the stopped counter and complete every represented Codec/Driver stage before entering MMIO-only ordered commit phases. | Retain the ordering and add no-write-on-error MMIO trace tests. |
| H-002 | Critical `[Gap]` | `EGR.UG` affects more than timebase: it reinitializes CNT direction-dependently, clears prescaler phase, and, when update events are enabled, can transfer preloads and affect TRGO/synchronized consumers. DMA timing can be affected when request gating permits; RM0008 supports the current `URS=1` suppression intent, but no retained MMIO trace/on-target verification proves the implementation sequence. Readable preload equality cannot prove active-shadow synchronization. | Model the whole update domain; separate dirty-write from commit necessity; quiesce/account for every consumer and trace URS/UDIS behavior. |
| H-003 | High `[Mitigated]` | `TIM_SetIRQSources` covers UIE/CC1-4/TIE and no longer derives or mutates NVIC state from DIER. TIM2-4 vectors are dedicated today; cross-peripheral aggregation becomes mandatory only for future shared-vector variants. | Retain Timer/NVIC ownership separation and require an explicit aggregation contract before shared-vector variants are admitted. |
| H-004 | High `[Mitigated]` | `TIM_GetIRQEvents` reports SR flags explicitly and `TIM_AckIRQEvents` performs no NVIC mutation. | Retain distinct event, request-source, and delivery contracts; add retained trace evidence. |
| H-005 | Deferred `[Deferred]` | If capture is admitted, input CCR read clears `CCxIF`, input CCR is read-only, and overcapture needs explicit vocabulary. | Require a mode-aware capture-consume API returning value/overcapture with tested read order; reject a generic public CCR setter. |
| H-006 | High `[Mitigated]` | EGR now uses a write-only raw view and has no LL read; input CCR aliases are read-only; DMAR is documented as a 16-bit transfer portal. | Retain these access classes and add layout/one-access tests before admitting capture or DMA. |
| H-007 | High `[Gap]` | Current DIR/CMS scalar transitions do not establish a complete CR1/SMCR mode contract, including hardware/raw encoder-state interactions. | Validate the complete current counter-mode context before staging DIR/CMS changes. |
| H-008 | High `[Gap]` | Read-stage-write sequences can race ISR/task/DMA/hardware updates; NVIC disable does not stop active handlers, triggers, DMA, or other callers. | Enforce single owner or per-instance transaction guard plus feature-specific quiescence; document ISR eligibility. |
| H-009 | Medium `[Gap]` | Early-return assertion macros inside temporary-state transactions create a structural cleanup hazard if a post-mutation call becomes fallible; no currently reachable cleanup skip has been proven. | Use acquire/mutate/single-cleanup/restore flow before adding fallible steps and define failure postconditions. |
| H-010 | Medium `[Partially mitigated]` | `TIM_GetProgrammedTickFrequency` and base-preset names now expose programmed/broad state, but active PSC/ARR shadow state remains unobservable. | Preserve programmed-versus-effective wording and apply the same admission rule to future CCR preloads. |
| H-011 | Medium `[Gap]` | Multi-register getters can mix hardware moments while the Timer is live. | Declare non-coherent sampling or hold a valid hardware/software snapshot guard. |
| H-012 | Scope `[Observed]` | Current topology intentionally assumes TIM2-4, four 16-bit channels, APB1, and dedicated vectors, matching the declared target. | Preserve this boundary; require capability data before any future variant is admitted. |
| H-013 | Good `[Observed]` | Current W0C Codec writes ones to unselected writable flags. | Retain this preservation rule; separately document the selected-source arrival race. |
| H-014 | Deferred `[Deferred]` | If channel/capture/PWM/sync families are admitted, SMS, CCxS, CCxE, input-prescaler reset, and preload rules impose additional cross-field/live-state constraints. | Make each constraint an admission and transaction gate; do not count absent future validation as a current bug. |
| H-015 | High `[Mitigated]` | Timer previously exposed clock-gate mutation that could pause an active request/trigger path. | Public Timer clock APIs are removed; RCC/application explicitly owns safe clock-gate sequencing. |
| H-016 | Accepted-next `[Gap]` | The admitted PWM path needs a coherent PSC/ARR/CCR preload commit, but no public/peer transaction currently provides it without abusing a base setter. | Freeze a whole-domain preload commit or coherent feature transaction before PWM implementation. |
| H-017 | Medium `[Gap]` | Core DMA-channel comments conflict with RM0008 Table 78 for Timer assignments, so repository metadata is not uniformly trustworthy. | Correct and test the Core route metadata against the authoritative table before admitting Timer-DMA integration. |

### Transaction and Concurrency Contract

Every mutating API must fill these fields before implementation:

| Contract field | Required statement |
| --- | --- |
| Ownership | Whether the API owns or merely borrows clock, Timer instance, channel, IRQ line, DMA route, and downstream trigger consumers |
| Entry state | Clock/counter/channel/DMA/trigger requirements and whether ISR context is permitted |
| Snapshot | Registers and peer state captured, plus whether the snapshot is coherent |
| Staging | Every fallible validation and Codec operation completed before commit |
| Commit | Exact register/peer call order and why intermediate states are safe |
| Concurrency | Single-owner guarantee, lock/critical section, hardware quiescence, and RMW race assumptions |
| Success postcondition | Programmed and active state, flags, clock, counter, channels, NVIC, and peer resources |
| Failure before commit | No hardware or caller output changed |
| Failure after commit | Guaranteed rollback/cleanup state, or explicitly documented irreversible/best-effort result |
| Cleanup | One path that restores temporary state and releases every acquired resource |

An interrupt critical section protects against software preemption only. It
does not stop Timer hardware, DMA, external triggers, another core, or a
downstream synchronization graph. The guard must match the actual writers.

For this single-core bare-metal target, either:

- enforce one owner per Timer instance and prohibit concurrent task/ISR
  mutation; or
- implement a per-instance transaction guard plus the necessary source/DMA/
  trigger quiescence.

The choice is an API contract decision, not an LL detail.

### Status and Output Contract

Use the shared status vocabulary consistently:

| Status | Meaning in a public Driver contract |
| --- | --- |
| `DRIVER_STATUS_SUCCESS` | The documented postcondition is fully established. |
| `DRIVER_STATUS_ERROR_NULL_PTR` | A required instance or caller-storage pointer is null; no write/output publication occurred. |
| `DRIVER_STATUS_ERROR_INVALID_ARG` | Selector, mask, range, unsupported instance/capability, or incompatible static configuration is invalid; no write occurred. |
| `DRIVER_STATUS_ERROR_STATE` | Required clock/resource/configured hardware state is unavailable; no write occurred unless a cleanup contract explicitly says otherwise. |
| `DRIVER_STATUS_ERROR_BUSY` | A live counter/channel/DMA/trigger/ownership condition prevents the operation; no write occurred. |
| `DRIVER_STATUS_ERROR_TIMEOUT` | A bounded hardware transition did not complete; cleanup/final state is documented. |
| `DRIVER_STATUS_ERROR_FAIL` or `DRIVER_STATUS_ERROR` | A lower-layer or cleanup failure prevented the promised postcondition; the precise failure state is documented. |
| `DRIVER_STATUS_OFF/ON` | Direct return only for an explicitly binary state getter; never returned as generic operation success. |

Getter outputs and grouped Codec destinations must remain unchanged on failure
unless a function explicitly documents partial observation—which canonical
Driver APIs should avoid.

### Verification Matrix

| Risk or contract | Pure unit | Fake-register / trace | Hardware integration | Example consumer |
| --- | --- | --- | --- | --- |
| Selector encode/decode, preservation, atomic publication | Codec tests | Not required | Boundary spot-check | Not sufficient alone |
| Named register address/direction and exactly-one access | Not sufficient | LL fake-register/layout tests | Smoke test | Not sufficient alone |
| No write on invalid/busy/state error | Validator unit where pure | Driver MMIO trace | Live running/disabled cases | Negative example optional |
| Multi-register ordering and cleanup | Not sufficient | Driver peer/MMIO trace with injected failure | Lifecycle transition | Config example |
| W0C acknowledgement and selected/unselected race contract | Codec mask tests | SR write trace | IRQ storm/arrival test | IRQ example |
| EGR update side effects | Codec action image | Full CR1/CR2/DIER/CCR/EGR trace | Preload/TRGO/DMA setup | PWM/sync example |
| Capture read-clear and overcapture ordering | Codec event/value tests | CCR/SR ordered-read trace | High-rate capture | Capture example |
| IRQ source/flag/NVIC separation | Mask tests | peer-call and aggregation trace | IRQ line tests | IRQ example |
| Channel-disabled and counter-stopped transitions | Validation tests | no-write/order trace | Active PWM/capture | Feature example |
| Instance capability/topology | LUT/static tests | routing trace | Each supported instance | At least one non-default instance |
| Concurrency/single-owner contract | State-machine tests | re-entry/interleaving injection | ISR/task contention where supported | Ownership documentation |
| Public Doxygen and style | Doxygen/lint | N/A | N/A | API use matches docs |

Examples prove usability and integration; they do not replace negative,
preservation, failure-injection, or access-order tests.

## Canonical Closure Gate Registry

This registry is the only binding completion-gate definition. The layer-local
checklists above are diagnostic review prompts; they do not replace or abridge
these gates. Copy the IDs and definitions unchanged into every peripheral
audit, then record evidence for the selected scope.

| Gate ID | Binding gate |
| --- | --- |
| S-01 | Concrete part, instances, capabilities, topology, reference manual, package data, errata, consumers, and non-goals are named and evidenced. |
| D-01 | Every in-scope intent has one admitted canonical Driver path, operation class, and audience; compatibility paths have migration plans. |
| D-02 | Every public contract defines values, capability behavior, preconditions, resources/side effects, success/failure state, statuses, and concurrency. |
| D-03 | Group, scalar, action, data, and lifecycle granularity matches one coherent hardware transaction; no god config or split invariant exists. |
| D-04 | All validation, snapshots needed for validation, calculations, and fallible Codec staging finish before the first MMIO/peer mutation. |
| D-05 | Every post-mutation path has one guaranteed cleanup/release flow and documented rollback or irreversible failure state. |
| D-06 | Programmed/preload and active/effective state are distinguished; every update/commit accounts for the full preload, flag, TRGO, DMA, and synchronization domain. |
| D-07 | Peripheral event flags, IRQ source enables, DMA source enables, and NVIC delivery state have separate types, semantics, and owners. |
| D-08 | Single-owner, lock/critical-section, ISR, DMA, trigger, hardware-writer, and snapshot-coherence contracts are explicit. |
| D-09 | RCC, reset, NVIC, GPIO/AFIO, DMA, cache, reference-count, lock, and shared-resource ownership use declared peer services without bypassing invariants. |
| D-10 | Every private helper centralizes a reusable configuration-domain staging path, repeated policy, topology/capability, a hazardous commit sequence, calculation, or cleanup; trivial renaming and whole-public-function shadow helpers are rejected. |
| D-11 | Application-facing APIs expose domain vocabulary only; any raw/diagnostic or typed peer endpoint has an explicit restricted audience. |
| C-01 | Every contract-frozen Driver transformation has exactly one Codec path or a written no-Codec reason; every exported Codec has an admitted consumer. |
| C-02 | Codec is pure: no peripheral pointer, volatile I/O, topology, sequencing, live-state policy, peer access, polling, or locking. |
| C-03 | Persistent concepts have Extract/Stage symmetry; actions, acknowledgements, status, and consuming reads have deliberate asymmetry. |
| C-04 | All validation precedes publication; failed grouped/multi-image operations change no caller storage and successful staging preserves unrelated bits. |
| C-05 | Public domain values plus private caller-owned `reg` images form the Driver/Codec boundary; raw images never cross the application API. |
| C-06 | Pure tests cover all selectors, invalid/reserved values, boundaries, lanes, preservation, atomic publication, and round trips; Doxygen/style pass. |
| L-01 | Every contract-frozen Driver observation/mutation has one legal named LL route and a register-demand trace row. |
| L-02 | Every extra complete-map accessor is classified as raw foundation, deferred, or removed; it creates no upper-layer scope. |
| L-03 | Each LL function performs exactly one mechanical read, write, or address formation and contains no public validation, semantic translation, batching, or policy. |
| L-04 | RO/RW/WO/action/W0C/W1C/read-clear/FIFO/data/portal/shadow semantics match the manual, C qualifiers, names, and legal direction. |
| L-05 | W0C/W1C, action, read-clear, FIFO, data, portal, and hardware-modified state bypass unsafe generic RMW/dirty-write operations. |
| L-06 | Buffered and mode-dependent registers have Driver-owned commit/mode contracts; LL never hides those side effects. |
| L-07 | Layout/static checks and exactly-one-access fake-register tests pass; generic error handling never fabricates plausible data; Doxygen/style pass. |
| X-01 | Every admitted public API has an end-to-end intent -> Driver helper -> Codec/no-Codec -> LL/peer -> resource -> status -> test trace. |
| X-02 | Every exported function and public semantic type/selector family is classified canonical, private, raw foundation, compatibility, deferred, misowned, or rejected. |
| X-03 | Required warning-clean builds, pure tests, MMIO/peer traces, failure injection, hardware checks, examples, Doxygen, style, and `git diff --check` pass. |
| X-04 | No unresolved question remains inside the selected implementation scope; deferred questions name an admission condition and owner. |
| X-05 | Independent evidence, architecture, and usability reviews are resolved; decisions record provenance, affected traces, and superseded guidance. |

### Current Timer Gate Evidence

`Planning-met` means the audit contains enough reviewed evidence or a frozen
planning rule; it does not mean the implementation has passed. `Open` blocks
the named implementation scope. `Deferred` is outside the admitted scope.

| Gate ID | Status | Evidence or required command/result | Baseline/date | Reviewer |
| --- | --- | --- | --- | --- |
| S-01 | Open | Capability table is populated; matching silicon errata review is absent. | `91635a4`, 2026-08-15 | Evidence review |
| D-01 | Planning-met | Public inventory, audience rules, admission table, and symbol disposition are recorded. | `91635a4`, 2026-08-15 | Architecture review |
| D-02 | Open | Per-function contract rows remain required before each source wave. | Current audit | Architecture review |
| D-03 | Planning-met | Base groups retained; god config rejected; candidate feature groups separated. | Current audit | Architecture review |
| D-04 | Implementation-met; evidence open | Root and grouped source ordering mitigate H-001; retained no-write-on-error MMIO trace evidence is still required. | `timer.c` staging/commit paths, 2026-08-15 | Evidence review |
| D-05 | Open P0 | Cleanup structure and injected-failure traces are not retained. | H-009 / Wave 1 | Architecture review |
| D-06 | Open P0 | H-002/H-010/H-016; no retained whole-domain commit trace. | RM0008 Ch. 15 / current source | Evidence review |
| D-07 | Implementation-met; evidence open | IRQ sources, event flags, and NVIC delivery now use separate contracts; retained Codec/MMIO and IRQ-delivery traces remain required. | Current Driver/Codec and IRQ example, 2026-08-15 | Evidence review |
| D-08 | Open P0 | No explicit instance ownership/guard contract or interleaving evidence. | H-008 | Architecture review |
| D-09 | Open | RCC topology exists; clock-OFF, NVIC, AFIO, DMA, and shared-resource contracts remain incomplete. | H-003/H-015/H-017 | Architecture review |
| D-10 | Planning-met | Existing and candidate helpers classified by earned responsibility. | Driver helper inventory | Architecture review |
| D-11 | Planning-met | Application, peer, compatibility, and raw audiences are defined; DMAR exception is restricted. | Admission section | Usability review |
| C-01 | Open | Current demand counted; accepted-next CCR/PWM transformations remain missing and dormant exports need disposition. | Codec inventory | Evidence review |
| C-02 | Planning-met | Source audit found no Codec MMIO, instance, RCC, NVIC, or peer access. | `timer_codec.c` | Evidence review |
| C-03 | Open | Grouped/output/event vocabulary and deliberate asymmetries need refactor. | Codec matrix | Architecture review |
| C-04 | Open | Grouped extraction publication defect and new multi-image contracts need tests. | Codec defect 1 | Evidence review |
| C-05 | Planning-met | Boundary rule corrected to domain values plus private `reg` images. | Codec completion section | Usability review |
| C-06 | Open | No retained repository-local Codec test artifact; Doxygen pass belongs to implementation wave. | Historical TODO record only | Evidence review |
| L-01 | Planning-met | Current and accepted-next register demand is mapped. | LL demand and trace tables | Architecture review |
| L-02 | Planning-met | CR2/SMCR/DCR/DMAR explicitly dormant; invalid EGR read identified. | LL classification table | Architecture review |
| L-03 | Open | Generic checked/unchecked contract and one-transfer proof remain unfrozen. | LL audit | Evidence review |
| L-04 | Implementation-met; evidence open | EGR/CCR/DMAR access classes were corrected; retained layout/one-access evidence and future mode-aware capture APIs remain required. | H-005/H-006, 2026-08-15 | Evidence review |
| L-05 | Open P0 | RegOps exclusions are planned but not yet proven across special registers. | Special RegOps section | Architecture review |
| L-06 | Open | Active/preload and mode-specific Driver contracts remain incomplete. | H-002/H-010/H-014 | Architecture review |
| L-07 | Open | No retained layout/one-access fake-register test evidence. | Wave 2 | Evidence review |
| X-01 | Open | Family trace exists; per-public-function expansion is a pre-code gate. | Traceability matrix | Usability review |
| X-02 | Planning-met | Functions and semantic symbol families have explicit disposition; re-run after each wave. | Public/LL/Codec inventories | Evidence review |
| X-03 | Open | This documentation commit validates docs/diff only; source-wave verification has not run. | Verification ledger below | Usability review |
| X-04 | Open by wave | Wave 1 entry decisions and errata evidence remain open; deferred families state admission conditions. | Entry conditions / waves | Architecture review |
| X-05 | Planning-met | Three independent reviews completed and their findings are incorporated in this planning baseline. | Review record, 2026-08-15 | Evidence, architecture, usability reviewers |

### Independent Review Record

The reviews were independent, read-only passes over baseline source plus this
working document. “Resolved” below means the planning contradiction was
incorporated or converted into an explicit open gate; it does not close the
source implementation.

| Review role | Scope | Material findings incorporated | Affected decisions/gates | Date/result |
| --- | --- | --- | --- | --- |
| Evidence verification | Reproduced symbol counts; checked source claims, RM0008 semantics, package/remap/DMA/ITR topology, links, and historical tests | H-001 through H-007, H-010, H-012/H-013, H-015/H-017; exact inventory commands and capability table | S-01, D-04/D-06/D-07, C-01/C-06, L-04/L-07, X-02 | 2026-08-15; resolved for planning |
| Adversarial architecture | Challenged atomicity, UG feasibility, IRQ ownership, concurrency, API admission, PWM commit, clock gating, DMA safety, and authority conflicts | Reset/dormant Wave 1 boundary; narrow PWM-output scope; coherent preload commit; typed DMAR contract; clock/mode work; conditional deferred waves | D-02 through D-09, L-04 through L-06, X-01/X-04; F-004 through F-016 | 2026-08-15; no remaining Critical/High planning contradiction |
| Document usability | Checked normative precedence, status language, Doxygen/Markdown behavior, reproducibility, canonical gates, worksheet completeness, and terminology | Explicit authority/status; Doxygen exclusion; canonical gate registry; complete scratchpad/evidence tables; lifecycle terminology | All gate definitions; X-03/X-05 | 2026-08-15; independently validated with no substantive blocker |

### Planning Baseline Validation Record

| Check | Result | Scope/qualification |
| --- | --- | --- |
| Reproducible inventory commands | Pass: 43 `timer.h` callables, 4 public pure utilities, 27 private Driver helpers, 47 external plus 30 private Codec functions, 38 LL transfer helpers plus 2 address macros | Baseline `91635a4` |
| Relative Markdown target check | Pass | This audit, `AGENTS.md`, `BareMetal/Driver/Timer/TODO.md`, and `BareMetal/Driver/Timer/TIMER_ARCHITECTURE.md` |
| Canonical/worksheet gate comparison | Pass: 30 IDs and definitions match exactly | Method/template consistency |
| `doxygen Doxyfile` | Exit 0; no warning references any of the three excluded planning documents | Existing unrelated repository Doxygen warnings remain and X-03 is therefore open for source-wave closure |
| Removed-test search/directory check | Pass: no Timer test-directory reference or retained directory remains | Historical result is text-only and non-gating |
| `git diff --check` plus new-file whitespace check | Pass: no whitespace diagnostic | Documentation/deletion working diff before commit |
| Source builds and executable tests | Not run | This planning-only commit changes no C implementation and intentionally removes the repository-local historical test artifact |

## Implementation Waves

### Planning Order Versus Build Order

Architecture is always planned top-down:

```text
public Driver intent -> private Driver transaction -> Codec need -> LL need
```

After one vertical feature contract is frozen, its code may be built in compile
dependency order:

```text
domain types/defines -> legal LL -> pure Codec -> private Driver -> public Driver
-> tests/documentation -> example/feature consumer
```

This does not reverse the architecture. Lower-layer code is implemented only
because the already-frozen Driver transaction demanded it.

### Mandatory Mini-Sequence For Every Wave

1. Add the consumer/intent and public contract rows to this document.
2. Freeze exact API signatures, structures, statuses, side effects, and
   compatibility plan.
3. Add or update domain types, selectors, validation helpers, and capability
   metadata using the smallest direct representation.
4. Add/correct only the legal LL access and special-register documentation
   demanded by the transaction.
5. Implement and exhaustively host-test the derived pure Codec transformations.
6. Implement private Driver validation, per-domain image staging, hazardous
   commit, and cleanup helpers without creating a private root-shadow wrapper.
7. Implement public Driver functions as owners of their complete
   Read/Modify/Write transactions; root configuration visibly composes domain
   staging before one ordered commit.
8. Run MMIO/peer-call trace tests, failure injection, builds, Doxygen, style,
   and diff checks.
9. Migrate one example or higher-level consumer without using Codec/LL directly.
10. Mark the wave complete only after its exit gate passes; do not mix the next
    feature into the same mitigation commit.

### Ordered Timer Mitigation Plan

| Wave | Priority | Deliverable | Why this order | Exit gate |
| --- | --- | --- | --- | --- |
| 0 | P0 | Freeze evidence, contracts, and compatibility policy | Prevents code from deciding architecture implicitly. | No unresolved question for the selected wave; every API has a trace row. |
| 1 | P0 | Repair base transaction, mode, clock-gate, and cleanup safety | Base source ordering is repaired; complete live-mode validation, safe clock-OFF semantics, and retained failure evidence remain open. | No-write-on-error, mode/clock transition, and injected-failure cleanup tests pass. |
| 2 | P0 | Correct raw/LL special-register semantics | Invalid EGR read and ambiguous CCR/DMAR mechanics undermine every later feature. | Access qualifiers, LL directions, RegOps restrictions, layout/access tests pass. |
| 3 | P0 | Split IRQ sources, event flags, and NVIC delivery | Channel/capture work needs trigger and overcapture semantics; current NVIC aggregation is incomplete. | All DIER/SR sources are modeled and NVIC ownership is unambiguous. |
| 4 | P1 | Clarify programmed tick state, active commit, and dedicated delay ownership | Removes misleading side effects before more consumers depend on them. | Programmed-tick pair preserves unrelated state; any active commit has explicit UG effects; compatibility and delay-service tests pass. |
| 5 | P1 | Add common channel and CCR foundations | Output compare, PWM, capture, encoder, IRQ, and DMA all depend on them. | Channel/CCR contracts, mode validation, Codec/LL/Driver tests pass. |
| 6 | P1 | Add the narrow Timer PWM-output foundation | The dormant PWM migration admits PWM1/PWM2 lane mode/preload/value behavior, not every output-compare selector. | All four channels, preload/no-preload, PWM mode, polarity, coherent commit, and active-channel negative tests pass. |
| 7 | P1 | Rebuild PWM feature driver and examples | Validates the new Timer primitives without leaking lower layers. | PWM uses only public Timer/GPIO services; frequency/duty/channel-sharing examples pass. |
| 8 | Deferred | Add input capture after consumer admission | Requires corrected event/overcapture and CCR consuming-read foundations. | A named consumer passes admission; capture/filter/read-clear/overcapture contracts and tests pass. |
| 9 | Deferred | Add master/slave, trigger, and encoder after consumer admission | Requires coherent channel inputs and complete event/topology modeling. | Named consumers pass admission; only their required SMS/TS/ITR/encoder scope is implemented and tested. |
| 10 | Deferred | Add Timer-side DMA after peer-contract admission | Requires corrected DIER source model, direction-aware register-window validation, CR2 timing, and DMAR semantics. | DMA peer contract passes admission; DIER/CCDS/DCR/window plus integration tests pass. |
| 11 | P3 | Add capability-gated Timer variants | Only safe after the general-purpose semantic model is stable. | Basic/advanced/32-bit/reduced-channel variants add data and variant modules without weakening existing contracts. |
| 12 | P3 | Remove compatibility debt and finalize whole-stack Doxygen | Removal is safe only after examples and peer consumers migrate. | No legacy consumer remains; full Doxygen/style/build/test matrix is green. |

### Wave 1 — Base Transaction Safety

- Limit the first safe lifecycle contract to an exclusively owned Timer. The
  application must explicitly deconfigure when deferred channel, master/slave,
  DMA, or external raw-LL state must begin dormant; the canonical root
  configure path preserves those unrepresented domains and cannot claim their
  ownership.
- Move counter-stopped and all other fallible checks before PSC/ARR/CNT writes.
- Stage every timebase image locally and publish none on Codec failure.
- Skip ordinary register writes when readable programmed images are unchanged.
- Do not use preload equality to skip an active commit. Define whether the API
  is preload-only, always commits, or has trustworthy commit provenance that
  proves the active shadow is synchronized.
- For Wave 1's application-owned entry domain, prove the documented UG behavior
  and preserve IRQ/channel/trigger/DMA/downstream state that is not represented
  by `tim_config_t`. The application explicitly invokes `TIM_DeConfig()` when
  it requires those separate or deferred domains to enter reset state.
- Replace post-mutation early returns with a single cleanup path.
- Keep `TIM_Config` and `TIM_DeConfig` as independent canonical conjugate
  root-lifecycle entry points. Neither calls the other; the application owns
  ordering and the decision to reset before configuration.
- Define `TIM_Config`, `TIM_DeConfig`, and delay success/failure postconditions
  for clock, CEN, NVIC, CNT, flags, and configuration.
- Validate current DIR/CMS transitions against the complete live CR1/SMCR mode
  context before writing.
- Retain RCC/application ownership of every Timer clock-gate transition; do not
  reintroduce Timer-owned clock mutation.
- After validated instance-to-index conversion, replace the dense TIM2/3/4
  clock-mask, reset-mask, and IRQ-line switches with separate typed LUTs;
  retain fully braced switches only for genuinely sparse mappings.
- Choose and enforce single-owner or per-instance transaction-guard policy.
- Update `BareMetal/Driver/Timer/TODO.md` and `BareMetal/Driver/Timer/TIMER_ARCHITECTURE.md` in the same source
  wave so the old “finalized update/NVIC ownership” claims cannot survive.

Required tests:

- running Timer returns BUSY with zero MMIO writes;
- invalid/null/disabled-clock cases produce zero writes and unchanged outputs;
- staged Codec failure produces zero writes;
- unsupported live DIR/CMS context produces zero writes;
- clock OFF rejects an active/non-quiescent Timer or proves its documented
  intentional pause state;
- equal programmed configuration produces no redundant state-register write;
- preload-only and active-commit contracts make the expected `UG` behavior
  explicit, including the case where programmed images are equal but active
  synchronization is unknown;
- injected failure after temporary state proves restoration;
- application-owned-domain UG trace proves every documented Wave 1 side effect
  is controlled and every out-of-domain feature state is preserved.

### Wave 2 — Raw Map, LL, and RegOps Semantics

- Change EGR to a write-only raw view and remove `LL_TIM_ReadEGR`.
- Establish neutral/read and output/write CCR aliases with mode-dependent LL
  notes.
- Mark DMAR as a transfer portal rather than stable configuration.
- Choose the checked or unchecked generic LL transfer contract and apply it
  consistently.
- Add special-register exclusions to RegOps dirty/RMW helpers.
- Add compile-time layout assertions and exactly-one-access fake-register
  tests.

### Wave 3 — Event and Interrupt Model

- Separate IRQ-source and event-flag types are implemented; DMA-source naming
  remains deferred with DMA admission.
- `TIE/TIF` and `CC1OF..CC4OF` are covered; `TDE` remains deferred.
- IRQ-named SR functions were replaced by event observation/acknowledgement.
- Timer source enable is independent from generic NVIC line state.
- If a convenience integration remains, aggregate all Timer sources and every
  peripheral sharing the vector before disabling/clearing NVIC.
- Enable-time stale-pending behavior belongs to the application/NVIC layer;
  the selected-source W0C race is documented on `TIM_AckIRQEvents`.
- Generic `CCxIF` acknowledgement is mode-aware. By Driver policy, reject it for
  an input-capture lane so an unread notification is not silently discarded;
  reserve normal consumption for `TIM_ReadCapturedValue` and add an explicit
  discard action only if a consumer admits it.
- Legacy compatibility wrappers are removed after consumer migration.

### Wave 4 — Programmed Tick, Active Commit, and Delay Service

- Add narrow `TIM_GetProgrammedTickFrequency` /
  `TIM_SetProgrammedTickFrequency` behavior that changes/observes PSC preload
  without claiming active-shadow knowledge.
- Add an explicit apply/commit behavior only if a consumer needs immediate
  effective timing and accepts the documented UG cross-feature side effects.
- Rename or explicitly document the full base-frequency preset and its default
  ARR/CNT/CR1 side effects.
- Move blocking delay behavior behind a dedicated-Timer service ownership
  contract while preserving temporary wrappers.
- Update existing Timer polling/IRQ and startup-delay users.

### Wave 5 — Common Channel and CCR Foundation

- Add supported-channel capability metadata and validators.
- Add channel enable-mask and single-channel polarity public contracts.
- Add private channel-to-CCMR and channel-to-CCR routing with fully braced
  switches or proven formulae where appropriate.
- Add generic CCR value Codec; separate output compare writes from input capture
  consuming reads at Driver level.
- Enforce `CCxS` changes only with `CCxE=0` and document capture-prescaler reset
  when disabling a channel.
- Add all-channel lane, invalid/multi-mask, unrelated-bit, and one-write tests.

### Wave 6 — Timer PWM-Output Foundation

- Introduce a narrow `tim_config_pwm_output_t` covering PWM1/PWM2 mode and
  preload; polarity and runtime enable remain common channel properties.
- Refactor the Codec to typed grouped PWM-output Extract/Stage and reject a
  getter when the live lane is not in the admitted PWM representation.
- Add Driver PWM-output config and compare-value pairs; keep general output-
  compare modes plus fast/clear selectors deferred pending consumers.
- Define counter-running and channel-disabled policy, preload behavior, update
  timing, polarity changes, and output glitch guarantees.
- Freeze the whole-domain preload commit/feature transaction needed to stage
  PSC/ARR/CCR coherently before CEN.
- Reject hidden shared-timebase mutation from channel-local APIs.

### Wave 7 — PWM Feature Integration

- Freeze an AFIO/board-route peer service for remap and SWJ conflicts before
  using remapped pins; otherwise constrain the first migration to package-
  available no-remap routes.
- Rebuild PWM on public Timer PWM-output/timebase/channel functions and
  public GPIO/AFIO routing services.
- Migrate TIM2/TIM3/TIM4 routes first; reject/defer legacy TIM1 routes until
  the advanced-Timer variant is admitted.
- Keep frequency/duty calculations and shared Timer channel arbitration in PWM.
- Define rounding, range, `0%`, `100%`, inverted mode, and multi-channel shared
  frequency behavior.
- Migrate PWM examples and remove every legacy direct Timer API/LL dependency.

### Wave 8 — Input Capture

Activate this wave only after a named application or feature consumer passes
the public API admission gate.

- Introduce input-selection, prescaler, filter, and grouped config types.
- Add transactional CCMR+CCER input-view Codec pair and Driver apply path.
- Admit direct/indirect input selection first; defer `CCxS=TRC` until the
  trigger dependency is admitted.
- Add captured-value consuming read with value plus overcapture result.
- Verify CCR-before-overcapture observation order, CCxIF read-clear behavior,
  W0C overcapture acknowledge, and high-rate arrival policy.

### Wave 9 — Synchronization and Encoder

Activate each native synchronization/encoder sub-family independently after a
named consumer passes admission; do not implement every SMS/TS field merely
because the register contains it.

- Expand SMS vocabulary and add complete trigger/external-trigger structures.
- Add instance-aware internal-trigger connection validation.
- Add master/slave grouped Codecs and Driver transactions.
- Account for triggers that can change CEN asynchronously.
- Build encoder as a coherent SMCR plus channel 1/2 input transaction with
  rollback/cleanup and GPIO routing owned above Timer.

### Wave 10 — Timer-Side DMA

Activate this wave only after the DMA peer contract defines transfer direction,
window ownership, setup/teardown ordering, and failure behavior.

- Complete DMA request source vocabulary and Driver APIs.
- Add CR2.CCDS and typed DCR burst Codecs.
- Provide DMAR address to the DMA peer under one explicit service contract.
- Require a 16-bit DMAR peripheral transfer width, compatible memory width and
  alignment/conversion policy, and a correctly typed endpoint; do not treat
  the 16-bit portal as an ordinary 32-bit `reg` transfer.
- Validate every DCR-selected register in the burst window against the peer's
  read/write direction, reserved offsets, mode-dependent access, and action/
  status semantics before enabling DMA.
- Keep DMA channel/controller state outside Timer.
- Verify update/channel/trigger request timing, transfer count `1U..18U`, base
  address selection, DMAR portal ordering, and cleanup on DMA setup failure.

### Wave 11 — Variant Expansion

Add support through capability data and variant-specific structures/modules;
do not widen every general-purpose type pre-emptively. Candidate extensions are
basic timers, advanced-control timers, repetition counter, break/dead-time,
main/complementary outputs, shared/split vectors, reduced channel counts, and
32-bit counters.

### Per-Wave Commit Rule

Each wave may require several commits. Follow repository layer order and keep
one layer or one cross-cutting contract correction per reviewable commit:
types/defines, LL, Codec, Driver, tests/documentation, then consumer migration.
Every commit leaves the repository buildable. Do not mix compatibility removal,
unrelated Doxygen cleanup, and a new hardware feature unless the same frozen
contract requires them together.

## Reusable Audit Worksheet

### Cross-Repository Applicability Notes

The three-layer method is a planning and ownership model, not a demand that
every public function make three calls.

- GPIO grouped configuration validates the normal path: Driver owns public
  validation and write ordering, Codec owns coupled STM32F1 MODE/CNF/ODR image
  translation, and LL owns register access.
- GPIO BSRR/BRR set/reset actions demonstrate a valid no-Codec path when the
  validated public mask is already the exact action payload and no reverse
  translation exists.
- GPIO IRQ demonstrates a composite transaction: one public operation can
  coordinate GPIO, AFIO, EXTI, NVIC, shared vectors, and rollback. Peer/shared
  ownership must therefore be audited alongside register layers.
- A module that currently lacks Codec is not automatically evidence that Codec
  is unnecessary. If Driver/LL contain selector-to-field translation, the
  missing Codec is architecture debt.
- A feature driver that owns no registers and composes only peer public APIs may
  legitimately have no private Codec or LL.
- Cached clocks, reference counts, shared IRQ groups, locks, and resource
  arbitration make peer public/service APIs mandatory; bypassing them through
  a peer LL can leave semantic state stale.

Use the following blank sections for any STM32 peripheral. Copy Section 0 and
all numbered sections together, retain rejected/deferred rows, resolve the
recorded repository paths at the destination, and do not begin implementation
until the selected Driver contract is contract-frozen.

### 0. Normative Method and Legends

| Field | Required value |
| --- | --- |
| Normative method source | Repository path to this audit method |
| Method/gate revision | Commit hash and date copied |
| Coding-preferences source | Repository path valid from the destination file |
| Coding-preferences revision | Commit hash and date reviewed |

| Label | Meaning |
| --- | --- |
| `[Observed]` | Directly verified evidence |
| `[Decision]` | Accepted architecture or ownership decision |
| `[Gap]` | Required behavior/evidence absent or incomplete |
| `[Deferred]` | Scope assigned to a named later admission condition or owner |
| `[Question]` | Evidence is required before a decision can freeze |
| `[Conclusion]` | Reviewed planning conclusion, not implementation proof |

Track lifecycle independently:

```text
Observed -> Candidate -> Admitted -> Contract-frozen -> Implemented -> Verified
```

### 1. Document Control

| Field | Value |
| --- | --- |
| MCU family and concrete part | |
| Supported peripheral instances | |
| API audiences | Application / Peer service / Compatibility / Raw diagnostic |
| Hardware references and errata | |
| Existing architecture reference | |
| Normative method source/revision | |
| Baseline commit/date | |
| Coding-preferences revision reviewed | |
| Doxygen processing disposition | Include / Exclude with reason |
| Audit owner/date/status | |

### 2. Scope and Non-Goals

#### In scope

- _List supported use-cases and variants._

#### Deferred or owned elsewhere

- _List explicit non-goals and the owning module or admission condition._

### 3. Evidence Ledger

| ID | State | Source | Finding | Consequence |
| --- | --- | --- | --- | --- |
| | `[Observed]` | | | |

#### Open Questions and Required Evidence

| Question ID | Scope/wave | Decision blocked | Required evidence | Owner | Due/admission condition | Status |
| --- | --- | --- | --- | --- | --- | --- |
| | | | | | | Open / Resolved / Deferred |

#### Finding and Hazard Ledger

| Finding ID | Severity/state | Evidence | Affected API/trace | Mitigation wave | Closure evidence |
| --- | --- | --- | --- | --- | --- |
| | | | | | |

### 4. Instance Capability and Topology

| Stable index | Instance | Bus/gate/reset | IRQ/DMA | Width/channels | Capabilities | Shared resources | Quirks |
| --- | --- | --- | --- | --- | --- | --- | --- |
| | | | | | | | |

Checks:

- [ ] Every supported instance has one stable index.
- [ ] Every topology claim cites a device header, datasheet, reference manual,
      or erratum.
- [ ] Optional capabilities, missing instances, aliases, and shared resources
      are explicit.
- [ ] Bus, reset, IRQ, DMA, width, channel, and special-register differences
      are known before public API design.

### 5. Consumer and Intent Inventory

| Intent ID | Consumer | User intent | Audience | Required guarantee | Disposition |
| --- | --- | --- | --- | --- | --- |
| | | | | | Accept / Defer / Feature driver / Reject |

### 6. Public API Admission

| Intent ID | Candidate API | Operation class | Semantic/stable | Clear owner | Testable contract | Register leakage | Decision/reason |
| --- | --- | --- | --- | --- | --- | --- | --- |
| | | Lifecycle / Config / State / Status / Action / Ack / Data / Derived / Composite | | | | | |

#### Existing Public Symbol Disposition

| Function or semantic type/selector family | Current audience | Lifecycle state | Canonical owner | Disposition/migration |
| --- | --- | --- | --- | --- |
| | | Observed / Candidate / Admitted / Contract-frozen / Implemented / Verified | | |

Questions for every candidate:

- Is there a named application or peer-driver consumer?
- Is the vocabulary semantic across the supported variants?
- Is there exactly one owner?
- Is the granularity coherent with coupled fields and transitions?
- Are capability/unsupported-instance rules defined?
- Are preconditions, side effects, postconditions, failure, and statuses
  expressible?
- Is thread/ISR/reentrancy behavior defined?
- Can negative and ordering behavior be tested?
- Is it canonical, a justified convenience, compatibility, or redundant?

### 7. Frozen Public Contracts

| API | Operation/audience | Inputs/ranges | Outputs | Capability/unsupported behavior | Preconditions | Resources/side effects | Success postcondition | Failure postcondition | Statuses | Concurrency |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| | | | | | | | | | | |

Public Doxygen for each contract-frozen function must follow the coding-
preferences source recorded in Sections 0 and 1, which is the sole style
authority. Resolve that path after copying the worksheet; do not assume the
current relative location. Record the reviewed revision rather than copying
its full switch, macro-alignment, call-layout, `reg`, and comment rules here.
At minimum verify:

- focused `@brief` and necessary `@details`;
- direction-qualified parameters with `Accepted values` or `Expected values`;
- referenced return type and function-specific operation label;
- exhaustive ``@retval - @ref `STATUS`:`` entries with repository alignment;
- `@pre`, `@note`, and `@warning` for ownership, state, side effects, and races;
- matching `//!` implementation comments for non-obvious logic and sequencing.

### 8. Private Driver Helper Derivation

| Helper | Category | Policy/invariant isolated | Callers | Why it earns existence |
| --- | --- | --- | --- | --- |
| | Validation / Topology / Capability / Snapshot / Transaction / Transition / Cleanup / Calculation | | | |

Reject a helper that only renames one safe call. Retain a single-use helper when
it isolates a hazardous hardware sequence or mandatory cleanup invariant.

### 9. Driver Transaction Plans

| API | Validate | Snapshot/read | Stage/compute | Commit order | Peer services | Cache/lock impact | Failure/rollback |
| --- | --- | --- | --- | --- | --- | --- | --- |
| | | | | | | | |

For each plan, explicitly record:

- ownership acquired and borrowed;
- software and hardware writers that can race;
- preconditions checked before the first write;
- the last fallible operation before commit;
- intermediate hardware states;
- success and failure final state;
- single cleanup/release path.

### 10. Codec Applicability and Derivation

| API/concept | Public value | Raw representation | Peripheral-specific transform? | Extract | Stage/Ack/Action | No-Codec reason |
| --- | --- | --- | --- | --- | --- | --- |
| | | | | | | |

Codec checks:

- [ ] No peripheral pointers, volatile I/O, topology, or sequencing.
- [ ] Persistent mutable concepts have Extract/Stage symmetry.
- [ ] Actions, status, acknowledgement, and consuming reads use deliberate
      asymmetry.
- [ ] Every omitted Codec path has a written justification.
- [ ] Validation completes before caller images/outputs change.
- [ ] Unrelated bits are preserved.
- [ ] Grouped/multi-image failure leaves all caller storage unchanged.
- [ ] Pure tests cover every selector, invalid value, boundary, channel/lane,
      preservation rule, and round trip.

### 11. LL Access Derivation

| Register | Owner | Access class | Hardware writers/side effects | Read needed by | Write needed by | Named LL route | Constraints |
| --- | --- | --- | --- | --- | --- | --- | --- |
| | | RO / RW / WO / W0C / W1C / Action / Read-clear / FIFO / Data / Portal / Shadowed | | | | | |

LL checks:

- [ ] No public selector translation or public validation.
- [ ] No batching, clock, lifecycle, peer, or feature policy.
- [ ] Special write/read semantics are visible in qualifiers, names, and docs.
- [ ] Every demanded access exists and performs exactly one transfer.
- [ ] Undemanded complete-map access is marked raw foundation.
- [ ] W0C/W1C/action/read-clear/FIFO/data paths bypass unsafe generic RMW/
      dirty-write helpers.
- [ ] Foreign register access has an explicit owner.
- [ ] Layout and one-access fake-register tests pass.

### 12. Metadata Decisions

| Mapping | Index domain | Element type | LUT/formula/switch/struct | Consumer | Representation reason | Static check |
| --- | --- | --- | --- | --- | --- | --- |
| | | | | | | |

Metadata checks:

- [ ] Convert instance pointer to a stable validated index once.
- [ ] Prefer one direct typed LUT for one dense property.
- [ ] Use separate LUTs for independently consumed properties.
- [ ] Use a struct only for inseparable properties always consumed together.
- [ ] Use a fully braced switch or explicit map for sparse/irregular cases.
- [ ] Compute formulaic positions; do not store redundant data.
- [ ] Use designated initializers and assert length against instance count.
- [ ] Use semantic types, `reg` for images/masks, and typed pointer/
      `uintptr_t` for addresses.

### 13. Peer and Shared-Resource Ownership

| Resource | Owning module | API used | Borrowed/owned | Shared users | Cache/refcount/lock | Release/failure rule |
| --- | --- | --- | --- | --- | --- | --- |
| | | | | | | |

### 14. Cross-Layer Traceability

| API | Audience | Intent | Driver helpers | Codec or no-Codec reason | Own LL | Peer API | Register/resource | Special semantics | Statuses | Tests |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| | | | | | | | | | | |

### 15. Verification Plan

| Contract/risk | Pure unit | Fake-register/trace | Hardware integration | Example | Negative/boundary cases |
| --- | --- | --- | --- | --- | --- |
| | | | | | |

#### Executed Verification Evidence

| Gate ID | Exact command/test | Result | Artifact/output | Commit/date | Reviewer |
| --- | --- | --- | --- | --- | --- |
| | | Pass / Fail / Not run | | | |

Mandatory command/evidence categories:

- source compile with warnings-as-errors;
- host Codec tests;
- LL layout and fake-register access tests;
- Driver MMIO/peer-call trace and failure-injection tests;
- target project/example builds;
- Doxygen warning review;
- macro `@def`, public `@retval`, banner/alignment, and `//!` logic audit;
- `git diff --check` and independent diff review.

### 16. Completion Gates

The definitions below mirror **Canonical Closure Gate Registry** and must stay
verbatim. Do not remove rows or substitute a shorter checklist.

| Gate ID | Binding definition | Status | Evidence/command/artifact | Commit/date | Reviewer | Reopen condition |
| --- | --- | --- | --- | --- | --- | --- |
| S-01 | Concrete part, instances, capabilities, topology, reference manual, package data, errata, consumers, and non-goals are named and evidenced. | | | | | |
| D-01 | Every in-scope intent has one admitted canonical Driver path, operation class, and audience; compatibility paths have migration plans. | | | | | |
| D-02 | Every public contract defines values, capability behavior, preconditions, resources/side effects, success/failure state, statuses, and concurrency. | | | | | |
| D-03 | Group, scalar, action, data, and lifecycle granularity matches one coherent hardware transaction; no god config or split invariant exists. | | | | | |
| D-04 | All validation, snapshots needed for validation, calculations, and fallible Codec staging finish before the first MMIO/peer mutation. | | | | | |
| D-05 | Every post-mutation path has one guaranteed cleanup/release flow and documented rollback or irreversible failure state. | | | | | |
| D-06 | Programmed/preload and active/effective state are distinguished; every update/commit accounts for the full preload, flag, TRGO, DMA, and synchronization domain. | | | | | |
| D-07 | Peripheral event flags, IRQ source enables, DMA source enables, and NVIC delivery state have separate types, semantics, and owners. | | | | | |
| D-08 | Single-owner, lock/critical-section, ISR, DMA, trigger, hardware-writer, and snapshot-coherence contracts are explicit. | | | | | |
| D-09 | RCC, reset, NVIC, GPIO/AFIO, DMA, cache, reference-count, lock, and shared-resource ownership use declared peer services without bypassing invariants. | | | | | |
| D-10 | Every private helper centralizes repeated policy, topology/capability, a hazardous transaction, calculation, or cleanup; trivial renaming helpers are rejected. | | | | | |
| D-11 | Application-facing APIs expose domain vocabulary only; any raw/diagnostic or typed peer endpoint has an explicit restricted audience. | | | | | |
| C-01 | Every contract-frozen Driver transformation has exactly one Codec path or a written no-Codec reason; every exported Codec has an admitted consumer. | | | | | |
| C-02 | Codec is pure: no peripheral pointer, volatile I/O, topology, sequencing, live-state policy, peer access, polling, or locking. | | | | | |
| C-03 | Persistent concepts have Extract/Stage symmetry; actions, acknowledgements, status, and consuming reads have deliberate asymmetry. | | | | | |
| C-04 | All validation precedes publication; failed grouped/multi-image operations change no caller storage and successful staging preserves unrelated bits. | | | | | |
| C-05 | Public domain values plus private caller-owned `reg` images form the Driver/Codec boundary; raw images never cross the application API. | | | | | |
| C-06 | Pure tests cover all selectors, invalid/reserved values, boundaries, lanes, preservation, atomic publication, and round trips; Doxygen/style pass. | | | | | |
| L-01 | Every contract-frozen Driver observation/mutation has one legal named LL route and a register-demand trace row. | | | | | |
| L-02 | Every extra complete-map accessor is classified as raw foundation, deferred, or removed; it creates no upper-layer scope. | | | | | |
| L-03 | Each LL function performs exactly one mechanical read, write, or address formation and contains no public validation, semantic translation, batching, or policy. | | | | | |
| L-04 | RO/RW/WO/action/W0C/W1C/read-clear/FIFO/data/portal/shadow semantics match the manual, C qualifiers, names, and legal direction. | | | | | |
| L-05 | W0C/W1C, action, read-clear, FIFO, data, portal, and hardware-modified state bypass unsafe generic RMW/dirty-write operations. | | | | | |
| L-06 | Buffered and mode-dependent registers have Driver-owned commit/mode contracts; LL never hides those side effects. | | | | | |
| L-07 | Layout/static checks and exactly-one-access fake-register tests pass; generic error handling never fabricates plausible data; Doxygen/style pass. | | | | | |
| X-01 | Every admitted public API has an end-to-end intent -> Driver helper -> Codec/no-Codec -> LL/peer -> resource -> status -> test trace. | | | | | |
| X-02 | Every exported function and public semantic type/selector family is classified canonical, private, raw foundation, compatibility, deferred, misowned, or rejected. | | | | | |
| X-03 | Required warning-clean builds, pure tests, MMIO/peer traces, failure injection, hardware checks, examples, Doxygen, style, and `git diff --check` pass. | | | | | |
| X-04 | No unresolved question remains inside the selected implementation scope; deferred questions name an admission condition and owner. | | | | | |
| X-05 | Independent evidence, architecture, and usability reviews are resolved; decisions record provenance, affected traces, and superseded guidance. | | | | | |

#### Implementation Wave Tracking

| Wave | Admitted scope | Dependencies/gates | Commits | Exit evidence | Status |
| --- | --- | --- | --- | --- | --- |
| | | | | | Not started / Active / Blocked / Complete |

### 17. Final Decisions

| ID | State | Evidence/provenance | Affected gate/trace IDs | Supersedes | Rationale | Follow-up |
| --- | --- | --- | --- | --- | --- | --- |
| | `[Decision]` / `[Deferred]` / `[Conclusion]` | | | | | |

## Final Planning Conclusions

### Current Architecture Verdict

The current Timer stack has a strong structural core but is not yet a complete
or safe canonical reference for every STM32 driver.

| Layer | Final planning verdict |
| --- | --- |
| Driver | Strong for base clock/counter/timebase grouping and centralized apply paths; timebase failure ordering and IRQ source/event/NVIC separation are implemented, while clock/mode transitions and retained evidence remain open. The accepted-next TIM2-4 channel/PWM-output surface is missing; capture/synchronization/DMA remain intentionally unadmitted. |
| Codec | Correct pure image-transformation boundary and sufficient surface for the current Driver; accepted-next CCR/PWM-output transformations and transactional grouped publication remain open. Input-capture/trigger/DMA breadth is candidate-only, not a current completeness defect. |
| LL | Complete TIM2-TIM4 register-address coverage with EGR/CCR/DMAR access classes corrected; retained access tests, mode-aware Driver ownership, and generic RegOps restrictions remain open. |
| Cross-layer | Ownership is mostly modular and event/request/NVIC separation is implemented, but preload/update side effects, concurrency, cleanup, and programmed-versus-effective state still need explicit contracts and tests. |

Therefore:

- the **base architectural decomposition is worth retaining**;
- the **current implementation must not be copied unchanged as the generic
  STM32 pattern**;
- the **top-down audit method in this file is the reusable reference**;
- the **P0 safety/event/LL waves must close before channel/PWM expansion**.

### Reviewed Planning Decisions

| ID | State | Final decision | Architectural effect |
| --- | --- | --- | --- |
| F-001 | `[Conclusion]` | Freeze public Driver intent and private transaction plans before deriving Codec and LL. | Register breadth can never create public scope. |
| F-002 | `[Conclusion]` | Perform a Codec applicability pass for every API, but permit a documented no-Codec path. | Exact action payloads and peer-only composite features avoid artificial layers. |
| F-003 | `[Conclusion]` | Keep one nested config structure per coherent base-configuration transaction beneath root `tim_config_t`. | The root base lifecycle remains coherent without absorbing explicitly controlled operational enables. |
| F-004 | `[Implemented]` | Separate IRQ sources, event flags, DMA sources, and NVIC delivery state, and keep IRQ-source enablement outside `tim_config_t`. | DIER generation intent, SR events, DMA, and NVIC delivery remain distinct and explicit. |
| F-005 | `[Conclusion]` | Separate PWM compare-value access from captured-value consumption. | Public APIs expose CCR mode semantics and read-clear/overcapture behavior accurately. |
| F-006 | `[Conclusion]` | Build PWM above Timer PWM-output/timebase/channel primitives. | Timer owns hardware primitives; PWM owns duty/frequency/GPIO/shared-channel policy. |
| F-007 | `[Conclusion]` | Use direct typed LUTs for independent dense metadata and fully braced switches/maps for sparse topology. | Wave 1 migrates current dense clock/reset/IRQ switches; no one-field or unrelated “god metadata” structs. |
| F-008 | `[Conclusion]` | Complete validation and Codec staging before the first write, then use one cleanup path. | A failure status cannot hide an undocumented partial commit. |
| F-009 | `[Conclusion]` | Treat W0C/W1C, action, read-clear, FIFO, and data-window registers as distinct access classes. | Generic RMW and dirty-write mechanics are restricted to ordinary state. |
| F-010 | `[Conclusion]` | Model every UG/preload transition as a cross-feature event. | Timebase, channel preload, TRGO, DMA, flags, and downstream synchronization share one side-effect analysis. |
| F-011 | `[Conclusion]` | Require an explicit single-owner or transaction-guard contract per instance. | Task, ISR, DMA, trigger, and hardware writers are considered before read-stage-write. |
| F-012 | `[Deferred]` | Public software-event generation remains private until a named consumer passes admission. | Existing private UG use does not create an application API by itself. |
| F-013 | `[Deferred]` | Basic, advanced-control, 32-bit, reduced-channel, and shared-vector variants follow capability-gated extensions. | TIM2-TIM4 structures are not widened speculatively. |
| F-014 | `[Implemented]` | Use independent `TIM_Config` / `TIM_DeConfig` lifecycle entry points and remove frequency-setting configuration helpers. | `tim_config_t` owns admitted base domains; configuration preserves IRQ/deferred domains, explicit deconfiguration resets them, and calculated-frequency getters remain observational only. |
| F-015 | `[Conclusion]` | Make generic CCx event acknowledgement mode-aware and reject input-capture CCxIF discard by default. | Capture consumption remains explicit; a discard action requires independent admission. |
| F-016 | `[Conclusion]` | Limit the first PWM migration to TIM2/TIM3/TIM4 and a narrow PWM1/PWM2 output contract. | TIM1 and broader output-compare modes remain deferred; AFIO/package/SWJ routing stays peer-owned. |

### Entry Conditions For The Next Coding Phase

The next coding phase is Wave 1, not a new feature. Before its first source
edit, freeze:

1. the per-instance concurrency model: enforced single owner or transaction
   guard;
2. whether timebase setters promise preload-only state or immediate active
   commit;
3. exact success/failure postconditions for `TIM_Config`, timebase setters,
   deconfiguration, and delay cleanup;
4. the compatibility/deprecation policy for current IRQ and frequency names;
5. the MMIO/peer-call trace harness used to prove no-write-on-error and cleanup.

After Waves 1-3 pass, implement common channel and narrow PWM-output primitives,
then rebuild TIM2/TIM3/TIM4 PWM and its examples. Input capture,
synchronization/encoder, and DMA proceed in dependency order only if named
consumers later pass admission.

### Maintenance Rule

This file remains both scratchpad and architecture record:

- add new evidence and questions before changing a contract;
- preserve rejected/deferred decisions so the same debate is not repeated;
- update traceability and completion rows in the same commit as the code;
- record verification evidence at each wave exit;
- re-run an independent adversarial review when capability scope or ownership
  changes.

The reusable rule is concise even though the evidence is detailed:

> Freeze semantic intent first, derive pure transformations second, derive
> legal register access last, and prove the complete transaction before calling
> the stack modular.
