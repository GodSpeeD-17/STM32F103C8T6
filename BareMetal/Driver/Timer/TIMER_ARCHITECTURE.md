# Timer Driver Refactor Architecture

> **Status — historical implementation snapshot.** The normative planning,
> API-admission, ownership, safety-mitigation, and completion-gate authority is
> [`../../../AGENTS.md`](../../../AGENTS.md) (Part 2: Driver Stack Top-Down Audit Method).
> Where this checkpoint treats every Timer register as an ordinary
> reader/writer pair or calls update sequencing complete, the top-down audit
> supersedes it. Update both files in the same implementation wave when the
> source architecture changes.

This document captures the intended Timer driver architecture and the current
refactor checkpoint. It is scoped to the current general-purpose Timer driver
surface for `TIM2`, `TIM3`, and `TIM4` on the STM32F103C8T6.

The first refactor pass preserves the current public Timer selector vocabulary
where practical, including existing `TIMx_*` selector names. Public helper APIs
that depended on the old configuration shape can be rebuilt after the layer
boundaries are correct.

## Target Layers

| Layer | Files | Owns | Must Avoid |
| --- | --- | --- | --- |
| Core register layer | `BareMetal/Core/Inc/stm32f1xx_timer.h`, raw Timer section in `stm32f1xx_defines.h` | Raw STM32F1 register structs, raw bit positions, masks, reset values, and base mappings | Driver selectors, validation policy, public config structs, orchestration |
| Timer data types | `BareMetal/Driver/Timer/Inc/timer_data_types.h` | Timer scalar aliases and plain shared data aliases | Public selector macros, validation, register access, hardware writes |
| Timer defines | `BareMetal/Driver/Timer/Inc/timer_defines.h` | Public Timer selectors, defaults, pure validation helpers, simple selector utilities | Hardware reads/writes, sequencing, raw field placement |
| Timer config | `BareMetal/Driver/Timer/Inc/timer_config.h` | Timer-independent public configuration structures, including `tim_config_t` as a structure of structures | Timer instance pointers, register access, RCC/NVIC access, driver orchestration |
| Timer LL | `BareMetal/Driver/Timer/Inc/timer_ll.h` | Dumb static inline register read/write helpers and minimal register address macros | Validation, encoding, mode decisions, batching, clock-state sequencing, NVIC policy |
| Timer codec | `BareMetal/Driver/Timer/Inc/timer_codec.h`, `BareMetal/Driver/Timer/Src/timer_codec.c` | Private Encode/Decode helpers and public Extract/Stage functions over caller-owned register images | Hardware reads/writes, clock-state sequencing, public API decisions |
| Timer driver | `BareMetal/Driver/Timer/Inc/timer.h`, `BareMetal/Driver/Timer/Src/timer.c` | Public API, validation, orchestration, clock-state handling, batching, dirty-register writes, status handling, DIER source state, and SR event handling | Raw register map definitions, direct register field placement when codec can own it, and generic NVIC delivery ownership |
| Project/application | `Projects/*` and shared startup code | Board/application behavior and examples | Driver internals and raw register writes unless intentionally teaching raw access |

## Core Register Layer

Core must stay hardware-shaped. Timer Core content should include only:

- `TIM_TypeDef` and advanced timer raw register structs.
- Raw `TIM_*_Pos`, `TIM_*_Msk`, and raw bit masks.
- Reset/raw values when they are direct reference-manual facts.
- Base addresses and instance macros.

Public Timer selectors and defaults have been moved out of Core during this
refactor. Core raw Timer macros now provide hardware-shaped positions, masks,
and register-positioned raw values for codec use. Shared channel-field values
use the STM32 field notation directly, for example `CCxS`, `OCxM`,
`ICxPSC`, and `ICxF`.

## Timer Data Types

`timer_data_types.h` defines Timer scalar aliases such as:

- `tim_channel_t`
- `tim_channel_index_t`
- `tim_count_mode_t`
- `tim_direction_t`
- `tim_arpe_t`
- `tim_opm_t`
- `tim_update_source_t`
- `tim_irq_source_t`
- `tim_event_flag_t`
- `tim_channel_mode_t`
- `tim_channel_ccs_t`
- `tim_channel_oc_preload_t`
- `tim_channel_oc_fast_t`
- `tim_channel_oc_clear_t`
- `tim_channel_polarity_t`
- `tim_prescaler_t`
- `tim_auto_reload_t`
- `tim_counter_value_t`

Frequency values use the Core-owned `frequency_t` alias. Timer does not define
a peripheral-specific frequency type.

The file should include `stm32f1xx_data_types.h`, not the full `stm32f1xx.h`
unless a later step proves that unavoidable.

`timer_data_types.h` does not own full configuration structures. Timer
configuration is intentionally richer than GPIO configuration, so public
configuration structures live in `timer_config.h`.

## Timer Configuration

`timer_config.h` owns Timer configuration structures only. They are deliberately
timer-instance independent so the same configuration object can be applied to
`TIM2`, `TIM3`, or `TIM4` by passing the instance separately to the
driver API.

Current first-pass structure shape:

- `tim_config_timebase_t`
  - `prescaler`
  - `auto_reload`
  - `initial_count`
- `tim_config_counter_t`
  - `direction`
  - `alignment`
  - `one_pulse`
  - `auto_reload_preload`
  - `update_source`
  - `digital_filter_clock_division`
- `tim_config_t`
  - `timebase`
  - `counter`

`tim_config_t` is the root structure of structures for the currently
implemented base-configuration domains. Timer IRQ sources are deliberately
excluded: applications configure them explicitly through
`TIM_SetIRQSources()`. Deferred domains are added only after their public
contracts are admitted; until then `TIM_Config()` leaves them unchanged.
Applications call `TIM_DeConfig()` explicitly when a complete hardware reset
is required before applying the admitted configuration.

`timer_config.h` must not own register access helpers, clock-state helpers,
NVIC helpers, public driver API declarations, or codec staging.

## Timer Defines

`timer_defines.h` owns public selector values and validation helpers.

Expected groups:

- Timer instance validation for `TIM2`, `TIM3`, and `TIM4`.
- Timer channel selectors and channel-mask validation.
- Counter mode, direction, preload, one-pulse, and update-source selectors.
- Channel output/input selectors and validation.
- IRQ selectors and IRQ-mask validation.
- Current default presets such as 10 kHz, 1 MHz, and reset count defaults.

The defines layer may contain pure helper functions for mask/index conversion,
but must not read or write hardware and must not place fields into register
images.

## Timer LL Layer

`timer_ll.h` should be intentionally dumb.

Expected accessors:

- `LL_TIM_REG(TIMx, REG)` built on `REGOPS_REG`.
- Generic `LL_TIM_ReadRegister()` and `LL_TIM_WriteRegister()` helpers using
  `RegOps_Read()` and `RegOps_Write()`.
- Generic compare-and-write mechanics provided by `RegOps_WriteIfChanged()`;
  Timer retains instance validation, register selection, and write sequencing.
- Named register readers/writers for `CR1`, `CR2`, `SMCR`, `DIER`, `SR`,
  `EGR`, `CCMR1`, `CCMR2`, `CCER`, `CNT`, `PSC`, `ARR`, `CCR1` through `CCR4`,
  `DCR`, and `DMAR` as needed.

This list records the first-pass complete map, not legal bidirectional access.
The current LL removes `LL_TIM_ReadEGR` and treats input-mode CCR reads as
consuming/read-clear, and treats DMAR as a typed transfer portal rather than
ordinary RW configuration.

LL must not:

- Validate public Timer config.
- Translate `TIMx_*` selectors.
- Decide which registers must be read.
- Encode CCMR/CCER/CR1/DIER/SR fields.
- Enable or disable NVIC IRQs.
- Enable, disable, or reset Timer clocks.
- Poll, delay, or make mode decisions.

## Timer Codec Layer

The codec layer must operate only on caller-owned register images.

Private helpers should be named and scoped for encoding/decoding raw fields,
for example:

- encode/decode counter config fields for `CR1`.
- encode/decode channel config fields for `CCMR1` and `CCMR2`.
- encode/decode channel polarity and enable fields for `CCER`.
- encode/decode interrupt-request sources into `DIER` enable masks.
- encode/decode event flags into `SR` observation and W0C acknowledge images.
- compute channel-local field positions.

Public codec APIs use Extract/Stage naming. The first-pass timebase and counter
configuration APIs include:

- `Codec_TIM_ExtractPrescaler(...)`
- `Codec_TIM_StagePrescaler(...)`
- `Codec_TIM_ExtractAutoReload(...)`
- `Codec_TIM_StageAutoReload(...)`
- `Codec_TIM_ExtractCounterValue(...)`
- `Codec_TIM_StageCounterValue(...)`
- `Codec_TIM_ExtractTimeBaseConfig(...)`
- `Codec_TIM_StageTimeBaseConfig(...)`
- `Codec_TIM_ExtractCounterConfig(...)`
- `Codec_TIM_StageCounterConfig(...)`
- `Codec_TIM_ExtractCounterDirection(...)`
- `Codec_TIM_StageCounterDirection(...)`
- `Codec_TIM_ExtractCounterAlignment(...)`
- `Codec_TIM_StageCounterAlignment(...)`
- `Codec_TIM_ExtractOnePulse(...)`
- `Codec_TIM_StageOnePulse(...)`
- `Codec_TIM_ExtractAutoReloadPreload(...)`
- `Codec_TIM_StageAutoReloadPreload(...)`
- `Codec_TIM_ExtractUpdateSource(...)`
- `Codec_TIM_StageUpdateSource(...)`
- `Codec_TIM_ExtractDigitalFilterClockDivision(...)`
- `Codec_TIM_StageDigitalFilterClockDivision(...)`

Channel/PWM and IRQ codec surfaces can build on the same rules:

- `Codec_TIM_ExtractCounterEnableState(...)` / `Codec_TIM_StageCounterEnableState(...)`
- `Codec_TIM_ExtractUpdateEventState(...)` / `Codec_TIM_StageUpdateEventState(...)`
- `Codec_TIM_ExtractChannelConfig(...)` / `Codec_TIM_StageChannelConfig(...)`
- `Codec_TIM_ExtractChannelEnableState(...)` / `Codec_TIM_StageChannelEnableState(...)`
- `Codec_TIM_ExtractChannelPolarity(...)` / `Codec_TIM_StageChannelPolarity(...)`
- `Codec_TIM_ExtractIRQEvents(...)` / `Codec_TIM_StageIRQEventAck(...)`
- `Codec_TIM_ExtractIRQSources(...)` / `Codec_TIM_StageIRQSources(...)`

Every meaningful Stage API should have a conjugate Extract API. Extraction-only
APIs are acceptable for naturally read-only or action-style register behavior.

Codec APIs may validate inputs and should return `driver_status_t` whenever
validation can fail. For binary state extraction APIs, the returned
`driver_status_t` is the decoded state itself, either `DRIVER_STATUS_OFF` or
`DRIVER_STATUS_ON`, with error statuses used only when validation can fail.
Non-state extraction APIs use output pointers and return operation status.

Timer public and codec-visible Doxygen should make value scope visible at the
call site:

- Use `Accepted values` for caller-provided input parameters.
- Use `Expected values` for values written through output parameters.
- Use `Accepted member values` or `Expected member values` when the parameter
  is a structure and the relevant scope is member-level.
- Prefer concrete `@ref` entries over wildcard text so users can jump directly
  to selector definitions.

## Timer Driver Layer

The driver layer owns public behavior.

It must:

- Expose public status-returning APIs in `timer.h`.
- Consume public Timer configuration structures from `timer_config.h`.
- Validate public arguments before touching hardware.
- Own or verify required clocks before register access, according to each API
  ownership boundary.
