# STM32F103C8T6 Timer PWM Architecture {#TIM_PWM_Peripheral_Guide}

## Status and scope

This page records the approved base architecture for pulse-width modulation
through the general-purpose `TIM2`, `TIM3`, and `TIM4` peripherals on the
STM32F103C8T6.

Timer PWM is a Timer-owned output-channel feature. It is not a separate
register-owning peripheral and it is not a peer driver that owns the Timer
timebase, GPIO configuration, RCC clocks, interrupt delivery, or board routing.

The base implementation is intentionally narrow:

- Timer base configuration is completed before PWM configuration.
- Successful `TIM_Config()` completion is the PWM lifecycle admission point;
  Timer PWM does not independently query or mutate the RCC clock gate.
- PWM configuration is completed while the Timer counter is disabled.
- The application enables the Timer only after every required PWM channel is
  configured and assigned a duty cycle.
- GPIO and AFIO configuration remain entirely application-owned.
- PWM frequency remains a Timer timebase property.
- PWM mode/polarity/preload/fast-mode configuration and duty configuration are
  separate public operations.
- No PWM configuration structure, endpoint object, handle, heap allocation, or
  shared-frequency registry is introduced.

This guide must be read with the normative planning and safety rules in
`AGENTS.md` (repository root; Part 2: Driver Stack Top-Down Audit Method). The existing
`BareMetal/Driver/Timer/TIMER_ARCHITECTURE.md` remains the broader Timer stack
reference. Those planning documents are deliberately excluded from generated
Doxygen output, so their repository paths are recorded without Doxygen links.

## Decision register

The following decisions are fixed for the first implementation:

| Topic | Decision |
| --- | --- |
| Repository placement | Add `Inc/timer_pwm.h` and `Src/timer_pwm.c` directly to the existing `Timer` module; do not create a new PWM folder or module. |
| Public identity | Pass `TIMx` and `tim_channel_t` explicitly. Do not hide them in an endpoint enum, group enum, handle, or configuration object. |
| API terminology | Keep `PWM` as the public subject and let `channel` / `channelMask` express selection scope: `TIM_ConfigPWM()` / `TIM_DeConfigPWM()`, `TIM_GetPWMConfig()`, `TIM_GetPWMDuty()` / `TIM_SetPWMDuty()`, and `TIM_GetPWMOutputEnable()` / `TIM_SetPWMOutputEnable()`. Retain `OutputEnable` because it names `CCxE` control without implying the physical output level. |
| Timer lifecycle | The application successfully configures the Timer first, configures PWM second, and enables the Timer last. |
| RCC boundary | Timer PWM neither queries nor mutates the Timer clock gate; successful `TIM_Config()` is its lifecycle prerequisite. |
| Frequency ownership | Timer timebase APIs own PSC, ARR, counter mode, and resulting PWM frequency. Timer PWM does not accept or set frequency. |
| GPIO ownership | The application owns GPIO mode, output speed, alternate-function selection, AFIO remap, and pin lifecycle. |
| PWM-channel configuration | `TIM_ConfigPWM()` accepts a non-empty OR-combined channel mask plus one channel mode, polarity, preload selector, and fast-mode selector applied atomically to the complete selection. |
| Duty configuration | `TIM_SetPWMDuty()` is a separate operation using the already-programmed ARR. |
| Output-enable state | Timer PWM controls selected `CCxE` channel-output-enable bits; `TIM_SetOperationState()` independently controls `CR1.CEN`. |
| Layering | `timer_pwm.c` is a Timer Driver source unit and follows the existing Timer Driver -> Codec -> LL stack. |
| Lower layers | Reuse and extend `timer_codec.*` and `timer_ll.h`; do not create PWM-specific Codec or LL files. |
| Memory | Use no dynamic allocation, opaque handle, caller-owned context, or static ownership registry. |
| Initial routing | Support Timer channel programming independently of physical pin routing; the application is responsible for selecting a valid pin. |

## Required application sequence

The required first-use sequence is:

```text
Application enables required RCC clock gates
        ↓
Application configures GPIO/AFIO for the selected physical output pin
        ↓
Application configures TIMx counter and timebase through TIM_Config()
        ↓
Application configures selected PWM channels through TIM_ConfigPWM()
        ↓
Application assigns each channel duty through TIM_SetPWMDuty()
        ↓
Application enables the required channel output(s)
        ↓
Application enables TIMx through TIM_SetOperationState()
```

The architectural precondition is therefore:

```text
TIM_Config() succeeds
        before
TIM_ConfigPWM() / TIM_SetPWMDuty()
        before
TIM_SetOperationState(TIMx, DRIVER_STATUS_ON)
```

Timer PWM never calls `TIM_Config()` or `TIM_SetOperationState()` on behalf of
the application and never queries or mutates the Timer RCC clock gate. A
successful `TIM_Config()` call is the evidence that the application admitted
the Timer lifecycle before entering the PWM subdomain. This keeps timebase
ownership, channel ownership, clock ownership, and counter operation visible
at the call site.

