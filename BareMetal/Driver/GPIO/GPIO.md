# GPIO Implementation Guide {#GPIO_Peripheral_Guide}

This directory contains the STM32F1 GPIO driver family. Use this file as the
implementation entry point before editing GPIO code.

This document builds on the repository-wide rules in
`../../../ARCHITECTURE.md` and records the GPIO-specific architecture,
implementation rules, naming policy, and alignment plan.

## Current And Intended File Map

| File | Layer | Responsibility |
|------|-------|----------------|
| `Inc/gpio_data_types.h` | Data Types | GPIO scalar typedef aliases |
| `Inc/gpio_defines.h` | GPIO Defines/Validation | Public GPIO selector macros and pure validation/policy macros |
| `Inc/gpio_ll.h` | Low-Level | Dumb single point for named GPIO register reads/writes and clock forwarding |
| `Inc/gpio_codec.h`, `Src/gpio_codec.c` | Codec | Selector encoding/decoding, raw CRL/CRH field placement, and staged register-image mutation |
| `Inc/gpio.h`, `Src/gpio.c` | Driver | Public GPIO API, validation, sequencing, batching, status returns, and generic GPIO convenience codecs |
| `Inc/gpio_irq*.h`, `Src/gpio_irq*.c` | GPIO IRQ | GPIO-backed EXTI routing, trigger staging, NVIC integration |

The current split is:

- `gpio_data_types.h`: typedef aliases only.
- `gpio_defines.h`: public GPIO selector macros and pure validation macros.
- `gpio_ll.h/.c`: named static inline register accessors exposed with
  `LL_GPIO_*` names.
- `gpio_codec.h/.c`: central selector-to-bitfield encoding/decoding and
  CRL/CRH/ODR register-image staging.

## Goals

- Keep direct GPIO register access in the low-level layer.
- Keep STM32F1 CRL/CRH field placement and selector translation in the codec
  layer.
- Keep GPIO data type aliases independent from selector and validation macros.
- Keep public mode/config selectors away from raw hardware access.
- Keep the low-level layer as the only direct GPIO register access point.
- Make STM32F1 `MODE/CNF` semantics explicit, because `CNF` is interpreted
  differently for input and output modes.
- Batch multi-pin configuration by reading each touched register once, staging
  local images, and writing each dirty register once.
- Keep board concepts such as the Blue Pill on-board LED outside the reusable
  GPIO driver.
- Improve IntelliSense grouping by using the `LL_<Module>_<Action>` naming
  shape for low-level APIs.

## Implementation Rules

- Keep raw register definitions in `BareMetal/Core`.
- Keep GPIO scalar aliases in `gpio_data_types.h`.
- Keep public selector macros and pure validation macros together in
  `gpio_defines.h`.
- Keep direct hardware reads/writes in the low-level layer.
- Keep low-level APIs as named full-register read/write accessors. They should
  not know pin masks, pin indices, CRL/CRH shifts, or public mode/config
  compatibility.
- Use the core `reg` alias for full register-image return values, parameters,
  and local staged register images. Use fixed-width aliases only for smaller
  selectors, indices, masks, and public scalar types.
- Keep public selector translation, raw field placement, and staged image
  mutation in the codec layer.
- Codec staging APIs must take caller-owned register images as input, write
  updated images through explicit output pointers, and return `driver_status_t`.
  The status makes null output pointers and unexpected decode/encode failures
  visible to the driver.
- Codec image-mutating APIs must be designed as conjugate pairs. Every
  `Codec_<Module>_Stage*()` API that writes a value into a caller-owned image
  needs the matching `Codec_<Module>_Extract*()` API that recovers the same
  driver-facing value from a caller-owned image. Read-only hardware images such
  as `IDR` may expose extraction-only APIs because there is no valid staged
  write operation for that register.
- Keep validation, clock sequencing, batching, and write ordering in the driver.
- Keep board-specific behavior outside the generic GPIO driver. The current
  Blue Pill LED codecs live in the BSP module, while generic GPIO pin
  configuration remains in this driver.
- Prefer `GPIO_SetPinModeConfig()` for semantic pin configuration on STM32F1.

## Codec Rationale

`gpio_codec.h/.c` is the compatibility boundary between stable public GPIO
selectors and raw STM32F1 bit definitions.