- Own reset/update-event sequencing.
- Decide which registers must be read.
- Read each required register once.
- Stage changes through codec APIs.
- Write only when the staged image differs from the original image.
- Return `driver_status_t` for primary public mutation/extraction APIs.
- Own Timer DIER/SR sequencing while leaving generic NVIC delivery ownership
  with the NVIC/application layer.

Existing value-returning APIs can remain as wrappers where useful, but the
primary public path should be status-returning.

Current first-pass public API scope:

- counter operation state APIs `TIM_GetOperationState()` /
  `TIM_SetOperationState()`
- conjugate root lifecycle APIs `TIM_Config()` and `TIM_DeConfig()`
- `TIM_GetIRQSources()` / `TIM_SetIRQSources()` for Timer-owned DIER state
- `TIM_GetIRQEvents()` / `TIM_AckIRQEvents()` for Timer-owned SR state
- dedicated 72 MHz-to-1 MHz blocking polling-delay configuration through
  `TIM_ConfigForBlockingDelay()`
- blocking polling delay helpers `TIM_BlockingDelayUs()` / `TIM_BlockingDelayMs()`
- grouped `TIM_GetTimeBaseConfig()` / `TIM_SetTimeBaseConfig()`
- grouped `TIM_GetCounterConfig()` / `TIM_SetCounterConfig()`
- scalar `Get`/`Set` APIs for `prescaler`, `auto_reload`, `counter_value`,
  `direction`, `alignment`, `one_pulse`, `auto_reload_preload`,
  `update_source`, and `digital_filter_clock_division`
- programmed-state convenience getter `TIM_GetProgrammedTickFrequency()`

`TIM_GetOperationState()` and `TIM_SetOperationState()` own only
`TIMx_CR1.CEN`. They do not enable or disable the RCC APB1 clock gate. Public
users must enable the Timer clock gate before calling the operation-state APIs
directly.

### Clock Ownership Policy

Timer public APIs must not hide broad side effects behind narrow names. RCC
owns APB1 clock-gate mutation, NVIC owns interrupt delivery, and Timer owns
only Timer-register configuration, status, acknowledgement, reset, and
`TIMx_CR1.CEN` operation state. `TIM_Config()` and `TIM_DeConfig()` require an
already-enabled application-owned clock gate and never mutate it. Neither
lifecycle entry point calls its conjugate; the application owns their ordering.

Grouped and scalar `Get`/`Set` APIs such as `TIM_GetPrescaler()`,
`TIM_SetAutoReload()`, and `TIM_SetDirection()` do not silently enable the RCC
clock gate. Their narrower contract is to read or modify one Timer-owned
configuration field. Those APIs verify that the RCC APB1 clock gate is already
enabled and return `DRIVER_STATUS_ERROR_STATE` when the clock gate is disabled.
The current implementation centralizes that precondition in the private
`_TIM_ValidateClockEnabled()` helper.

This keeps call-site behavior explicit:

```c
ASSERT_DRIVER_STATUS
(
	RCC_SetPeripheralClockState
	(
		RCC_APB1_BUS,
		RCC_APB1ENR_TIM2EN,
		DRIVER_STATUS_ON
	)
);
ASSERT_DRIVER_STATUS(TIM_SetPrescaler(TIM2, prescaler));
```

Instead of allowing `TIM_SetPrescaler()` to change RCC state as a hidden side
effect.

The implemented `TIM_Config()` contract owns the maximum admitted Timer
configuration transaction. It requires the application-owned Timer clock gate,
validates the complete root request, rejects a running counter, stages the
timebase and counter domains represented by `tim_config_t`, and commits them in
the required hardware order.
It never calls `TIM_DeConfig()`. Deferred domains and all `TIMx_DIER` state
remain unchanged. The timebase commit uses `UDIS=0`, `URS=1`, and `EGR.UG` to
load buffered values without asserting `UIF` or requesting interrupt/DMA
service. Successful configuration leaves the application-owned clock gate
unchanged and the counter disabled without changing Timer IRQ sources or NVIC
state.

### Configuration Decomposition Policy

The target Timer decomposition follows the repository-wide structured
configuration rule. Each admitted `tim_config_t` member owns one reusable
structure-to-image staging path:

```c
_TIM_StageCounterConfig
(
	const tim_config_counter_t* const	pCounterConfig,
	reg* const							pControlRegisterImage
);

_TIM_StageTimeBaseConfig
(
	const tim_config_timebase_t* const	pTimeBaseConfig,
	reg* const								pPrescalerRegisterImage,
	reg* const								pAutoReloadRegisterImage,
	reg* const								pCounterRegisterImage
);

_TIM_StageTimeBaseUpdate
(
	reg* const	pCommitControlRegisterImage,
	reg* const	pUpdateEventRegisterImage
);
```

These helpers perform no MMIO. They validate and transform only their supplied
configuration domain and caller-owned images, delegate field placement to the
Timer Codec, preserve unrelated fields, and publish staged images only after
all fallible work succeeds.

The existing `Codec_TIM_StageCounterConfig()` and
`Codec_TIM_StageTimeBaseConfig()` functions own field placement. Their matching
Driver `_TIM_Stage<Domain>Config()` helpers own the atomic local-copy and
publication boundary for exactly one `tim_config_t` member. Temporary `CR1`
commit policy and the write-only `EGR.UG` action are staged separately through
`_TIM_StageTimeBaseUpdate()` because neither belongs to
`tim_config_timebase_t`.

`TIM_GetCounterConfig()` and `TIM_GetTimeBaseConfig()` form the extraction side
of those grouped domains. Each getter validates its destination and live
preconditions, reads the required images once, and publishes the complete
structure only after Codec extraction succeeds.

The same helpers serve both transaction scopes:

```text
TIM_SetCounterConfig()
  -> validate clock and stopped-state policy
  -> read CR1
  -> _TIM_StageCounterConfig(...)
  -> dirty-write CR1

TIM_SetTimeBaseConfig()
  -> validate clock and stopped-state policy
  -> read PSC, ARR, CNT, and required CR1 context
  -> _TIM_StageTimeBaseConfig(...)
  -> perform the ordered timebase commit

TIM_Config()
  -> validate the complete root and lifecycle contract
  -> read all required current images
  -> _TIM_StageCounterConfig(...)
  -> _TIM_StageTimeBaseConfig(...)
  -> commit only after every represented domain stages successfully
  -> preserve TIMx_DIER without reading or modifying NVIC state
  -> perform one root cleanup path
```

`TIM_Config()` must not call the grouped public setters because each setter
would commit independently and could leave a partially configured root request.
It must also not delegate to a private `_TIM_ApplyConfig()` wrapper that hides
the same maximum transaction. The public root function visibly owns domain
composition, while narrow private helpers own reusable staging.

`_TIM_StageCounterConfig()` produces one final `CR1` image for every admitted
counter transaction. If the request changes direction while the current mode
is center-aligned, staging returns `DRIVER_STATUS_ERROR_STATE` with no MMIO;
the application must explicitly select edge-aligned mode, change direction,
and restore its requested center-aligned mode through separate public calls.
The grouped/scalar counter setters dirty-write the final `CR1` image at most
once. The root transaction coalesces that counter image into the mandatory
timebase update sequence, avoiding a separate counter write before the
temporary update-policy image and final CR1 restoration. Timer staging helpers
consume the public transaction's register snapshots and never reread hardware,
and their modular boundaries do not create separate writes.

Public root/grouped timebase functions directly dirty-write the `PSC` and `ARR`
registers owned by their transaction. Scalar prescaler and auto-reload setters
snapshot and directly dirty-write only their owned preload register. All four
paths reuse `_TIM_StageTimeBaseUpdate()` to prepare the temporary `CR1` and
`EGR.UG` images, then visibly perform the required update action and CR1/CNT
restoration in the owning public function. No private commit helper hides the
actual MMIO sequence or the public API's authority. The former
`_TIM_ApplyCounterConfig()` and `_TIM_ApplyTimeBaseConfig()` wrappers are
removed; public grouped setters now own their narrow transactions and the root
function visibly composes both shared staging paths before its first write.

Timer IRQ-source configuration is a separate explicit sequence:

```c
ASSERT_DRIVER_STATUS(TIM_Config(TIM3, &config));
ASSERT_DRIVER_STATUS(TIM_AckIRQEvents(TIM3, TIMx_IRQ_EVENT_UPDATE));
ASSERT_DRIVER_STATUS(NVIC_ClearPendingIRQ(TIM3_IRQn));
ASSERT_DRIVER_STATUS(NVIC_EnableIRQ(TIM3_IRQn));
ASSERT_DRIVER_STATUS
(
	TIM_SetIRQSources
	(
		TIM3,
		TIMx_IRQ_SOURCE_UPDATE,
		DRIVER_STATUS_ON
	)
);
ASSERT_DRIVER_STATUS(TIM_SetOperationState(TIM3, DRIVER_STATUS_ON));
```

This ordering clears stale Timer/NVIC state and establishes NVIC delivery
before the Timer source can generate a request. It also proves that the
application deliberately requested both layers before starting the counter.
`TIM_Config()` never reads or modifies Timer IRQ sources or NVIC delivery
state.

`TIM_GetIRQEvents()` and `TIM_AckIRQEvents()` are ISR-oriented hot paths. They
validate the instance and caller-owned event data but deliberately do not query
the RCC gate on every interrupt; successful initialization and the complete
IRQ-service lifetime own that invariant. Event observation performs one `SR`
read. Event acknowledgement stages a W0C image without reading `SR`, performs
only the channel-mode reads required by selected capture/compare flags, and
then issues one `SR` write.

General frequency-targeting configuration functions are intentionally absent.
Applications provide explicit prescaler and timebase configuration data. The
sole fixed-frequency exception is `TIM_ConfigForBlockingDelay()`: a narrowly named
bootstrap for the admitted blocking polling-delay service that first validates a 72 MHz
Timer kernel clock, then delegates the canonical configuration to
`TIM_Config()`.
`TIM_GetInputClockFrequency()` exposes the RCC-derived Timer kernel clock
without Timer MMIO or a Timer clock-gate precondition so policy layers can
calculate their explicit prescaler values after System/Core clocks stabilize.
`TIM_GetProgrammedTickFrequency()` remains the separate observational
calculation from that live Timer kernel clock and the programmed PSC value.

`TIM_BlockingDelayUs()` is a blocking polling helper for a dedicated Timer that the
application successfully allocated through `TIM_ConfigForBlockingDelay()` and has not
subsequently reconfigured. It does not create a general delay service and does
not use IRQ/NVIC state. The helper verifies only the Timer access preconditions;
it deliberately does not revalidate PSC or the remaining base configuration.
`TIM_ConfigForBlockingDelay()` permanently enables OPM and update events, disables
ARPE, clears `SR.UIF`, and leaves the counter stopped for the duration of the
application-owned allocation. Each `TIM_BlockingDelayUs()` call updates
`ARR = delayUs - 1`, resets `CNT`, clears `SR.UIF`, starts `CEN`, and blocks
while polling `SR.UIF` until the one-pulse update event completes or the
bounded budget expires. One cleanup path stops the counter when necessary and
clears `SR.UIF` for both success and failure. The delay is a minimum blocking
delay because software setup, polling, and cleanup can add a small positive
overhead. The public `uint16_t` input bounds the accepted range to
`1U..0xFFFFU`, so the maximum requested delay is `65535 us`.

`TIM_BlockingDelayMs()` is a thin blocking wrapper over `TIM_BlockingDelayUs()`. It rejects
`0U`, then performs one `TIM_BlockingDelayUs(TIMx, 1000U)` chunk for each requested
millisecond. Because the wrapper composes repeated microsecond-delay calls, its
delay is also a minimum delay and accumulates the per-chunk software overhead.

### Configuration Latch Policy

Timer configuration is not just "write the register and forget it." Some Timer
register writes feed shadow/preload logic:

- `TIMx_PSC` is loaded into the active prescaler on an update event.
- `TIMx_ARR` may be immediate or preloaded depending on `TIMx_CR1.ARPE`.
- `TIMx_EGR.UG` forces an update event so staged timebase values become active.
- `TIMx_CR1.UDIS` can block that update event, so timebase commits temporarily
  use `UDIS=0`.
- `TIMx_CR1.URS=1` prevents software-generated `UG` from asserting `UIF` or
  requesting interrupt/DMA service.

The current `TIM_Config()` sequence is:

1. Validate `TIMx`, require its application-owned APB1 clock gate, and validate
   every field in @p pConfig before Timer MMIO.
2. Reject a running counter, then snapshot every register image required by
   both represented configuration domains.
3. Stage the complete counter domain into one final `CR1` image through
   `_TIM_StageCounterConfig()`; reject a direction change that is not writable
   in the current center-aligned mode.
4. Stage the complete timebase domain through `_TIM_StageTimeBaseConfig()`, then
   separately stage the temporary update policy and write-only `EGR.UG` action
   through `_TIM_StageTimeBaseUpdate()`.
5. Retain exclusive application ownership from the cached snapshot through the
   commit; do not reread CR1 or issue a separate counter-configuration write.
6. Write only changed `PSC` and `ARR` images, apply the staged counter fields
   together with temporary `UDIS=0` and `URS=1`, then generate `TIMx_EGR.UG` so
   buffered timebase values become active without asserting `UIF`.
7. Apply the requested final `CR1` only when it differs from the temporary
   commit image, then restore the requested `CNT`.
8. Preserve all `TIMx_DIER` state without reading or modifying NVIC delivery;
   both remain explicit application operations.
9. Leave operation disabled until the user calls
    `TIM_SetOperationState(TIMx, DRIVER_STATUS_ON)`.
10. On precommit failure, leave all Timer registers and the application-owned
    clock gate unchanged; never call `TIM_DeConfig()` or reset
    application-owned peripheral state.

`TIM_DeConfig()` requires the application-owned Timer clock gate, pulses only
the corresponding RCC APB1 reset bit, and leaves both clock-gate and NVIC state
unchanged. The reset restores the complete Timer register bank, including
channel, IRQ, DMA, and master/slave state.

Unlike GPIO, Timer should keep a structured configuration API. GPIO can remain
ergonomic with a small fixed argument list because its basic configuration is
only port, pin mask, mode, and config. Timer base configuration already spans
clock period, counter behavior, and update-event behavior; admitted channel
domains will add their own coherent grouped structures. This keeps persistent
configuration modular without hiding operational intent. IRQ-source enablement
is deliberately not a structure member and remains an explicit public action.
The refactor should therefore fix ownership and staging of `tim_config_t`, not
remove the structured base-configuration model.

## Coding Style

Timer files follow the established banner style:

```c
// ==================================================================================================== //
```

Public implementation sections in `timer.c` mirror the public declaration order
in `timer.h`. Each public API section uses the same main banner text as the
header, and conjugate getter/setter APIs are grouped under the same sub-banner:

```c
// ==================================================================================================== //
//										Timer Clock State APIs											//
// ==================================================================================================== //

// -------------------------------------- Timer Clock State Pair -------------------------------------- //
```

For public Timer Doxygen, `@returns` should be function-specific and status
oriented, for example:

```c
@returns @ref driver_status_t "Clock State - Operation Status"
```

Use explicit `@retval` entries that name the relevant parameter with backticks:

```c
@retval DRIVER_STATUS_ERROR_INVALID_ARG @p TIMx / @p clockState was invalid.
```

Source-file overview Doxygen may use `@section` blocks for scope, field
ownership, and source layout when a file has enough architecture to justify it.

Use tabs for indentation.

When a function declaration, function definition, or function-like macro call
has exactly one argument, keep the argument on the same line as the function
name:

```c
driver_status_t TIM_DeConfig(TIM_TypeDef* const TIMx);
ASSERT_DRIVER_STATUS(TIM_ValidateInstance(TIMx));
```

Reserve the multiline parenthesized layout for functions or function-like
macro calls with multiple arguments, or for expressions that cannot stay
readable in a single line.