## Part I — Theory with Register Mapping Bridge

### PWM period and frequency

In edge-aligned up-counting mode, the Timer counter advances from zero through
the programmed auto-reload value. Both endpoints are included, so one PWM
period contains `ARR + 1` Timer ticks.

```text
Timer tick frequency = Timer input clock / (PSC + 1)

Period ticks P = ARR + 1

PWM frequency = Timer input clock / ((PSC + 1) × (ARR + 1))
```

`PSC`, `ARR`, counter direction, alignment, and the resulting frequency are
Timer timebase concerns. PWM duty operations read the already-programmed
period but do not choose or mutate the frequency; PWM configuration does not
access the Timer base.

The application must configure an edge-aligned, up-counting, continuous Timer
timebase suitable for PWM before calling `TIM_ConfigPWM()`.

### Compare value and active time

Each output channel has one capture/compare register, `CCR1` through `CCR4`.
The compare value defines the transition point within the programmed period.

The public duty representation is permille:

```text
0       = 0.0%
1       = 0.1%
500     = 50.0%
1000    = 100.0%
```

For period ticks `P`, active ticks are calculated with round-half-up behavior:

```text
active ticks A = round(duty permille × P / 1000)
```

The implementation uses 64-bit intermediates for the multiplication and
rounding operation even though the public duty value and hardware registers
are narrower.

### PWM mode 1

For edge-aligned up-counting PWM mode 1, the internal output reference is
active while `CNT < CCR` and inactive for the remainder of the period.

```text
PWM mode 1: CCR = A
```

Important boundaries are:

```text
0% duty:   CCR = 0
100% duty: CCR = P
```

### PWM mode 2

PWM mode 2 inverts the placement of the active interval relative to the
compare point. With duty defined as time spent at the selected active polarity:

```text
PWM mode 2: CCR = P - A
```

Important boundaries are:

```text
0% duty:   CCR = P
100% duty: CCR = 0
```

PWM mode therefore controls where the active interval appears within the
period. It does not change the public meaning of duty.

### Exact 0% and 100%

The Timer has 16-bit ARR and CCR registers. Exact endpoint behavior in both
PWM modes requires the value `P = ARR + 1` to fit in CCR. Timer PWM therefore
admits this base-period range:

```text
ARR:          1..65534
Period ticks: 2..65535
CCR:          0..65535
```

`ARR=0` is not a usable PWM period. `ARR=65535` produces 65536 period ticks,
which cannot be written to a 16-bit CCR for the mode-dependent exact endpoint.
PWM duty conversion rejects either boundary instead of silently approximating
or clamping it.

### Polarity

Polarity is applied after the internal output-reference signal:

- Active-high polarity makes the active interval electrically high.
- Active-low polarity makes the active interval electrically low.

Duty always means the fraction of the period spent at the selected active
polarity. Changing polarity changes the electrical level identified as active;
it does not change the duty calculation.

### Channel output-enable state and counter operation state

Two independent enable decisions are involved:

- `TIMx_CCER.CCxE` connects the selected channel output to the Timer output
  path.
- `TIMx_CR1.CEN` starts or stops the shared Timer counter.

Timer PWM owns the public channel-output operation, but the application retains
explicit Timer-counter control. Enabling a PWM channel does not start the
Timer, and disabling a PWM channel does not stop the Timer.

This independence permits several configured channels to share one running
counter while the application changes individual channel output-enable states.

### Preload, fast mode, and update events

CCR preload is caller-selectable. With preload enabled, a CCR write changes
the programmed preload value while the active compare value is transferred at
a natural update event. This prevents an active-period duty update from
changing the compare boundary partway through that same period.

With preload disabled, a CCR write changes the active compare value
immediately. This gives the application direct control but can move the
compare boundary during the current period. The Driver preserves the selected
policy instead of silently forcing period-boundary synchronization.

Output-compare fast mode is also caller-selectable. `OCxFE` controls the
hardware's trigger-to-output response for PWM mode 1 or PWM mode 2; it does not
change the duty equation or the ownership of trigger configuration.

Two cases are distinguished:

1. **Timer stopped and selected output disabled:** Timer PWM writes that lane's
   active CCR directly. If preload is configured enabled, it temporarily
   disables only that lane's preload and restores the configured state before
   returning.
2. **Timer running with preload enabled:** Timer PWM writes the CCR preload and
   allows the next natural update event to activate it.
3. **Timer running with preload disabled:** Timer PWM writes the active CCR
   immediately, accepting the caller-selected mid-period update behavior.

The stopped path deliberately does not write `EGR.UG`. A software update event
is Timer-wide: it can transfer every enabled ARR/CCR preload, reset the
prescaler phase and counter, affect status/request timing, and drive `TRGO`.
Using the selected lane's direct-load window therefore preserves every other
channel's pending preload and all Timer-wide update consumers. If the selected
output is still enabled, the stopped duty setter returns
`DRIVER_STATUS_ERROR_BUSY`; the application explicitly disables it first.