For example, the driver layer may continue to expose and accept
`GPIO_PIN_MODE_INPUT`, while the codec decides which raw `MODE/CNF/ODR` bits
that selector means for the current hardware family. If the hardware bit layout
or raw definitions change later, the driver API should not change; the codec
mapping should absorb that difference.

Use the codec layer for:

- selector-to-bitfield encoding
- bitfield-to-selector decoding
- staged mutation of caller-owned register images
- image-only side effects such as input pull-up/pull-down ODR staging
- conjugate `Stage*()` / `Extract*()` pairs for every mutable image concept

Do not use the codec layer for direct hardware access, public input validation,
clock sequencing, batching decisions, or board-specific pin policy.

## Layer Ownership

| Layer | GPIO Files | Owns | Must Avoid |
|-------|------------|------|------------|
| Core | `BareMetal/Core/Inc/stm32f1xx_gpio.h` | GPIO register structs, offsets, raw masks, and reset-level hardware facts | Driver selectors, board aliases, public validation |
| Data Types | `Inc/gpio_data_types.h` | GPIO scalar typedef aliases such as `gpio_pin_t`, `gpio_pin_mode_t`, and `gpio_pin_config_t` | Public selector macros, validation macros, `GPIO_TypeDef`, full MCU include coupling, register layout, board behavior |
| Defines/Validation | `Inc/gpio_defines.h` | Public GPIO selector macros and pure validation/policy macros that guard those selectors | Hardware reads/writes, clock sequencing, batching decisions, raw register field placement |
| Low-Level | `Inc/gpio_ll.h` | Named static inline register read/write accessors, raw GPIO clock forwarding, `LL_GPIO_*` API names | Public selector translation, pin-index field mapping, raw CRL/CRH field placement, public compatibility policy, batching decisions, board behavior |
| Codec | `Inc/gpio_codec.h`, `Src/gpio_codec.c` | Encoding/decoding selectors, pin-index to CRL/CRH image mapping, raw CRL/CRH field placement, and caller-owned register-image staging/extraction | Hardware reads/writes, clock sequencing, public API policy |
| Driver | `Inc/gpio.h`, `Src/gpio.c` | Public APIs, explicit pin/mode/config input validation, mode/config compatibility, clock sequencing, read/write batching, dirty tracking, status returns | Raw register map definitions, board-specific shortcuts |
| Board/Project | `BareMetal/Driver/BSP` and `Projects/*` | Blue Pill LED aliases, package pin availability decisions, examples | GPIO internals and raw register assumptions |

## Naming Convention

Use full descriptive header suffixes:

- `gpio_data_types.h` for scalar typedef aliases.
- `gpio_defines.h` for public selector macros and pure validation macros.
- `gpio_codec.h` for selector-to-hardware-bitfield encoding/decoding.
- Do not introduce abbreviated forms such as `gpio_defs.h`.

Use `LL_<Module>_<Action>` for new low-level APIs and macros.

For GPIO, the intended namespaces are `LL_GPIO_*` and `LL_GPIO_IRQ_*`. The LL
layer should be a dumb single point of register access: named static inline
functions read and write full register images, while public selector
compatibility, field placement, and orchestration stay above it.

Preferred:

```c
image = LL_GPIO_ReadCRL(GPIOx);
LL_GPIO_WriteCRL(GPIOx, image);
image = LL_GPIO_ReadODR(GPIOx);
LL_GPIO_WriteODR(GPIOx, image);
image = LL_GPIO_IRQ_ReadIMR();
LL_GPIO_IRQ_WriteIMR(image);
```

Avoid adding new APIs with legacy shapes like these:

```c
GPIO_LL_READ_REG(...);
GPIO_LL_SetPin(...);
GPIO_IRQ_LL_EnableIRQ(...);
```

The implementation exposes only the `LL_GPIO_*` and `LL_GPIO_IRQ_*` names. Do
not keep compatibility aliases for old `GPIO_LL_*`, `GPIO_EXTI_LL_*`,
`GPIO_IRQ_LL_*`, or `LL_GPIO_EXTI_*` shapes inside GPIO.

Use `Codec_<Module>_<Action>` for codec APIs.

For GPIO, the intended namespace is `Codec_GPIO_*`. The codec layer should be a
single point for selector encoding/decoding and register-image mutation:

```c
status = Codec_GPIO_StagePinConfigMode
(
	crxImage,
	odrImage,
	pin,
	config,
	mode,
	&crxImage,
	&odrImage
);
status = Codec_GPIO_ExtractPinConfigMode
(
	crxImage,
	odrImage,
	pin,
	&config,
	&mode
);
status = Codec_GPIO_StagePinOutputState
(
	odrImage,
	pin,
	DRIVER_STATUS_ON,
	&odrImage
);
status = Codec_GPIO_ExtractPinOutputState
(
	odrImage,
	pin,
	&pinState
);
status = Codec_GPIO_ExtractPinInputState
(
	idrImage,
	pin,
	&pinState
);
status = Codec_GPIO_StagePinLockState
(
	lckrImage,
	pin,
	DRIVER_STATUS_ON,
	&lckrImage
);
status = Codec_GPIO_StageLockKeyState
(
	lckrImage,
	DRIVER_STATUS_ON,
	&lckrImage
);
status = Codec_GPIO_ExtractPinLockState
(
	lckrImage,
	pin,
	&lockState
);
status = Codec_GPIO_ExtractLockKeyState
(
	lckrImage,
	&lockKeyState
);
```

Avoid adding new codec APIs with the legacy module-before-layer shape.

Codec APIs that operate on caller-selected pins should accept the driver-facing
`gpio_pin_t` single-pin mask. Codec owns the conversion from that mask to
`gpio_pin_index_t`, register bit position, or field position internally. Static
private helpers inside the codec may use `gpio_pin_index_t` after the public
codec entry point has decoded the mask.

Keep these conjugate pairs complete:

| Stage API | Extract API | Notes |
|-----------|-------------|-------|
| `Codec_GPIO_StagePinConfigMode()` | `Codec_GPIO_ExtractPinConfigMode()` | `ODR` participates only to resolve input pull-up/down |
| `Codec_GPIO_StagePinOutputState()` | `Codec_GPIO_ExtractPinOutputState()` | Operates on ODR images |
| `Codec_GPIO_StagePinLockState()` | `Codec_GPIO_ExtractPinLockState()` | Operates on LCKR pin bits |
| `Codec_GPIO_StageLockKeyState()` | `Codec_GPIO_ExtractLockKeyState()` | Operates on LCKR `LCKK` |
| `Codec_GPIO_IRQ_StagePortRouting()` | `Codec_GPIO_IRQ_ExtractPortRouting()` | Operates on AFIO EXTICR images |
| `Codec_GPIO_IRQ_StageTrigger()` | Driver-owned extraction or future codec extraction | Current public driver does not expose trigger get yet |

Extraction-only APIs are allowed only when the target hardware register is
naturally read-only for driver purposes, such as `Codec_GPIO_ExtractPinInputState()`
for `GPIOx_IDR`.

## Intended Configuration Flow

Public configuration should enter through a semantic mode/config API:

```c
driver_status_t GPIO_SetPinModeConfig(
	GPIO_TypeDef *GPIOx,
	gpio_pin_t pinMask,
	gpio_pin_mode_t mode,
	gpio_pin_config_t config);
```

The driver should:

1. Validate `GPIOx`, `pinMask`, `mode`, and `config`.
2. Validate the mode/config pair.
3. Enable the GPIO port clock when the API is an initialization path.
4. Enable AFIO only when alternate function or EXTI routing requires it.
5. Walk selected pin masks with a `while (remainingPins != GPIO_PIN_NONE)` loop
   that extracts and clears the lowest selected bit each iteration. Do not scan
   all 16 possible pins when only a sparse mask was requested.
6. Pass each extracted single-pin mask into codec APIs. Codec owns conversion
   from `gpio_pin_t` to `gpio_pin_index_t` or register bit position internally.
7. Read each touched `CRL`/`CRH` image once, and read `ODR` only when pull-state
   staging or extraction requires it.
8. Stage each selected pin through codec functions.
9. Write dirty images in the hardware-safe order.
10. Return a user-facing `driver_status_t`.

Driver APIs that configure semantic GPIO state must follow a staged
read-modify-write model: read each touched register image once, mutate the local
image, then write each dirty image once. This applies to `CRL`, `CRH`, and ODR
pull-state staging used by input pull-up/pull-down configuration.