## Current Checkpoint

Completed:

- Core Timer raw field macros exist for the first codec pass.
- Public Timer scalar aliases live in `timer_data_types.h`.
- Public Timer selectors/defaults/validation helpers live in `timer_defines.h`.
- Public Timer configuration structures live in `timer_config.h`.
- Timer LL exists and exposes dumb full-register read/write helpers.
- Timer codec exists and operates on caller-owned register images.
- Binary state codec extractors return decoded `DRIVER_STATUS_OFF` or
  `DRIVER_STATUS_ON` directly.
- `timer.h` exposes root/timebase/counter, IRQ, and bounded delay APIs.
- RCC owns Timer clock-gate control; the Timer public surface exposes no
  clock-gate mutation or query APIs.
- Counter operation state APIs `TIM_GetOperationState()` and
  `TIM_SetOperationState()` are exposed.
- Grouped and scalar `Get`/`Set` APIs verify that the RCC APB1 clock gate is
  already enabled and return `DRIVER_STATUS_ERROR_STATE` when it
  is not.
- `timer.h`, `timer_config.h`, and `timer_codec.h` now document input
  `Accepted values` and output `Expected values` for the current public and
  codec-visible Timer scope.
- `timer.h` public API return documentation now uses function-specific
  `@returns @ref driver_status_t "... - Operation Status"` labels and
  parameter-specific `@retval` wording.
- `TIM_Config()` never calls `TIM_DeConfig()`; it visibly stages counter and
  timebase configuration before one root-owned commit while preserving
  deferred domains, all DIER fields, and application-owned NVIC state.
- Timer IRQ-source enablement remains explicit through
  `TIM_SetIRQSources()` and is never part of `tim_config_t`.
- `TIM_DeConfig()` requires an enabled application-owned clock, pulses the
  matching RCC reset, and leaves clock-gate and NVIC state unchanged.
- `TIM_ConfigForBlockingDelay()` validates a 72 MHz Timer kernel clock and applies the
  canonical dedicated blocking polling-delay configuration through
  `TIM_Config()`.
- `TIM_BlockingDelayUs()` and `TIM_BlockingDelayMs()` exist as blocking polling helpers for
  Timers explicitly allocated through `TIM_ConfigForBlockingDelay()`. The application
  preserves that dedicated configuration; delay calls do not revalidate it and
  retain bounded polling.
- IRQ source/event APIs use separate types, cover trigger and overcapture
  vocabulary, and never mutate the per-instance NVIC line.
- `timer.c` orchestrates config-owned fields through full-domain validation and
  reusable Driver/Codec staging; each public configuration API directly owns
  its ordered MMIO sequence and `driver_status_t` handling, while RegOps owns
  generic compare/write behavior for independent scalar transactions.
- `timer.c` public implementation sections now mirror `timer.h` banner and
  sub-banner order, and the file overview uses Doxygen `@section` blocks for
  scope, field ownership, and source layout.

Completed PWM extension:
- `timer_pwm.h/.c` implement the Timer-owned PWM channel surface directly in
  this module using the existing Timer Codec and LL. Stopped PWM transactions
  directly load only the selected active CCR and do not generate `EGR.UG`.
- `TIMER_PWM_ARCHITECTURE.md` is the normative PWM ownership, lifecycle,
  arithmetic, transaction, and application-boundary contract.
- `Projects/PWM/06_PWM_Poll` uses the Timer-first, PWM-second, CEN-last
  lifecycle; the legacy heap/handle PWM module has been removed.

## Compatibility Boundary

Initial refactor steps should prefer compatibility over unnecessary renaming:

- Preserve existing public `TIM_*` function names where possible.
- Preserve existing `TIMx_*` selector names initially.
- Keep `tim_config_t` and related public configuration structures as the
  Timer configuration API. `TIMx` stays a function argument, not a config
  member, so one configuration can be reused across Timer instances.
- Update examples only after the public header/API contract changes.

The legacy Timer IRQ compatibility wrappers were removed after the Timer IRQ
example migrated to the four canonical source/event APIs.