`TIM_Config()` remains responsible for activating the base PSC/ARR state before
PWM channel configuration begins. Timer PWM never repeats that base-owned
update transaction.

### Shared Timer timebase

All four channels of one Timer share:

- `PSC`;
- `ARR`;
- `CNT`;
- counter direction and alignment;
- update events;
- `CR1.CEN`.

Timer PWM does not create a second group abstraction or frequency registry.
`TIMx` itself is the shared group, and its timebase is already configured by
the Timer Driver before any PWM channel is configured.

If the application changes PSC or ARR later, it must stop the Timer and reapply
the duty of every configured PWM channel before enabling the Timer again. Each
selected channel output must be disabled while its stopped-state duty is
reapplied. Timer PWM stores no requested-duty registry from which to repair
channels automatically.

### Register mapping bridge

PWM has no independent register block. TIM2, TIM3, and TIM4 start at
`0x40000000`, `0x40000400`, and `0x40000800`, respectively. Every address below
is the selected instance base plus the stated offset. The LL performs aligned
32-bit transfers even where only the low 16 bits are implemented.

| Concept | Register/field | Software owner | Offset | Implemented width/range | Access class | Semantics and relationships |
| --- | --- | --- | ---: | --- | --- | --- |
| Counter operation state | `CR1.CEN` | Timer base | `0x00` | Bit 0 | R/W | Duty setting reads CEN only to select stopped direct loading or running preload staging; deconfiguration reads it to require a stopped counter. PWM never revalidates or changes Timer-base policy |
| Master trigger selection | `CR2.MMS[2:0]` | Timer synchronization/application | `0x04` | Bits 6:4, encodings 0..7 | R/W | Selects `TRGO`; PWM preserves it and emits no software update action that could create a hidden trigger pulse |
| Slave/external clock control | `SMCR` | Timer synchronization/application | `0x08` | Implemented low 16 bits | R/W | Can make counter state asynchronous to software; PWM does not configure it and requires serialized application ownership |
| IRQ/DMA request enables | `DIER` | Timer IRQ/DMA domains | `0x0C` | Implemented bits 0..4, 6, 8..14 | R/W | PWM neither reads nor writes request enables |
| Event status | `SR` | Timer event domain | `0x10` | Implemented bits 0..4, 6, 9..12 | R/W, write-0-to-clear | PWM never acknowledges flags; unrelated status cannot be lost through a PWM transaction |
| Software event action | `EGR.UG` | Timer base commit domain | `0x14` | UG is bit 0 | Write-only action, self-clearing | UG transfers all enabled preloads and resets counter/prescaler phase; Timer PWM deliberately never writes EGR |
| Channel 1/2 mode lanes | `CCMR1.CCxS`, `OCxFE`, `OCxPE`, `OCxM`, `OCxCE` | Timer PWM per selected lane | `0x18` | Two 8-bit lanes in bits 15:0 | R/W | Selected lane is staged locally; other lane and reserved bits are preserved |
| Channel 3/4 mode lanes | `CCMR2.CCxS`, `OCxFE`, `OCxPE`, `OCxM`, `OCxCE` | Timer PWM per selected lane | `0x1C` | Two 8-bit lanes in bits 15:0 | R/W | Same lane semantics as CCMR1; CCxS is changed only while CCxE is clear |
| Output gate and polarity | `CCER.CCxE`, `CCxP` | Timer PWM | `0x20` | Enable/polarity pairs at bits 0/1, 4/5, 8/9, 12/13 | R/W | Masked state updates stage one complete CCER image; CEN remains independent |
| Counter phase | `CNT[15:0]` | Timer base | `0x24` | `0..65535` | R/W | PWM neither reads nor writes CNT; the configured starting phase remains Timer-base ownership |
| Timer tick divider | `PSC[15:0]` | Timer base | `0x28` | `0..65535`, divide by PSC+1 | R/W, buffered | PWM neither reads nor writes PSC; the base Timer owns frequency |
| Period boundary | `ARR[15:0]` | Timer base | `0x2C` | Hardware `0..65535`; PWM admits `1..65534` | R/W, optionally buffered | PWM reads the programmed period for exact duty arithmetic and never writes ARR |
| Channel compare boundaries | `CCR1..CCR4[15:0]` | Timer PWM selected channel | `0x34`, `0x38`, `0x3C`, `0x40` | `0..65535` | Output: R/W; input: read-only capture | With OCxPE=1, access targets preload; with OCxPE=0, writes affect active compare immediately. An input-mode read can consume capture notification state, so PWM establishes output interpretation first |

GPIO `CRL`/`CRH` and AFIO `MAPR` determine whether a Timer signal reaches a
physical pin, but those registers are outside Timer PWM ownership in this base
architecture.

The hardware behavior above is grounded in the general-purpose Timer chapters
of the
[`STM32F103C Reference Manual`](../../../Reference_Docs/STM32F103C_Reference_Manual.pdf)
and the package/pin information in the
[`STM32F103C8T6 Datasheet`](../../../Reference_Docs/STM32F103C8T6_Datasheet.pdf).