Public output state APIs intentionally use the hardware action registers where
that is the safer and cheaper operation:

- `GPIO_PinSet()` writes `GPIOx_BSRR`.
- `GPIO_PinReset()` writes `GPIOx_BRR`.
- `GPIO_PinToggle()` reads and writes `GPIOx_ODR` because toggle must observe the
  current output latch image.

Do not use direct ODR writes for simple set/reset operations. Reserve direct ODR
access for full-image updates, pull-state extraction/staging, and toggle-style
read-modify-write flows.

For multi-pin masks, the driver should treat the mask as a bit field:

```c
while (remainingPins != GPIO_PIN_NONE)
{
	selectedPin = remainingPins & -remainingPins;
	remainingPins = remainingPins & (remainingPins - 1U);
}
```

The actual implementation should use unsigned casts or small local codecs for
the isolation and clear operations, but the iteration must remain proportional
to the number of selected pins rather than the fixed port width.

Codec functions that operate on one public pin should take a single-pin
`gpio_pin_t` mask. Public driver APIs may accept multi-pin masks; the driver
extracts one selected pin at a time, and codec converts that single-pin mask to
the target raw bit or field position.

For input pull-up/pull-down, program the output-latch pull state before exposing
the new `CNF=10` input-pull configuration in `CRL/CRH`.

## STM32F1 Mode/CNF Rule

On STM32F1, `CNF[1:0]` is not independently semantic:

| MODE | CNF | Meaning |
|------|-----|---------|
| `00` | `00` | Analog input |
| `00` | `01` | Floating input |
| `00` | `10` | Input pull-up/down, selected by `ODR` |
| `00` | `11` | Reserved |
| non-zero | `00` | General-purpose output push-pull |
| non-zero | `01` | General-purpose output open-drain |
| non-zero | `10` | Alternate-function output push-pull |
| non-zero | `11` | Alternate-function output open-drain |

Because of this, standalone public calls that set only mode or only config must
decode the current counterpart field and validate the final pair before writing
any staged image. The preferred public API should configure mode and config
together.

## GPIO IRQ Architecture

GPIO-backed EXTI should use the same ownership rules:

- GPIO IRQ scalar aliases belong in `gpio_data_types.h`.
- GPIO IRQ public selectors and pure validation macros belong in
  `gpio_defines.h`, not in the public IRQ driver header.
- GPIO IRQ LL owns named full-register EXTI/AFIO accessors and AFIO clock
  forwarding only.
- The GPIO IRQ codec layer owns AFIO EXTICR routing-field placement, route
  state extraction, and trigger-image staging through `Codec_GPIO_IRQ_*` APIs.
- GPIO IRQ driver owns GPIO input compatibility checks, AFIO clock sequencing,
  EXTI register batching, NVIC enable/disable policy, and pending-bit ordering.
- GPIO IRQ follows the same conjugate-pair policy: route staging is paired with
  route extraction, and trigger staging should gain trigger extraction before a
  public trigger-get API is introduced.

The GPIO IRQ codec must not include the public GPIO IRQ driver header. It should
include `gpio_defines.h` and use aliases from `gpio_data_types.h` through that
defines header.

## Alignment Plan

1. Normalize naming policy.
   - Use `LL_GPIO_*` and `LL_GPIO_IRQ_*` names.
   - Do not keep compatibility macros for old `GPIO_LL_*`, `GPIO_EXTI_LL_*`,
     `GPIO_IRQ_LL_*`, or `LL_GPIO_EXTI_*` names.
   - Update any remaining external call sites directly.
   - Remove public-policy validation from LL APIs once callers validate before
     entering the low-level layer.

2. Continue GPIO data/defines cleanup.
   - Keep only pure scalar typedef aliases in `gpio_data_types.h`.
   - Keep public pin/mode/config selector macros in `gpio_defines.h`.
   - Keep pure validation/policy macros in `gpio_defines.h`.
   - Keep CRL/CRH layout codecs in codec internals.
   - Review raw MODE/CNF aliases and keep them available only where codec or
     driver code genuinely needs the shared type.
   - Keep `GPIO_Init()` explicit: `GPIOx`, `pinMask`, `mode`, and `config`.
   - Avoid descriptor-only initialization APIs in the GPIO driver.
   - Keep pointer-based port validation out of `gpio_data_types.h`; place it in
     `gpio_defines.h` initially, then move target/package restrictions to board
     policy when that layer exists.