### Applicable errata review

The applicable
[`ES096 STM32F103x8/B errata sheet`](https://www.st.com/resource/en/errata_sheet/es096-stm32f101x8b-stm32f102x8b-and-stm32f103x8b-mediumdensity-device-limitations-stmicroelectronics.pdf)
was reviewed for this scope. Its Timer findings do not require a PWM Driver
workaround because:

- the consecutive-compare limitation states that output modes other than
  toggle operate as expected, while this Driver admits only PWM mode 1/2;
- the output-compare-clear and 100-percent regulation limitations require
  `OCxCE`, which the admitted shape forces and validates disabled;
- the input-capture flag limitations reinforce the rule that configuration
  never reads an input-mode CCR and getters establish output interpretation
  before reading CCR; and
- the TRGO documentation erratum reinforces application-owned synchronization;
  Timer PWM preserves CR2/SMCR and generates no `EGR.UG` action.

## Part II — Implementation with Theory Bridge

### Module classification

Timer PWM is implemented as a cohesive public subdomain of the existing Timer
Driver:

```text
Application
        ↓
timer_pwm.h public Timer PWM API
        ↓
timer_pwm.c Timer Driver orchestration
        ↓
timer_codec.h / timer_codec.c image extraction and staging
        ↓
timer_ll.h named Timer register access
        ↓
STM32F103 Timer registers
```

This classification is why the source and header live directly in the Timer
module. `timer_pwm.c` may use the existing Timer Codec and LL interfaces in the
same way as `timer.c`, but it must not:

- dereference a `TIM_TypeDef` register field directly;
- encode raw CCMR/CCER fields in the Driver layer;
- create `timer_pwm_codec.*` or `timer_pwm_ll.*`;
- call GPIO, AFIO, NVIC, or project code;
- query or mutate RCC clock gates;
- allocate memory;
- call public Timer APIs as a substitute for owning its complete transaction.

Public Timer PWM APIs own validation, register snapshots, Codec staging,
ordered writes, selected-lane preload sequencing, and detailed status
propagation.

### Core representation and volatile-access contract

`TIM_TypeDef` is a volatile Core-owned structure whose member order matches the
hardware offsets above. CR1, CR2, SMCR, DIER, SR, CCMR1, CCMR2, and CCER expose
32-bit `.REG` images; CNT, PSC, and ARR are volatile 32-bit scalar members; each
CCR exposes a read-only input alias and a read/write output alias. EGR is marked
with the project's write-only `_O` convention because standard C has no native
write-only qualifier.

Timer PWM never uses `.BIT` members or a live register pointer. Each LL call
performs one named, aligned, 32-bit volatile transfer. The Driver snapshots
through LL, transforms caller-owned `reg` images through Codec, and sends final
images back through LL. `volatile` guarantees an access is emitted; the Driver
transaction and documentation supply the mode, ordering, and side-effect
rules that C qualifiers cannot express.

### Theory-to-implementation trace

| Public intent | Core registers | LL route | Codec role | Driver transaction |
| --- | --- | --- | --- | --- |
| Configure PWM channel state | CCMR1/2 and CCER | Named CCMR/CCER reads and dirty writes | Stage mode, preload, fast mode, clear-disabled state, polarity, and enable-off state | Trust the completed Timer-base lifecycle, validate selected CCxE state, then commit one final image per touched shared register without accessing CR1, ARR, CNT, or CCR |
| Get PWM channel state | CCMR1/2 and CCER | Named reads | Extract and validate complete admitted shape | Decode into locals and publish mode, polarity, preload, and fast mode only after success |
| Set stopped duty | CR1, ARR, CCMR1/2, CCER, selected CCR | Named reads/writes | Validate shape, calculate/stage compare, stage temporary OCxPE=0 lane | Require CCxE=0, directly load only the selected active CCR, and restore its final CCMR image without reading CNT |
| Set running duty | CR1, ARR, CCMR1/2, CCER, selected CCR | Named reads plus one dirty CCR write | Validate shape and stage mode-correct compare | Update the preload for deferred activation when OCxPE=1 or the active compare immediately when OCxPE=0 |
| Get duty | ARR, CCMR1/2, CCER, selected CCR | Named reads | Extract admitted mode, ARR, and compare scalar | Reconstruct achieved permille duty locally and publish only after success without reading CR1 or CNT |
| Set channel output-enable state | CCMR1/2 and CCER | Read required CCMRs and one CCER; write CCER at most once | Validate selected PWM lanes and stage each CCxE | Validate the complete mask first, then commit one coherent CCER image without touching CEN |
| Get channel output-enable state | CCMR1/2 and CCER | Named reads | Validate PWM shape and extract CCxE | Return ON/OFF directly without collapsing errors |
| Deconfigure one channel | CR1, CCMR1/2, CCER, selected CCR | Named reads/writes | Validate current shape and stage reset selectors/scalar | Require CEN=0 and CCxE=0, then reset only the selected lane, polarity/enable pair, and CCR |

No Timer PWM path accesses PSC or CNT, writes ARR/CR1, accesses EGR/SR,
changes CR2/SMCR/DIER, or performs a subword Timer transfer. Those deliberate
omissions preserve Timer base, synchronization, IRQ/DMA, and event ownership.

### Implementation file layout

Only these new files belong to the base implementation:

```text
BareMetal/Driver/Timer/Inc/timer_pwm.h
BareMetal/Driver/Timer/Src/timer_pwm.c
BareMetal/Driver/Timer/TIMER_PWM_ARCHITECTURE.md
```

Existing Timer files receive only domain-appropriate extensions:

- `timer_data_types.h` owns `tim_pwm_duty_cycle_t` and the register-semantic
  `tim_compare_value_t` scalar used by CCR Codecs.
- `timer_defines.h` owns duty limits and PWM-specific validation helpers.
- `timer_codec.h/.c` own any missing CCR and coherent output-field
  Extract/Stage operations.
- `timer_ll.h` supplies the demanded named CR1, ARR, CCMR1/2, CCER, and
  CCR1-4 access; Timer PWM deliberately does not demand EGR or SR access.
- `timer.h/.c` continue to own the general Timer timebase and counter APIs.

There is no `timer_pwm_config.h` because the agreed public API has no PWM
configuration structure.

### Public scalar type and constants

The public duty type is a Timer-owned scalar alias:

```c
typedef uint16_t tim_pwm_duty_cycle_t;
```

The public limits are:

```c
#define TIM_PWM_DUTY_CYCLE_MIN		((tim_pwm_duty_cycle_t) 0U)
#define TIM_PWM_DUTY_CYCLE_MAX		((tim_pwm_duty_cycle_t) 1000U)
```

No PWM-specific frequency, GPIO, route, endpoint, group, ownership-token, or
handle type is introduced.

### Public API

The admitted base API is:

```c
driver_status_t TIM_ConfigPWM
(
	TIM_TypeDef* const TIMx,
	const tim_channel_t channelMask,
	const tim_channel_mode_t channelMode,
	const tim_channel_polarity_t channelPolarity,
	const tim_channel_oc_preload_t outputComparePreload,
	const tim_channel_oc_fast_t outputCompareFast
);

driver_status_t TIM_GetPWMConfig
(
	TIM_TypeDef* const TIMx,
	const tim_channel_t channel,
	tim_channel_mode_t* const pChannelMode,
	tim_channel_polarity_t* const pChannelPolarity,
	tim_channel_oc_preload_t* const pOutputComparePreload,
	tim_channel_oc_fast_t* const pOutputCompareFast
);

driver_status_t TIM_DeConfigPWM
(
	TIM_TypeDef* const TIMx,
	const tim_channel_t channelMask
);

driver_status_t TIM_SetPWMDuty
(
	TIM_TypeDef* const TIMx,
	const tim_channel_t channel,
	const tim_pwm_duty_cycle_t dutyCycle
);

driver_status_t TIM_GetPWMDuty
(
	TIM_TypeDef* const TIMx,
	const tim_channel_t channel,
	tim_pwm_duty_cycle_t* const pDutyCycle
);

driver_status_t TIM_SetPWMOutputEnable
(
	TIM_TypeDef* const TIMx,
	const tim_channel_t channelMask,
	const driver_status_t outputEnableState
);

driver_status_t TIM_GetPWMOutputEnable
(
	TIM_TypeDef* const TIMx,
	const tim_channel_t channel
);
```

Identity is never embedded in configuration state:

- `TIMx` selects the Timer instance.
- `channelMask` selects one or more Timer channels for one coherent
  configuration, deconfiguration, or output-enable-state transaction.
- `channel` selects exactly one Timer channel for getters and duty-cycle
  operations, because those domains can differ independently per channel.

### Why there is no PWM configuration structure

Channel mode, channel polarity, output-compare preload, and output-compare fast
mode form the narrow PWM-channel configuration transaction and are passed
directly to `TIM_ConfigPWM()`. Duty has a distinct lifecycle and update
contract, so it is configured separately.

Frequency and GPIO output speed are deliberately absent:

- Frequency belongs to the previously configured Timer timebase.
- GPIO output speed belongs to the application-owned GPIO configuration.

A structure containing those values would cross ownership boundaries and
allow independent fields to imply an orchestration policy that Timer PWM does
not own.

### `TIM_ConfigPWM()` contract

`TIM_ConfigPWM()` configures one or more Timer channels for PWM but
never enables a channel or starts the counter. The same `channelMode`,
`channelPolarity`, `outputComparePreload`, and `outputCompareFast` values are
applied to every selected channel as one atomic transaction.

Required preconditions:

- `TIMx` is `TIM2`, `TIM3`, or `TIM4`.
- The application successfully completed `TIM_Config()` and has not
  invalidated the admitted Timer lifecycle.
- The application has not enabled `TIMx_CR1.CEN`; PWM configuration trusts
  this ordering and does not read CR1.
- Every selected `CCxE` bit is clear.
- The application preserved the PWM-suitable Timer base supplied to
  `TIM_Config()`; Timer PWM does not defensively revalidate it.
- `channelMask` contains one or more OR-combined values from
  `TIMx_CHANNEL_1` through `TIMx_CHANNEL_4`.
- `channelMode` is PWM mode 1 or PWM mode 2.
- `channelPolarity` is active-high or active-low.
- `outputComparePreload` is preload disabled or enabled.
- `outputCompareFast` is fast mode disabled or enabled.
- The application calls `TIM_SetPWMDuty()` for every selected channel before
  enabling that channel output.

The transaction:

1. Validates every public argument before Timer MMIO.
2. Reads each touched CCMR plus CCER once and does not access CR1, ARR, CNT,
   or any CCR.
3. Validates every selected `CCxE` before staging any channel mutation.
4. Stages every selected `CCxS` as output, the shared requested PWM mode,
   caller-selected CCR preload and output-compare fast states, and
   output-compare clear disabled.
5. Stages the shared requested polarity while keeping every selected `CCxE`
   disabled.
6. Stages every prospective register image before the first hardware write.
7. Writes CCER at most once, then writes each touched final shared CCMR at
   most once.
8. Returns with every selected channel disabled and its CCR state unchanged.

An error before the first write leaves Timer state unchanged. After the first
write, no further fallible validation or Codec operation is permitted.

When converting an input-capture lane, the Driver never reads or writes CCR,
so capture notification state and the existing compare/capture scalar remain
untouched. Unselected lanes in a touched CCMR are preserved from the single
register snapshot. The separate duty setter owns the first active compare
load before the application enables the output.

### `TIM_GetPWMConfig()` contract

The getter verifies that the selected channel has the complete admitted PWM
shape:

- `CCxS` selects output compare.
- `OCxM` selects PWM mode 1 or PWM mode 2.
- `OCxPE` is a decodable disabled or enabled selector.
- `OCxFE` is a decodable disabled or enabled selector.
- `OCxCE` is disabled because clear-input behavior remains outside PWM scope.
- Polarity is decodable.

All four output pointers are required. The function decodes into local
variables and writes `pChannelMode`, `pChannelPolarity`,
`pOutputComparePreload`, and `pOutputCompareFast` only after the complete
operation succeeds.
Caller output storage remains unchanged on failure.

### `TIM_SetPWMDuty()` contract

The setter accepts one configured PWM channel and a duty value in
`0..1000`. It never changes PWM mode, polarity, PSC, ARR, GPIO state, channel
enable state, or counter enable state.

The transaction:

1. Validates `TIMx`, the single channel, and duty.
2. Reads and validates the channel's admitted PWM configuration.
3. Reads CEN only to select stopped or running sequencing, reads ARR for duty
   arithmetic, and derives `P = ARR + 1` without revalidating Timer-base policy.
4. Rejects an invalid period instead of clamping it.
5. Calculates active ticks and the mode-dependent CCR with 64-bit
   intermediates and round-half-up behavior.
6. Selects the corresponding CCR register.
7. Writes CCR only after all fallible work succeeds.
8. If the Timer is stopped, requires selected CCxE clear, temporarily disables
   selected OCxPE only when configured enabled, writes the selected active CCR
   directly, and restores the caller-selected OCxPE state.
9. If the Timer is running, honors OCxPE: enabled defers activation to the next
   natural update event, while disabled updates the active compare immediately.

With a running Timer and preload enabled, successful return describes
programmed preload state, not proof that the active CCR has already
transferred. With preload disabled, successful return describes an immediate
active compare update that may affect the current period.

The stopped-Timer return contract guarantees that the selected active compare
value was loaded without transferring another channel's pending preload or
generating a Timer-wide update/TRGO action.

### `TIM_GetPWMDuty()` contract

The getter reads the programmed ARR and selected CCR, validates the PWM mode,
reconstructs active ticks according to PWM mode 1 or PWM mode 2, and reports
the achieved duty rounded to the nearest permille value. It does not read CR1
or CNT and does not revalidate Timer-base policy.

No requested-duty value is retained. The getter reports what the programmed
Timer state represents. Its output remains unchanged on failure.

If the Timer is running with preload enabled, the result describes the
readable programmed CCR preload and may differ temporarily from the active
compare value until the next update event. With preload disabled, it describes
the directly updated active compare state.

### PWM channel output-enable-state contracts

`TIM_SetPWMOutputEnable()` changes only `CCxE` for the selected channel mask.
It accepts `DRIVER_STATUS_OFF` and `DRIVER_STATUS_ON` and does not change
`CR1.CEN`. Before enabling, the application must have successfully called
`TIM_SetPWMDuty()` for every selected channel; the driver deliberately keeps
no hidden duty-initialization registry.

Before enabling a channel, the function verifies that every selected channel
has the admitted PWM configuration. It stages one final CCER image and writes
CCER at most once.

For initial startup, the application should enable every required PWM channel
while the Timer is stopped and then call:

```c
TIM_SetOperationState(TIMx, DRIVER_STATUS_ON);
```

Changing `CCxE` while the Timer is already running takes effect immediately.
An enabled channel may therefore join an existing period partway through that
period. Applications requiring aligned first pulses configure and enable all
channels before starting the shared counter.

`TIM_GetPWMOutputEnable()` accepts exactly one channel and returns
`DRIVER_STATUS_OFF`, `DRIVER_STATUS_ON`, or a detailed error status directly.

### `TIM_DeConfigPWM()` contract

Deconfiguration accepts the same non-empty OR-combined channel-mask domain as
configuration. It requires both the Timer counter and every selected channel
output to be disabled. It validates the complete selection before reading any
selected CCR or writing hardware, then resets only the selected channels'
owned fields:

- output-compare mode and selection;
- preload, fast, and clear selectors;
- polarity;
- channel enable;
- selected CCR values.

It snapshots each touched shared register once, stages every selected lane,
writes each touched CCMR and CCER at most once, and clears each selected CCR.
It preserves all unselected channels, the Timer timebase, IRQ/DMA state, RCC
clock gate, NVIC state, GPIO configuration, and AFIO routing. Any failure before
the first write leaves all Timer state unchanged.

### Status behavior

All fallible public APIs return `driver_status_t`.

| Status | Meaning in Timer PWM |
| --- | --- |
| `DRIVER_STATUS_SUCCESS` | Configuration, duty-cycle, output-enable-state, or deconfiguration operation completed. |
| `DRIVER_STATUS_OFF` | Direct output-enable-state getter reports the selected channel disabled. |
| `DRIVER_STATUS_ON` | Direct output-enable-state getter reports the selected channel enabled. |
| `DRIVER_STATUS_ERROR_NULL_PTR` | `TIMx` or required output storage is `NULL`. |
| `DRIVER_STATUS_ERROR_INVALID_ARG` | Instance, channel, mask, channel mode, channel polarity, preload selector, fast-mode selector, duty cycle, or admitted ARR range is invalid. |
| `DRIVER_STATUS_ERROR_STATE` | A required register field is undecodable or the channel does not have the admitted PWM shape. |
| `DRIVER_STATUS_ERROR_BUSY` | Configuration requires every selected channel disabled; deconfiguration requires a stopped counter and every selected channel disabled; stopped-duty loading requires selected CCxE clear. |

Peer and lower-layer statuses are propagated rather than collapsed into a
Boolean result.

### GPIO and AFIO application ownership

The first implementation deliberately performs no pin orchestration. Timer PWM
does not know which package pin carries a selected channel and does not verify
the current remap state.

The application must:

- enable the required GPIO and AFIO clocks;
- select a package-bonded Timer channel pin;
- configure alternate-function push-pull mode and output speed;
- configure or preserve the intended AFIO remap state;
- resolve any SWJ/debug-pin conflict;
- retain the GPIO configuration while PWM output is required;
- restore or repurpose the GPIO after Timer PWM deconfiguration.

For the initial `Projects/PWM/06_PWM_Poll` reference, the application uses the
default non-remapped route:

```text
TIM2 CH4 -> PA3
```

GPIO ownership may be revisited only after the base Timer PWM implementation
is functional and validated. Any future ownership change must update this
guide before changing source behavior.

### Shared-channel behavior without a registry

Timer PWM keeps no hidden group state:

- Multiple channels share whatever PSC/ARR the application programmed.
- Configuring a channel set does not claim the Timer or alter an unselected channel.
- Duty setters derive their result from the current readable ARR.
- Channel output-enable state never starts or stops the shared counter.
- The application decides which channels are enabled before starting TIMx.
- The application stops TIMx before changing the shared timebase.
- The application reapplies every duty value after changing ARR.

This model makes the shared hardware relationship explicit instead of
duplicating it in software state.

### Concurrency and ownership guard

The base Driver contains no lock, critical section, reference count, or ISR
coordination. The application must serialize Timer base and Timer PWM calls for
one `TIMx`, prevent an ISR/DMA peer from mutating the same register images, and
prevent slave/trigger hardware from starting a supposedly stopped counter
during configuration. A getter racing a mutator is likewise outside the base
contract.

This is an explicit exclusive-owner precondition, not hidden state. A future
RTOS, DMA waveform, or synchronized-Timer feature must introduce its own
top-down ownership review before it can share these transactions.

### Documentation boundary

`TIMER_PWM_ARCHITECTURE.md` is the canonical theory, ownership, sequencing, and
implementation guide for Timer PWM.

Source Doxygen will document local callable contracts and link to
@ref TIM_PWM_Peripheral_Guide rather than duplicating this complete theory.
`DRIVER_DOCUMENTATION.md` is updated only after the implementation and guide
have been reviewed together.

## Validation plan

### Host and Codec validation

Tests must cover:

- channel 1/2 selection through CCMR1;
- channel 3/4 selection through CCMR2;
- CCxS output selection;
- PWM1/PWM2 mode staging and extraction;
- mandatory preload and disabled fast/clear fields;
- all four CCER enable and polarity lanes;
- CCR1 through CCR4 selection;
- unrelated-bit preservation;
- no output-image mutation on Codec failure;
- no hardware write on Driver validation failure.

### Duty validation

Pure and Driver-level tests must cover:

- duty `0`, `1`, `500`, `999`, and `1000`;
- invalid duty above `1000`;
- PWM mode 1 and PWM mode 2;
- active-high and active-low polarity semantics;
- ARR `1`, representative intermediate values, and `65534`;
- rejected ARR `0` and `65535`;
- round-half-up boundaries;
- exact 0% and 100%;
- 64-bit intermediate arithmetic;
- readable programmed state versus deferred active preload state.

### Transaction validation

Tests must demonstrate:

- `TIM_ConfigPWM()` rejects an empty or unsupported channel mask;
- `TIM_ConfigPWM()` rejects unsupported preload and fast-mode selectors before
  Timer MMIO;
- `TIM_ConfigPWM()` rejects the complete transaction when any selected
  channel is enabled and performs no hardware write;
- `TIM_ConfigPWM()` does not access CR1, ARR, CNT, or any CCR and trusts the
  completed Timer-base lifecycle;
- Timer timebase fields are preserved;
- IRQ, DMA, RCC, and NVIC state are preserved;
- a mask spanning CCMR1 and CCMR2 preserves every unselected lane and snapshots
  and commits each touched shared register only as required;
- `TIM_ConfigPWM()` returns with every selected channel disabled and every CCR
  unchanged;
- configuration and getter round-trip every legal preload/fast-mode
  combination;
- stopped duty updates require selected CCxE clear and load only the selected
  active CCR without reading CNT or writing EGR, SR, CR1, or another channel;
- running duty updates wait for the next natural update event with preload
  enabled and update active compare immediately with preload disabled;
- multi-channel output-enable-state updates use one staged CCER write;
- `TIM_DeConfigPWM()` rejects the complete transaction when any selected
  channel is enabled or does not have the admitted PWM shape and performs no
  hardware write;
- multi-channel deconfiguration resets every selected lane and CCR while
  preserving every unselected channel;
- getters leave caller output unchanged on failure.

### Build and documentation validation

The implementation wave must include:

- `arm-none-eabi-gcc -fsyntax-only -Wall -Wextra -Werror` for affected Timer
  sources;
- affected Timer, GPIO, Template, and PWM project builds;
- Doxygen generation with warning review;
- direct-register-access and ignored-status searches;
- RCC include, query, and mutation searches in `timer_pwm.c`;
- legacy PWM API and dynamic-allocation searches;
- `git diff --check`.

Temporary host test artifacts remain under `/tmp`; no repository `Test/` or
`Tests/` directory is introduced.

### On-target reference validation

The first board reference must demonstrate:

- application-owned PA3 alternate-function configuration;
- TIM2 configured before its PWM channel;
- TIM2 CH4 configured while TIM2 is stopped;
- the channel output enabled before TIM2 is started;
- PWM output running from the configured Timer timebase;
- exact 0% and 100%;
- representative intermediate duties;
- a running duty ramp using caller-selected CCR preload;
- PWM mode, polarity, preload, and fast-mode behavior for the shared
  selected-channel configuration.

## Deferred scope

The following work is explicitly deferred from the base implementation:

- GPIO/AFIO route discovery, validation, configuration, restoration, or
  ownership transfer;
- Timer-frequency request solving inside Timer PWM;
- hidden Timer ownership, shared-frequency registries, or automatic duty
  repair after an ARR change;
- TIM1 advanced-control output, MOE, complementary outputs, break, and dead
  time;
- remapped Timer channels and SWJ conflict policy;
- center-aligned, down-counting, and one-pulse PWM;
- input capture, encoder, DMA waveform, or interrupt-driven PWM services;
- phase-coherent enabling of a new individual channel into an already-running
  period;
- RTOS/thread/ISR synchronization;
- dynamic allocation and opaque handles.

Deferred features require a new top-down ownership and transaction review. They
must not be added by enlarging `TIM_ConfigPWM()` with unrelated fields.

## Implementation admission gates

Source implementation is commit-admissible only after these Timer PWM safety
gates are resolved:

- coherent programmed-versus-active preload semantics;
- no Timer PWM access to write-only EGR or W0C SR;
- no Timer PWM query or mutation of RCC clock-gate state;
- selected-lane stopped loading without a Timer-wide update event;
- complete no-write-on-error and post-mutation cleanup proof;
- channel-mask validation and CCMR1/CCMR2/CCR selection coverage;
- preservation of unrelated Timer channel, IRQ, DMA, RCC, and NVIC state;
- review of the errata applicable to the deployed STM32F103C8T6 revision.

These gates are implementation prerequisites, not reasons to weaken the
ownership decisions recorded in this guide.