3. Move board behavior out of GPIO.
   - Keep `BSP_GPIO_OB_LED_PORT`, `BSP_GPIO_OB_LED_PIN`, and `BSP_OB_LED_*` in the BSP
     module.
   - Keep generic GPIO unaware of active-low LEDs.
   - Keep `GPIO_LED_Init()` generic because it only applies a fixed GPIO mode
     and configuration to a caller-provided port/pin mask.

4. Keep semantic configuration atomic at the API level.
   - Prefer `GPIO_SetPinModeConfig()` for public pin setup.
   - Keep `GPIO_SetPinMode()` and `GPIO_SetPinConfig()` guarded by decoding the
     current pin field and rejecting invalid final mode/config combinations.
   - Keep public GPIO set/reset on `BSRR`/`BRR`.
   - Keep public GPIO toggle on one ODR read-modify-write path.

5. Preserve input pull write ordering.
   - Stage `ODR` before `CRL/CRH` for pull-up/pull-down configuration.
   - Document the order in the public API notes.

6. Keep status-returning getters as the primary API shape.
   - Keep `GPIO_PinGet(...)` returning `DRIVER_STATUS_OFF` / `DRIVER_STATUS_ON`
     states plus error statuses.
   - Keep `GPIO_GetPinModeConfig(..., gpio_pin_mode_t *mode, gpio_pin_config_t *config)`
     as the primary mode/config extraction API.
   - Keep value-returning getters only as convenience wrappers with documented
     fallback values.

7. Make target/package pin availability explicit.
   - Keep family register maps broad enough for STM32F1.
   - Restrict Blue Pill-visible ports and pins in board or target policy.
   - Avoid accepting GPIOE/F/G for the STM32F103C8T6 LQFP48 board target.

8. Normalize GPIO IRQ.
   - Keep GPIO IRQ scalar aliases in `gpio_data_types.h`.
   - Keep only public GPIO IRQ trigger selector macros in `gpio_defines.h`.
   - Keep AFIO EXTICR port-field encoding private to `gpio_irq_codec.*`.
   - Keep GPIO IRQ codec files/APIs on the `gpio_irq_codec.*` and
     `Codec_GPIO_IRQ_*` naming shape.
   - Keep trigger selectors out of `gpio_irq.h`.
   - Keep `GPIO_IRQ_Init()` as a single public setup call over
     `GPIOx + pinMask + inputConfig + trigger`.
   - Keep GPIO IRQ LL symbols on `LL_GPIO_IRQ_*` and restrict them to named
     full-register accessors plus AFIO clock forwarding.
   - Keep `GPIO_IRQ_Deinit()` route-safe by verifying current AFIO EXTICR
     ownership before clearing a selected line.

9. Clean project examples.
   - Replace obsolete selector names such as `GPIO_PIN_CNF_IN_FLOAT`.
   - Ensure example project names match their folders.
   - Keep examples on public APIs unless the example is intentionally teaching LL use.

## Suggested Commit Order

1. Documentation and naming policy.
2. Data aliases, selector defines, and validation split.
3. Low-level `LL_GPIO_*` / `LL_GPIO_IRQ_*` rename.
4. Codec cleanup and pull-order staging support.
5. Driver API behavior changes.
6. Board module extraction.
7. EXTI type/layer cleanup.
8. Project example migration.

## Before Editing

1. Read `../../../ARCHITECTURE.md`.
2. Decide which layer owns the change.
3. Keep the edit inside that layer unless the change is intentionally part of a
   staged normalization step.
4. Compile at least the touched GPIO source files with the Arm toolchain.

## Useful Compile Check

```bash
/opt/arm-gnu-toolchain-14.3/bin/arm-none-eabi-gcc \
  -mcpu=cortex-m3 -mthumb -DSTM32F103C8T6__ \
  -IBareMetal/Core/Inc \
  -IBareMetal/Driver \
  -IBareMetal/Driver/GPIO/Inc \
  -IBareMetal/Driver/RCC/Inc \
  -IBareMetal/Driver/NVIC/Inc \
  -std=gnu11 -Wall -Wextra -ffreestanding \
  -c BareMetal/Driver/GPIO/Src/gpio.c \
  -o /tmp/gpio.o
```
