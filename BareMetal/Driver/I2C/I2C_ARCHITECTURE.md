# I2C Implementation Guide

This document is the living architecture, hardware evidence, audit, deviation,
decision, traceability, implementation-wave, and completion record for the
STM32F103C8T6 I2C stack. It is the planning authority for work under this
directory. It does not authorize SSD1306 integration.

## Document Control

| Field | Value |
| --- | --- |
| Current document status | `VERIFIED` — initial evidence pass complete; accepted Wave 1 contracts are ready for implementation |
| Current implementation status | `DEVIATION` — the active source does not compile warning-clean and implements no usable transaction |
| Last verified commit | `b865ebad95d8d5ee65569182e0f20bedf7792cec` (`2026-08-28`) |
| Evidence date | `2026-08-28` |
| Planning direction | Driver -> Codec -> LL |
| Planning authority | [`../../../AGENTS.md`](../../../AGENTS.md) (Part 2: Driver Stack Top-Down Audit Method) |
| Repository references | `AGENTS.md`, `DRIVER_DOCUMENTATION.md`, USART/Timer/NVIC architecture pages, all files in this directory, the Core I2C header, and the shared CMake source resolver |
| Hardware reference sources | ST RM0008 Rev 21, especially Sections 9.3.9, 11.1.4, 13.3.7, and 26; DS5319 Rev 21; ES096 Rev 15, especially Sections 2.3.6-2.3.8 and 2.8.1-2.8.7 |
| Doxygen publication status | `DEFERRED` — this mutable plan is not yet a reviewed canonical two-part peripheral guide |

Status vocabulary used by this document:

| Status | Meaning |
| --- | --- |
| `UNREVIEWED` | Evidence or a contract has not yet received the required review. |
| `VERIFIED` | The stated claim was checked against named source, build, test, or hardware evidence. |
| `PLANNING-MET` | The architecture decision is frozen, but implementation and retained verification remain open. |
| `DEVIATION` | Current code or documentation differs from the accepted architecture or hardware contract. |
| `PLANNED` | The resolution and its evidence gate are defined but no implementation evidence exists. |
| `IN_PROGRESS` | Work has begun and the exit gate has not passed. |
| `RESOLVED` | The finding is retained and linked to evidence proving its correction. |
| `DEFERRED` | Valid work is intentionally outside the accepted implementation domain and has an admission condition. |
| `BLOCKED` | Required external evidence or authority is unavailable and prevents progress. |

Evidence labels and lifecycle state are independent. Ledger claims use
`[Observed]`, `[Decision]`, `[Gap]`, `[Deferred]`, `[Question]`, and
`[Conclusion]`. Public candidates move through:

```text
Observed -> Candidate -> Admitted -> Contract-frozen -> Implemented -> Verified
```

## Scope

### Current supported domain

No end-to-end I2C capability is currently supported. The active `I2C_Config`
body is empty, has no declared return type, and has no public declaration. The
only active callable header logic is an unchecked enum-to-pointer mapper plus
raw/mixed-responsibility inline LL code. The legacy polling transactions are
excluded by `__OLD_I2C__` and are evidence, not supported behavior.

### Intended supported domain

The first accepted implementation domain is deliberately narrow:

- STM32F103C8T6 `I2C1` and `I2C2` only;
- application-owned APB1 clock gate and GPIO/AFIO routing;
- I2C protocol, not SMBus or PMBus;
- controller/master operation with ordinary unreserved 7-bit target addresses;
- validated Standard-mode timing up to 100 kHz and Fast-mode timing up to
  400 kHz, including both Fast-mode duty choices;
- explicit configuration/deconfiguration and peripheral operation state;
- bounded, synchronous master transmit, master receive, and combined
  write-then-repeated-START-read transactions;
- exact STM32F1 one-byte, two-byte, and multi-byte receive tails;
- deterministic reporting of validation, busy, timeout, NACK, arbitration,
  bus, and overrun failures through `driver_status_t` plus a typed transfer
  result;
- mandatory ES096 workarounds that can be implemented without taking ownership
  of board pin routing; and
- single-owner, non-reentrant operation per I2C instance.

### Explicitly deferred domain

| Domain | Status | Admission condition |
| --- | --- | --- |
| Slave transmitter/receiver | `DEFERRED` | Named consumer, event-latency policy, own-address contract, and complete STOPF/ADDR/error state machine |
| 10-bit controller addressing | `DEFERRED` | Named consumer and frozen ADD10/repeated-START sequences |
| Reserved/general-call/CBUS addresses | `DEFERRED` | Protocol-specific owner and explicit address semantics |
| Multi-master retry policy | `DEFERRED` | Named consumer and application-visible ARLO/retry policy; ARLO detection remains required now |
| Interrupt-driven transactions | `DEFERRED` | Retained state-machine design, ISR ownership, latency proof against ES096, and event/error-vector integration |
| DMA-driven transactions | `DEFERRED` | DMA peer contract, channel-sharing policy, `LAST` sequencing, and ES096 receive-tail proof |
| SMBus/PMBus, PEC, ARP, alert, timeout mode | `DEFERRED` | Separate protocol scope; ES096 2.8.3 limitation accepted by that design |
| Bus recovery by SCL/SDA GPIO toggling | `DEFERRED` to BSP/application | Board route abstraction and exclusive pin ownership contract |
| Thread-safe queues, callbacks, and transaction scheduling | `DEFERRED` | Scheduler/RTOS consumer and explicit synchronization owner |

### Application and BSP ownership

The application or BSP owns GPIO port clocks, AFIO clock/remap state, SCL/SDA
pin mode, external pull-ups, electrical rise time, route conflicts, bus-device
power sequencing, transaction serialization, and board-specific bus recovery.
It also owns the I2C APB1 clock gate, NVIC priority/delivery, and any DMA
controller channel configuration.

### Content prohibited from the generic I2C Driver

The generic Driver must not contain device register addresses, device commands,
framebuffers, display policy, sensor policy, heap allocation, ring-buffer
policy, fixed board pins, hidden RCC/GPIO/AFIO changes, automatic NVIC delivery,
or unbounded retry/polling. SSD1306 is only a future consumer proving the need
for generic master transmit; its address and command format do not shape this
API.

## Authoritative References

### Repository authorities

- [`../../../AGENTS.md`](../../../AGENTS.md) (single authority: Part 1 Coding Preferences; Part 2 Driver Stack Top-Down Audit Method)
- [`../DRIVER_DOCUMENTATION.md`](../DRIVER_DOCUMENTATION.md)
- [`../USART/USART_ARCHITECTURE.md`](../USART/USART_ARCHITECTURE.md)
- [`../Timer/TIMER_ARCHITECTURE.md`](../Timer/TIMER_ARCHITECTURE.md)
- [`../NVIC/NVIC_ARCHITECTURE.md`](../NVIC/NVIC_ARCHITECTURE.md)
- [`../../Core/Inc/stm32f1xx_i2c.h`](../../Core/Inc/stm32f1xx_i2c.h)
- [`../../CMake/Modules/STM32F103Sources.cmake`](../../CMake/Modules/STM32F103Sources.cmake)

### Primary hardware authorities

- [RM0008 Rev 21](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf): I2C behavior and registers; AFIO remap; interrupts; DMA mapping.
- [`../../../Reference_Docs/STM32F103C_Reference_Manual.pdf`](../../../Reference_Docs/STM32F103C_Reference_Manual.pdf): repository copy of RM0008 Rev 21.
- [`../../../Reference_Docs/STM32F103C8T6_Datasheet.pdf`](../../../Reference_Docs/STM32F103C8T6_Datasheet.pdf): DS5319 Rev 21, concrete medium-density part/package capability and pins.
- [ES096 Rev 15](https://www.st.com/resource/en/errata_sheet/es096-stm32f101x8b-stm32f102x8b-and-stm32f103x8b-mediumdensity-device-limitations-stmicroelectronics.pdf): medium-density STM32F103C8/B limitations for every listed silicon revision.

The repository did not contain ES096 at the verified commit. The official ST
copy above is therefore required evidence. Hardware manuals supersede source
comments. ES096 2.8.4 names `CR2` while discussing `START`/`SWRST`; RM0008
locates both fields in `CR1`, and implementation must use `I2Cx_CR1`.

## Hardware Evidence Ledger

| Evidence ID | Source | Verified behavior | Consequence |
| --- | --- | --- | --- |
| HW-001 | RM0008 26.3.3 | `START` enters master mode only when `BUSY=0`; `SB` clears through SR1 read then address write to DR. | Every transaction uses bounded BUSY/SB waits and one ordered address write. |
| HW-002 | RM0008 26.3.3, 26.6.6-26.6.7 | `ADDR` clears only by SR1 read followed by SR2 read; an unrelated SR2 read after SR1 can accidentally clear a newly set ADDR. | LL exposes SR2 read, but Driver calls it only inside a confirmed ADDR-clear sequence. |
| HW-003 | RM0008 Figures 273-277 | Transmit closes on EV8_2; receive tails differ for one, two, and more than two bytes. | One generic RXNE loop is forbidden. |
| HW-004 | RM0008 26.6.1 | While START, STOP, or PEC is set, software must not write CR1 until hardware clears that action. | Driver serializes CR1 action writes and polling; generic LL set/clear/toggle helpers are forbidden. |
| HW-005 | RM0008 26.6.8-26.6.9 | CCR and TRISE are configured only while `PE=0`; CCR minimum is 4 except Fast 16:9 minimum 1. | Configuration validates disabled state and stages timing before the first write. |
| HW-006 | RM0008 26.4 | Event request is `ITEVTEN && (SB || ADDR || ADD10 || STOPF || BTF || (ITBUFEN && (TxE || RxNE)))`; error request is `ITERREN && (BERR || ARLO || AF || OVR || PECERR || TIMEOUT || SMBALERT)`. | Peripheral request generation is separate from NVIC delivery; enabling a gate with a live flag can request service immediately. |
| HW-007 | RM0008 11.1.4 | Event/error IRQs are separate: I2C1 31/32 and I2C2 33/34. | NVIC owns four independent delivery lines. |
| HW-008 | RM0008 13.3.7 Table 78 | DMA1 channels: I2C2 TX/RX 4/5; I2C1 TX/RX 6/7. | DMA is possible but shares channels and needs a peer contract. |
| HW-009 | ES096 2.8.1-2.8.2 | Late event service can duplicate/miss/corrupt data; polling receive method 2 needs interrupt exclusion around critical tail operations. | Accepted receive tails define bounded critical sections and retained MMIO-order tests. |
| HW-010 | ES096 2.8.4 | A misplaced STOP can block master entry; SB wait timeout/BERR during START requires software reset. | START polling performs deterministic recovery and never hangs. |
| HW-011 | ES096 2.8.5 | Standard-mode repeated START at 88-100 kHz can violate setup time with slow rise or slave stretching. | Combined transfers above 88 kHz Standard mode are rejected; use <=88 kHz or Fast mode. |
| HW-012 | ES096 2.8.7 | Analog-filter lock can hold BUSY despite reset; workaround requires GPIO transitions then SWRST. | Driver reports stuck BUSY; BSP/application owns route-specific line recovery. |
| HW-013 | ES096 2.3.6-2.3.8 | Clocked I2C1/I2C2 can conflict with PB5/PB12 alternate functions. | BSP/application must validate board-wide AF conflicts; Driver cannot infer them. |

### Open Questions and Required Evidence

No unresolved question blocks Wave 1. Later accepted waves retain these
questions so implementation details cannot silently become architecture:

| Question ID | Scope/wave | Decision or claim requiring evidence | Required evidence | Owner/admission condition | Status |
| --- | --- | --- | --- | --- | --- |
| I2C-Q-001 | Wave 5 receive tails | Exact repository primitive for save/mask/restore of processor interrupts | Fake-MMIO ordering trace plus restoration tests from both initially enabled and initially masked states | Driver implementation review before Wave 5 | `DEFERRED` to Wave 5 |
| I2C-Q-002 | Wave 6 recovery | Exact configuration image restored after CR1.SWRST and the failure state if restoration cannot complete | Frozen recovery transaction, injected-failure trace, and target START-failure test | Driver/Codec review before Wave 6 | `DEFERRED` to Wave 6 |
| I2C-Q-003 | Wave 8 hardware closure | Which board, targets, pull-ups, and fault-injection setup supply retained electrical evidence | Named fixture, measured SCL rates, ACK/NACK cases, IRQ-load receive stress, and safe stuck-bus test | Hardware integration owner before final verification | `DEFERRED` to Wave 8 |

## Instance Capability Table

| Instance | Base | Bus/kernel clock | RCC gate/reset | IRQ lines | DMA1 routes | Board-owned routes | Errata/shared resources |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `I2C1` | `0x40005400` | APB1 / `PCLK1`, target maximum 36 MHz | EN/RST bit 21 | event 31, error 32 | TX ch6, RX ch7 | default PB6/PB7; remap PB8/PB9; SMBA PB5 | ES096 I2C limits; PB5 conflicts with remapped SPI1/TIM3 use |
| `I2C2` | `0x40005800` | APB1 / `PCLK1`, target maximum 36 MHz | EN/RST bit 22 | event 33, error 34 | TX ch4, RX ch5 | PB10/PB11; SMBA PB12 | ES096 I2C limits; PB12 conflicts with SPI2/USART3 synchronous use |

Both instances implement the same nine-register block and controller/slave I2C
features. Neither owns its pins or APB1 clock. I2C1 remap is AFIO state;
I2C2 has no remapped SCL/SDA route on this target.

## Current File Map

| File | Actual current responsibility | Audit result |
| --- | --- | --- |
| `Inc/i2c.h` | Active selector/types/config declarations; unchecked header LUT; large `__OLD_I2C__` raw transaction surface | Driver, data types, defines, config, LL, and legacy device-helper responsibilities are mixed. No active transaction API is declared. |
| `Inc/i2c_config.h` | Entire file commented out; historical GPIO/RCC/config helpers | Unused legacy text with incorrect clock/GPIO ownership and duplicate `i2c_config_t`. |
| `Inc/i2c_ll.h` | Full read/write/set/clear/toggle surface, RCC mutation, PE control, private selectors, and field staging | LL, Codec, RCC, and Driver policy are mixed. Unsafe RMW exists for DR/SR1/SR2/action-bearing CR1. |
| `Inc/i2c_irq.h` | Entire file commented out | No active IRQ API or state-machine contract. |
| `Inc/i2c_dma.h` | Entire file commented out | No active DMA contract. |
| `Inc/i2c_rb.h` | Entire file commented out | Device/application buffering is misplaced in the peripheral stack. |
| `Src/i2c.c` | Empty active `I2C_Config`; compiled-out unbounded polling and register-oriented helpers | Active implementation is nonfunctional and warning-clean compilation fails. Legacy code contains sequencing, validation, error, and concurrency defects. |
| `Src/i2c_config.c` | Entire file commented out | Historical timing/GPIO/clock implementation only. |
| `Src/i2c_irq.c` | Entire file commented out | Historical CR2 interrupt-gate RMW only; no event/error handling. |
| `Src/i2c_dma.c` | Comment-only include | Unused placeholder. |
| `Src/i2c_rb.c` | Entire file commented out | Historical ring-buffer-to-DR coupling, unbounded polling, raw Boolean results. |

Current dependency direction is not layered: `i2c.h` includes LL and RCC,
`i2c_ll.h` includes RCC LL and mutates peer clocks, public semantics are
duplicated in Driver and LL, and legacy device helpers perform direct MMIO.
There is no Codec layer.

## Intended File Map

| File | Intended responsibility | Disposition |
| --- | --- | --- |
| `Inc/i2c_data_types.h` | Scalar aliases, address type, sizes, error/event masks, transfer result | `PLANNED` new file with admitted consumers |
| `Inc/i2c_defines.h` | Public selector values and pure validation macros | `PLANNED` new file with admitted consumers |
| `Inc/i2c_config.h` | Instance-independent timing configuration structure | `PLANNED` replace commented legacy content |
| `Inc/i2c.h`, `Src/i2c.c` | Public Driver contracts, validation, RCC verification/reset composition, configuration, bounded transactions, recovery | `PLANNED` retain names and replace content |
| `Inc/i2c_codec.h`, `Src/i2c_codec.c` | Pure timing/address/error/CR1 register-image extraction and staging | `PLANNED` new files demanded by frozen Driver paths |
| `Inc/i2c_ll.h` | Named single-transfer register/data-port access only | `PLANNED` replace mixed implementation; remain header-only |
| `i2c_irq.*` | No separate core file without an admitted asynchronous state machine | `DEFERRED`; remove commented placeholders in cleanup wave |
| `i2c_dma.*` | No separate core file without a DMA peer contract | `DEFERRED`; remove commented placeholders in cleanup wave |
| `i2c_rb.*` | No generic I2C ownership | `PLANNED` removal; buffering belongs above I2C |

Files are created only when an accepted Driver path consumes them. A future
interrupt or DMA subdomain may earn a cohesive file pair; symmetry alone is
not sufficient.

## Goals

- Every public call validates pointers, instance, address, length, state, and
  timeout budget before transaction MMIO.
- Configuration derives deterministic legal CR2.FREQ, CCR, and TRISE images
  from the live APB1 clock and requested preset.
- All waits are bounded and inspect relevant error flags on every iteration.
- Public addresses are unshifted 7-bit values; Codec creates the address byte.
- Receive output and progress publication have explicit partial-failure rules.
- NACK, ARLO, BERR, OVR, hardware TIMEOUT, software timeout, and stuck BUSY are
  distinguishable in a typed result.
- ADDR, ACK, POS, STOP, BTF, repeated START, and ES096 ordering is preserved by
  named Driver sequences and trace tests.
- Each required register sample/write occurs once unless hardware sequencing
  explicitly requires another access.
- IRQ source generation, NVIC delivery, DMA requests, and transaction state
  remain separate domains.
- No device- or board-specific policy enters the Driver.

## Architectural Invariants

1. Driver owns public validation, lifecycle, transaction policy, event order,
   bounded polling, error mapping, cleanup, and exact progress publication.
2. Codec performs pure transformations over caller-owned `reg` images and
   domain values; it performs no MMIO, polling, topology, or sequencing.
3. LL performs one dumb mechanical volatile transfer per call. It performs no
   validation, translation, RCC/NVIC/DMA policy, or generic RMW.
4. Core defines the register model, offsets, qualifiers, and raw fields only.
5. Application/BSP owns GPIO/AFIO route, pull-ups, electrical constraints, and
   GPIO-based recovery.
6. RCC/application owns the APB1 clock gate. `I2C_DeConfig` may request the RCC
   reset pulse but leaves the gate unchanged.
7. Peripheral source generation and NVIC delivery are independent domains.
8. No polling loop is unbounded and no error is silently collapsed.
9. No preservation assumption is implicit: every staged mask and reset/default
   policy is recorded.
10. No volatile register is reread merely because helpers are modular.
11. `DR`, SR1 acknowledgements, ADDR clearing, START, and STOP are data/action/
    side-effect transactions, never ordinary dirty-write storage.
12. A transaction has one exclusive owner from preflight through cleanup; the
    initial Driver is non-reentrant and is not callable concurrently or from an
    ISR for the same instance.

## Layer Ownership

| Layer | Owns | Must not own |
| --- | --- | --- |
| I2C Driver | Public contracts, topology switches, RCC precondition/reset calls, configuration orchestration, polling/state machine, critical receive tails, result publication, cleanup | Raw field placement when Codec applies; GPIO/AFIO; NVIC delivery; DMA channels; device protocol |
| I2C Codec | Timing calculation, semantic selector encoding/decoding, 7-bit address-byte encoding, CR1 persistent/action image staging, SR1 error extraction/ack images | Peripheral pointers, volatile access, loops, RCC/NVIC/DMA, transaction order |
| I2C LL | Named 32-bit register reads/writes, 32-bit DR port read/write, SR1 error-ack write | Set/clear/toggle RMW, validation, selectors, clocks, waits, flag interpretation |
| Core | `I2C_TypeDef`, base mapping, raw positions/masks, correct access qualifiers | Driver selectors, timing policy, status mapping |
| RCC | APB1 frequency observation, clock-gate query/mutation, reset pulse | I2C transaction policy |
| NVIC | Priority, pending, enable, active, and delivery for IRQs 31-34 | I2C flag/source acknowledgement |
| DMA | Channels 4-7 configuration and controller lifecycle | I2C `DMAEN`/`LAST` sequencing or transaction termination |
| Ring Buffer | Generic software storage only | I2C protocol, DR access, START/STOP, waiting |
| BSP/application | Pins, remap, pull-ups, serialization, IRQ integration, GPIO bus recovery, device protocols | Raw I2C register sequencing |

## Consumer and Intent Inventory

| Intent | Consumer evidence | Required guarantee | Disposition |
| --- | --- | --- | --- |
| Configure either hardware instance for a legal bus rate | Generic application intent; Core and CMake expose both instances | Deterministic timing from live PCLK1; no pin/clock mutation | `ADMITTED` |
| Transmit a byte span to a 7-bit target | Generic I2C controller intent | Bounded complete message with STOP and exact NACK/error result | `ADMITTED` |
| Receive a byte span from a 7-bit target | Generic I2C controller intent | Correct N=1/N=2/N>2 tail and partial-progress contract | `ADMITTED` |
| Write then read without releasing the bus | Generic register-addressed-device intent | BTF-complete write phase and repeated START | `ADMITTED` |
| Raw START/STOP/DR control | Legacy/SSD1306 code only | Exposes protocol internals and defeats cleanup | `REJECTED` as application API |
| Register-byte/block helpers | Legacy/SSD1306 code only | Assumes an 8-bit device subaddress protocol | `FEATURE DRIVER` above generic transfer APIs |
| SSD1306 display traffic | Existing future consumer | Generic transmit only | `DEFERRED`; no display edits or design influence |
| IRQ/DMA/ring-buffer transfer | Placeholder files, no current project | Complete asynchronous/peer ownership | `DEFERRED` |

No current project selects `I2C` directly. Existing SSD1306 sources call the
compiled-out legacy surface, but SSD1306 integration and migration are outside
this branch scope.

## Existing Public API Disposition

### Active Driver/types surface

| Existing symbol | Current declaration/definition and caller | Disposition | Reason/evidence |
| --- | --- | --- | --- |
| `i2c_t`, `I2C_1`, `I2C_2` | `i2c.h`; used by empty `i2c.c` only | `REMOVE` | Use validated `I2C_TypeDef *` identity like Timer/USART; enum mapper permits out-of-bounds indexing. |
| `I2C_D2L_GetInstance` | Header-defined; no caller | `REMOVE` | Unsafe public LUT and redundant pointer conversion. |
| `i2c_protocol_speed_mode_t`, `I2C_PROTOCOL_SPEED_MODE_SM/FM` | `i2c.h`; no active caller | `REPLACE` | Speed preset must also encode the ES096-safe standard option and remove mode ambiguity. |
| `i2c_protocol_speed_fm_duty_t`, `I2C_PROTOCOL_SPEED_FM_DUTY_2_1/16_9` | `i2c.h`; no active caller | `RENAME/KEEP SEMANTICS` | Move to data-types/defines; correct high/low documentation. |
| `i2c_protocol_ack_t`, `I2C_PROTOCOL_ACK_DISABLE/ENABLE` | `i2c.h`; no active caller | `INTERNALIZE` | ACK is transaction state, not caller configuration in master-only scope. |
| `i2c_protocol_stretch_t`, `I2C_PROTOCOL_STRETCH_DISABLE/ENABLE` | `i2c.h`; no active caller | `DEFER` | NOSTRETCH affects slave mode only. |
| `i2c_protocol_t` | `i2c.h`; no active caller | `REPLACE` | Bitfields mix master timing and slave receive policy and encode no requested rate. |
| `smbus_protocol_t` | Empty `i2c.h` structure | `REMOVE/DEFER` | Empty structures are nonportable and SMBus is unadmitted. |
| `i2c_address_mode_t`, `I2C_ADDRESS_MODE_7BIT/10BIT` | `i2c.h`; no active caller | `REPLACE/DEFER` | Freeze 7-bit controller address type; 10-bit remains deferred. |
| `i2c_dual_address_t`, `I2C_DUAL_ADDRESS_DISABLE/ENABLE` | `i2c.h`; no active caller | `DEFER` | Slave-only. |
| `i2c_general_call_t`, `I2C_GENERAL_CALL_DISABLE/ENABLE` | `i2c.h`; no active caller | `DEFER` | Slave/reserved-address protocol only. |
| `i2c_address_t` | `uint16_t` in `i2c.h` | `REPLACE` | Public accepted address is an unshifted 7-bit semantic alias. |
| `i2c_slave_address_config_t` | `i2c.h`; no active caller | `DEFER` | Claims unsupported 10-bit dual-address combinations; OAR2 is seven-bit only. |
| `i2c_mode_t`, `I2C_MODE_MASTER/SLAVE` | `i2c.h`; no active caller | `REMOVE/DEFER` | Hardware role is transaction/event state; first scope is controller only. |
| `i2c_config_t` | `i2c.h`; consumed only by empty source definition | `REPLACE` | New config contains timing domain (bus_speed_khz, fast_duty_ratio, disable_clock_stretching). |
| `I2C_Config` | Definition only, implicit `int`, empty body | `KEEP NAME/REPLACE CONTRACT` | Canonical root configuration API returns `driver_status_t` and accepts pointer instance/config. |

### Guarded legacy Driver surface

All rows below are inside `__OLD_I2C__`, so they have no active declaration or
linkage unless a nonstandard build defines that macro.

| Existing symbol | Actual callers | Disposition |
| --- | --- | --- |
| `I2C_readAddress`, `I2C_writeAddress` | Legacy I2C code/SSD1306 | `INTERNALIZE` as Codec address-byte encoding |
| `I2C_Init` | None found | `REMOVE`; configuration and operation state remain explicit |
| `I2C_busReady` | Legacy I2C/SSD1306 | `REPLACE` with fallible bounded Driver precondition |
| `I2C_sendStart`, `I2C_sendStop`, `I2C_writeByte`, `I2C_readByte` | Legacy I2C/SSD1306 | `INTERNALIZE`; raw transaction steps cannot be public safe APIs |
| `I2C_Master_Write_Start`, `I2C_Master_Write_Data`, `I2C_Master_Stop`, `I2C_Master_Write_Byte` | SSD1306 legacy paths | `REPLACE` with one bounded `I2C_MasterTransmit` transaction |
| `I2C_Master_Write_Start_Reg`, `I2C_Master_Read_Start` | SSD1306/register helpers | `REPLACE` with generic combined transfer; device subaddress stays above I2C |
| `I2C_Write_Reg_Byte`, `I2C_Write_Reg_Block`, `I2C_Read_Reg_Byte`, `I2C_Read_Reg_Block` | Legacy source; possible SSD1306 use | `REMOVE` from generic Driver; device-register protocol belongs to consumer |

### Current LL and placeholder surfaces

| Existing family/symbol | Disposition | Reason |
| --- | --- | --- |
| `__I2C_Read/WriteCR1`, `CR2`, `OAR1`, `OAR2`, `DR`, `SR1`, `SR2`, `CCR`, `TRISE` | `RENAME/RESTRICT` to `LL_I2C_*`; retain only hardware-legal directions demanded by Driver | Named one-transfer LL is required. |
| Every `__I2C_Set*`, `__I2C_Clear*`, `__I2C_Toggle*` for those nine registers | `REMOVE` | Generic RMW is unsafe for action, data, status, and hardware-modified registers and duplicates Codec/Driver ownership. |
| `I2C_CLK_POS`, `I2C_CLK_MASK`, `_I2C_EnableClock`, `_I2C_DisableClock` | `REMOVE` | RCC/application owns clock gates; Driver uses explicit instance-to-mask switch for validation/reset. |
| `_I2C_Enable`, `_I2C_Disable` | `REPLACE` with public operation-state pair plus Codec/LL path | Missing types/status and hides Driver validation. |
| `_i2c_protocol_*` aliases/selectors and four `_I2C_StageProtocol*` helpers | `MOVE/REPLACE` in Codec | Semantic translation is not LL; current switch defaults silently preserve invalid input. |
| Commented `I2C_Calc_tRISE`, `I2C_Calc_CCR`, `I2C1/2_Load_Default`, `I2C_Clk_*`, `I2C_Enable/Disable` | `REMOVE/REPLACE` | Stale duplicate policy with hidden RCC/GPIO ownership. |
| Commented `I2C_IRQ_Enable/Disable` | `DEFER` | Gates only, no state machine, flag model, NVIC boundary, or statuses. |
| Commented `I2C_DMA_enable/disable` | `DEFER` | No DMA peer lifecycle or LAST/EOT contract. |
| Commented `I2C_RB_TX_Byte/Block` | `REMOVE` | Ring buffer does not own I2C transactions; raw Boolean results collapse errors. |

## Target Public API

The following is contract-frozen for the first implementation wave. Exact
Doxygen is implemented with the code and must follow coding preferences.

```c
driver_status_t I2C_DeConfig(I2C_TypeDef* const I2Cx);
driver_status_t I2C_Config(I2C_TypeDef* const I2Cx, const i2c_config_t* const pConfig);
driver_status_t I2C_GetOperationState(I2C_TypeDef* const I2Cx);
driver_status_t I2C_SetOperationState(I2C_TypeDef* const I2Cx, const driver_status_t operationState);

driver_status_t I2C_MasterTransmit
(
	I2C_TypeDef* const           I2Cx,
	const i2c_address_t          address,
	const i2c_data_t* const      pData,
	const i2c_transfer_size_t    size,
	const uint32_t               timeout_ms
);

driver_status_t I2C_MasterReceive
(
	I2C_TypeDef* const           I2Cx,
	const i2c_address_t          address,
	i2c_data_t* const            pData,
	const i2c_transfer_size_t    size,
	const uint32_t               timeout_ms
);

driver_status_t I2C_MasterWriteRead
(
	I2C_TypeDef* const           I2Cx,
	const i2c_address_t          address,
	const i2c_data_t* const      pWriteData,
	const i2c_transfer_size_t    writeSize,
	i2c_data_t* const            pReadData,
	const i2c_transfer_size_t    readSize,
	const uint32_t               timeout_ms
);
```

Contract rules:

- `I2Cx` is exactly `I2C1` or `I2C2`.
- `address` is the unshifted seven-bit value `0x08U..0x77U`; Codec inserts the
  R/W bit. Reserved addresses are rejected.
- Transfer sizes and `timeout_ms` are nonzero. Buffer pointers are
  non-`NULL`. Zero-byte transfers are rejected rather than interpreted as a
  quick command.
- Configuration/lifecycle APIs require the application-owned APB1 clock gate.
  Configuration requires `PE=0` and a free bus and leaves `PE=0`.
- Transfer APIs require `PE=1`, a legal configured I2C timing image, no
  concurrent owner, and a free bus before a fresh START.
- `timeout_ms` is a transaction-wide timeout in milliseconds. Each hardware
  sample consumes time; it is not reset per byte, so work remains globally bounded.
- Transfer APIs report typed failure through their return status; `DRIVER_STATUS_ERROR_FAIL` never hides the hardware cause.
- A receive destination is unchanged beyond `rxBytesTransferred`; the prefix
  may contain received bytes on transaction failure. Whole-buffer rollback is
  impossible without caller-provided scratch storage and is not claimed.
- Success means the requested STOP was generated (or the final read completed
  after STOP request), ACK/POS defaults were restored, selected software-
  clearable error flags were acknowledged, and all requested bytes completed.
- The API is non-reentrant per instance. No task, ISR, DMA, or other code may
  access that instance from preflight through cleanup.
- A combined transfer uses a repeated START and is rejected when configured in
  Standard mode above 88 kHz because ES096 2.8.5 physical preconditions are not
  observable by this Driver.

`i2c_error_t` is the primary typed error selected from none, address NACK,
data NACK, arbitration lost, bus error, overrun/underrun, hardware timeout,
software timeout, stuck BUSY, and recovery failed.

## Configuration Model

`i2c_config_t` contains one coherent timing domain:

- `bus_speed_khz`: admitted presets `88`, `100`, and `400` (kHz);
- `fast_duty_ratio`: `2` (2:1) or `16` (16:9), meaningful only for `400` kHz;
- `disable_clock_stretching`: `0` (enable) or `1` (disable), allows master to
  prevent slave clock stretching.

The 88 kHz preset is the portable Standard-mode choice for repeated START on
affected silicon. The 100 kHz preset remains valid for ordinary transfers and
for boards that independently satisfy ES096, but the generic combined API
rejects it.

Configuration order:

1. Validate pointers, instance, selectors, and application-owned RCC gate.
2. Get stable `PCLK1`; require an integer MHz value in `2..36` for Standard
   mode or `4..36` for Fast mode.
3. Require `PE=0`, no pending START/STOP, and `BUSY=0` before the first write.
4. Snapshot CR1, CR2, CCR, and TRISE exactly once.
5. Stage a master-I2C baseline (`SMBUS=0`, `ACK=1`, `POS=0`) and timing into
   local images, including clock stretching control from `disable_clock_stretching`.
   Preserve CR2 IRQ/DMA source fields because they are separate explicit domains;
   reject incompatible deferred CR1 state instead of silently preserving it.
6. Commit dirty CR2, CCR, TRISE, then CR1 while disabled.
7. Leave `PE=0`; the application explicitly enables operation afterward.

Let `PCLK1_Hz` be the live APB1 frequency, `FREQ_MHz = PCLK1_Hz / 1,000,000`,
and `SCL_Hz` the selected preset.

```text
CR2.FREQ = FREQ_MHz

Standard mode:
  CCR_exact = PCLK1_Hz / (2 * SCL_Hz)
  CCR = ceil(CCR_exact), clamped only by validation to 4..4095
  TRISE = FREQ_MHz + 1

Fast mode, fast_duty_ratio = 2 (Tlow/Thigh = 2:1):
  CCR_exact = PCLK1_Hz / (3 * SCL_Hz)
  CCR = ceil(CCR_exact), valid 4..4095
  TRISE = floor((FREQ_MHz * 300 ns) / 1000 ns) + 1

Fast mode, fast_duty_ratio = 16 (Tlow/Thigh = 16:9):
  CCR_exact = PCLK1_Hz / (25 * SCL_Hz)
  CCR = ceil(CCR_exact), valid 1..4095
  TRISE = floor((FREQ_MHz * 300 ns) / 1000 ns) + 1
```

Ceiling division prevents the programmed SCL from exceeding the requested
rate. Examples at 36 MHz: Standard 100 kHz gives CCR 180 and TRISE 37;
Standard 88 kHz gives CCR 205 and an actual rate about 87.8 kHz; Fast 400 kHz
2:1 gives CCR 30 and TRISE 11; Fast 400 kHz 16:9 requires 3.6 and therefore
rounds to CCR 4, yielding 360 kHz. A request whose rounded fields are illegal
fails before MMIO.

Own-address registers remain at reset/unowned values in the admitted
controller-only scope. IRQ gates, DMAEN/LAST, GPIO/AFIO, NVIC, and RCC gate
state are not members of `i2c_config_t`. `I2C_DeConfig` pulses the matching
APB1 reset through RCC and leaves clock/NVIC/GPIO state unchanged.

## Master Transaction Model

All `wait(flag)` steps mean: sample SR1 once, decode errors first, consume one
transaction poll, then test the required event. No step can loop without
budget. Error cleanup never calls an assertion-style early-return macro after
the first transaction mutation.

### Common START and address phase

1. Validate all input/state and confirm `SR2.BUSY=0` before a fresh message.
2. Stage/write CR1 `START=1`; do not write CR1 again until hardware clears it.
3. Wait for `SB=1`; timeout or BERR during START performs CR1.SWRST recovery
   per ES096 2.8.4, restores configuration, and reports the recovery result.
4. Read SR1 as the first half of EV5, then write one encoded address byte to DR.
5. Wait for `ADDR=1` or AF/error. AF before ADDR is address NACK.
6. Clear ADDR only by the ordered SR1 then SR2 reads. SR2 is never sampled
   speculatively after an SR1 read.

### Master write

1. Run the common START/address-write phase.
2. Write each byte only when TXE permits it; wait for BTF when software cannot
   meet EV8 timing and always after the final byte.
3. Treat AF after any data write as data NACK.
4. At EV8_2 (`TXE=1` and `BTF=1`), set STOP once.
5. Bound the wait for hardware to clear master/BUSY state, then publish success.

The completion test is `(SR1 & (TXE | BTF)) == (TXE | BTF)`, not a test that
passes when either bit is set.

### Master read: one byte

1. Before clearing ADDR, enter a bounded critical section that saves and masks
   processor interrupt delivery.
2. Clear ACK, keep POS clear, perform SR1->SR2 ADDR clear, and set STOP
   immediately while the ADDR stretch is released (EV6_3).
3. Restore the prior processor interrupt mask.
4. Wait for RXNE, read DR once, then restore ACK=1/POS=0 after the action bits
   permit another CR1 write.

### Master read: two bytes

1. Set POS=1 and ACK=1 before address reception starts.
2. After ADDR, perform SR1->SR2 clear and clear ACK.
3. Wait for BTF, proving DR and shift register contain the final two bytes.
4. Save/mask interrupts, set STOP, read DR for byte 1 immediately, read DR for
   byte 2, then restore the prior interrupt mask. This closes the ES096 2.8.2
   corruption window.
5. Restore ACK=1/POS=0.

### Master read: more than two bytes

1. Use ACK=1/POS=0, clear ADDR, and read ordinary bytes while more than three
   remain.
2. With three remaining, wait for BTF; clear ACK; save/mask interrupts; read
   N-2, set STOP, and read N-1 without interruption as required by RM0008
   method 2 and ES096 2.8.2.
3. Restore interrupt mask, wait for RXNE, read N, then restore ACK=1/POS=0.

### Combined write then repeated-START read

1. Reject zero write/read sizes and ES096-unsafe Standard mode above 88 kHz.
2. Perform address-write and transmit the entire write span.
3. Wait for final BTF; set START instead of STOP to request repeated START.
4. Wait for SB, write the same seven-bit address encoded with R/W=1, then use
   the receive tail selected by `readSize`.
5. Generate exactly one STOP in the receive close sequence.

### Failure and cleanup

- Before START: no transaction MMIO and no result publication.
- After START: capture the primary error before acknowledging it; attempt the
  hardware-legal STOP only when still master and arbitration was not lost.
- ARLO already releases the lines and returns the peripheral to slave state;
  do not force STOP.
- Clear selected BERR/ARLO/AF/OVR/TIMEOUT flags with a semantic W0C image, not
  RMW. Never clear unobserved errors accidentally.
- Restore ACK=1 and POS=0 only when START/STOP action rules permit CR1 access.
- If STOP/BUSY does not complete, report stuck BUSY and leave GPIO recovery to
  BSP/application. CR1.SWRST is attempted where ES096/RM0008 permits it.
- The final return status describes operation success/failure and preserves the exact cause, progress, and recovery outcome.

## Event, Error, Interrupt, and NVIC Model

### Separate domains

1. SR1/SR2 flags describe peripheral events and errors.
2. CR2.ITEVTEN/ITBUFEN/ITERREN gate peripheral requests.
3. Hardware Boolean logic asserts event or error request lines.
4. NVIC IRQs 31-34 independently gate and prioritize processor delivery.
5. A future interrupt Driver state machine interprets events in transaction
   context.
6. Driver performs the hardware-specific clear/consume sequence; NVIC pending
   clear does not acknowledge I2C.

### Request/clear table

| Source/flag | Peripheral request condition | Set event | Clear/consume action |
| --- | --- | --- | --- |
| SB | `ITEVTEN && SB` | START/repeated START generated | SR1 read then address write to DR |
| ADDR | `ITEVTEN && ADDR` | Address sent/matched | Confirm SR1.ADDR, then read SR2 |
| ADD10 | `ITEVTEN && ADD10` | First 10-bit header sent | SR1 read then second address byte to DR |
| STOPF | `ITEVTEN && STOPF` | STOP detected in slave mode | SR1 read then CR1 write |
| BTF | `ITEVTEN && BTF` | DR/shift pair full or empty with stretching | SR1 read then DR read/write; START/STOP in TX close also clears |
| RXNE | `ITEVTEN && ITBUFEN && RXNE` | Received byte copied to DR | DR read or PE clear |
| TXE | `ITEVTEN && ITBUFEN && TXE` | Transmit DR empty | DR write, START/STOP, or PE clear |
| BERR/ARLO/AF/OVR/PECERR/TIMEOUT/SMBALERT | `ITERREN && any selected error` | Named hardware error | W0C selected flag or PE clear; mode-specific recovery still required |

Because request logic is level-sensitive to a live enabled condition, enabling
a source while its flag is already set can request service immediately. An
uncleared source can repend/re-enter after handler return. CR2 source bits do
not enable NVIC delivery.

The initial blocking implementation keeps all three CR2 interrupt gates off or
preserves explicit application state and does not install handlers. Public IRQ
source/event APIs remain deferred until a complete state machine passes ES096
latency and concurrency gates.

## Error and Recovery Model

| Error | Detection/reporting | Required immediate action | Recovery authority |
| --- | --- | --- | --- |
| `BERR` | SR1; typed bus error | Abort; STOP if still legal master; W0C; SWRST if associated with failed START | Driver, except pin-level recovery |
| `ARLO` | SR1; typed arbitration lost | Do not STOP; hardware releases lines and clears MSL; W0C | Driver reports; retry belongs to application |
| `AF` during address | SR1; address NACK | STOP/repeated START as contract allows; W0C | Driver |
| `AF` after data | SR1; data NACK | Abort at byte boundary; STOP; W0C | Driver |
| `OVR` | SR1 | Abort; record partial transfer; W0C | Driver; slave-specific retransmission deferred |
| hardware `TIMEOUT` | SR1, SMBus-only | Hardware may send STOP; acknowledge and report | Deferred SMBus semantics; still never ignored |
| software timeout | poll budget exhausted | Attempt legal STOP/reset, publish exact stage/progress | Driver |
| stuck BUSY before START | SR2 remains busy through budget | No START; optional SWRST; report stuck BUSY | Driver reset, then BSP GPIO recovery if still stuck |
| invalid software state | live register/state mismatch | Return before first transaction write | Driver |
| aborted/partial TX/RX | result counters/error | One cleanup path; destination prefix contract | Driver reports; application decides retry/discard |

RCC reset is the canonical complete `I2C_DeConfig` action. CR1.SWRST is the
in-transaction peripheral recovery tool. Neither can guarantee release from
ES096 analog-filter lock. SCL/SDA recovery requires board route knowledge and
therefore cannot be hidden inside the generic Driver.

## DMA and Ring-Buffer Disposition

DMA is `DEFERRED`, not rejected. Hardware routes and CR2.DMAEN/LAST exist, and
ES096 recommends DMA for event latency. Admission requires a DMA peer API,
channel-sharing arbitration, DR endpoint width, direction/count lifecycle,
EOT/EOT_1 and LAST sequencing, ITBUFEN exclusion, error/abort cleanup, and
tests for N=1/N=2/N>2 receive completion. Existing `i2c_dma.*` placeholders
provide none of these guarantees and will be removed until such a contract is
approved.

Ring-buffer ownership is `REJECTED` from the I2C stack. A queue or device
driver may feed the generic transmit API, but I2C must not dequeue application
state directly into DR. Existing `i2c_rb.*` is comment-only, returns raw
Boolean status, polls without bounds, and has no failure-atomic dequeue rule.

## Current Deviation Ledger

| ID | Layer | Evidence | Current behavior | Required behavior | Risk | Planned resolution | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| I2C-DRV-001 | Driver | warning-clean syntax check | Implicit return type on empty `I2C_Config`; unused inputs | Declared `driver_status_t` implementation | Build failure | Wave 3 replacement | `PLANNED` |
| I2C-DRV-002 | Driver | `i2c.h`/`i2c.c` | No active public transfer API or usable configuration | Frozen lifecycle and transfer surface | No generic capability | Waves 3-6 | `PLANNED` |
| I2C-DRV-003 | Driver | legacy loops | Every wait is unbounded and ignores errors | Transaction-wide poll budget and error-first waits | Permanent hang | Waves 4-6 | `PLANNED` |
| I2C-DRV-004 | Driver | legacy read path | Read address written twice; one/two/multi-byte tails incorrect; no POS handling | RM0008 Figures 274-277 and ES096 sequences | Corrupt/missing data | Wave 5 | `PLANNED` |
| I2C-DRV-005 | Driver | legacy transmit tests | `(TXE | BTF)` mask is treated as any-bit rather than both-bit EV8_2 | Explicit required-bit equality | Premature STOP/data loss | Wave 4 | `PLANNED` |
| I2C-DRV-006 | Driver | `I2C_D2L_GetInstance` | Unvalidated public LUT index | Pointer identity plus validated switch topology | Out-of-bounds read | Wave 1 | `PLANNED` |
| I2C-DRV-007 | Driver | legacy helpers | Void/raw-byte/Boolean returns collapse errors and progress | `driver_status_t` plus typed result | Undiagnosable failure | Waves 1,4-6 | `PLANNED` |
| I2C-DRV-008 | Driver | legacy global `temp` | Shared mutable static transaction state | Function-local images and exclusive-owner contract | Cross-instance/task race | Wave 4 | `PLANNED` |
| I2C-DRV-009 | Driver | current config types | Claims slave, 10-bit dual address, general call, and mode support without implementation | Master-only admitted domain; deferred claims removed | False API promise | Wave 1 | `PLANNED` |
| I2C-DRV-010 | Driver | legacy config | Hidden RCC, AFIO, and GPIO mutation | Verify RCC; application owns gates and pins | Shared-resource corruption | Wave 3 | `PLANNED` |
| I2C-CODEC-001 | Codec | file inventory | No Codec exists; field staging lives in LL/legacy Driver | Pure timing/address/error/CR1 transformations | Layer inversion/duplication | Wave 2 | `PLANNED` |
| I2C-CODEC-002 | Codec | legacy timing code | Timing is commented out, truncates blindly, and lacks bounds/failure status | Ceiling math, legal limits, boundary tests | Overclock/invalid timing | Wave 2 | `PLANNED` |
| I2C-LL-001 | LL | `i2c_ll.h` | LL mutates RCC and owns protocol selectors/staging | Mechanical I2C register access only | Broken dependency boundary | Wave 1 | `PLANNED` |
| I2C-LL-002 | LL | all register families | Generic Set/Clear/Toggle RMW exists for CR1, DR, SR1, SR2 | Legal named directions; semantic action/data/W0C paths | Replayed actions/lost data/flags | Wave 1 | `PLANNED` |
| I2C-LL-003 | LL | function declarations | Four inline functions and many APIs use missing/incorrect types and old naming | Explicit `void`/`reg`, `LL_I2C_*`, one transfer | Build/style failure | Wave 1 | `PLANNED` |
| I2C-CORE-001 | Core | `I2C_TypeDef.SR2` | SR2 full/member views are `_IO` to serve old LL | Read-only qualifier matching hardware | Accidental writes compile | Wave 1 | `PLANNED` |
| I2C-CORE-002 | Core | OAR1 fields | Mandatory bit 14 has no raw position/mask | Core-owned named raw mask | Invalid own-address staging later | Wave 1 | `PLANNED` |
| I2C-IRQ-001 | IRQ | comment-only files | Only CR2 gate toggles; no request Boolean, event clear, state machine, or NVIC boundary | Complete model before asynchronous admission | IRQ storm/data loss | Deferred wave | `DEFERRED` |
| I2C-DMA-001 | DMA | placeholder files | No peer contract despite fixed shared DMA routes | Coordinated DMA/I2C transaction | Channel conflict/tail corruption | Deferred wave | `DEFERRED` |
| I2C-RB-001 | Ring buffer | commented source | I2C dequeues directly into DR with raw status/unbounded waits | Queue above generic transaction API | Lost queue data/error collapse | Cleanup wave removal | `PLANNED` |
| I2C-CMAKE-001 | Build | source resolver | I2C correctly depends on RCC, but glob compiles every placeholder and failing `i2c.c` | Only purposeful sources; new Codec auto-resolves; warning-clean target | Module cannot build | Waves 1-3/cleanup | `PLANNED` |
| I2C-DOC-001 | Documentation | prior inventory | No architecture page; stale wrong section numbers and timing prose | Living evidence/trace/gates document | Unsafe assumptions | Initial architecture resolved here; source Doxygen remains I2C-DOC-002 | `RESOLVED` |
| I2C-DOC-002 | Documentation | `i2c.h` | Fast duty prose reverses `Tlow/Thigh` relationships and overclaims pull-up guidance | RM0008-correct equations and scoped electrical ownership | Wrong timing design | Wave 1/2 docs | `PLANNED` |
| I2C-ERR-001 | Cross-layer | ES096 2.8 | No workaround is implemented or documented in source | Mandatory latency, reset, repeated-START, and BUSY policies | Silicon-specific corruption/hang | Waves 4-6 | `PLANNED` |
| I2C-CONS-001 | Consumer | repository search | SSD1306 calls only legacy I2C symbols while CMake declares dependency | No integration on this branch; future migration only after gates | Broken future consumer | External deferred work | `DEFERRED` |

Findings are never deleted. Resolution changes the status and adds commit/test
evidence.

## Decision Ledger

| Decision ID | Question | Alternatives | Decision | Rationale | Consequences | Status |
| --- | --- | --- | --- | --- | --- | --- |
| I2C-DEC-001 | Public address form | Shifted byte / 7-bit / 10-bit union | Unshifted 7-bit `0x08..0x77`; Codec adds R/W | This is stable protocol vocabulary and prevents double shifting. | Reserved and 10-bit addresses are rejected in the first scope. | `VERIFIED` |
| I2C-DEC-002 | Initial transaction engine | Blocking / IRQ / DMA | Bounded polling first | It is the smallest testable complete state machine. | IRQ and DMA require separate admission; polling is always bounded. | `VERIFIED` |
| I2C-DEC-003 | Public granularity | Raw steps / device-register helpers / whole messages | Transmit, receive, combined write-read | Whole messages own complete cleanup without device policy. | Raw START/STOP and device-register helpers leave the public surface. | `VERIFIED` |
| I2C-DEC-004 | Timeout ownership | Infinite / per-event / transaction-wide | Caller-supplied nonzero transaction poll budget | One budget creates a deterministic whole-call bound independent of a tick service. | Every hardware sample consumes the same budget. | `VERIFIED` |
| I2C-DEC-005 | Configuration identity | `i2c_t` enum / peripheral pointer | `I2C_TypeDef *` | It matches repository convention and avoids a public lookup table. | Driver validates exact I2C1/I2C2 pointer identity. | `VERIFIED` |
| I2C-DEC-006 | Clock/pin ownership | Hidden Driver mutation / external | RCC gate and GPIO/AFIO stay application-owned | Those resources are shared and board-dependent. | I2C validates the clock and never selects pins/remap. | `VERIFIED` |
| I2C-DEC-007 | Result reporting | Generic status only / raw SR1 / typed result | `driver_status_t` plus semantic result/progress | Common status propagation and protocol diagnosis are both required. | Callers receive exact cause and completed-byte counts without depending on raw flags. | `VERIFIED` |
| I2C-DEC-008 | 100 kHz repeated START | Assume board conditions / reject / remove 100 kHz | Keep 100 kHz ordinary transfers; reject generic combined operation above 88 kHz Standard mode | The ES096 physical preconditions cannot be observed by the generic Driver. | Portable repeated START uses <=88 kHz Standard or Fast mode. | `VERIFIED` |
| I2C-DEC-009 | Receive critical timing | Hope polling is fast / mask interrupts / own GPIO | Save/mask/restore interrupts only across minimal RM/ES096 windows | It closes the known preemption window without seizing pin routing. | Wave 5 must prove exact ordering and prior-mask restoration. | `PLANNED` implementation proof |
| I2C-DEC-010 | DMA | Keep placeholders / implement now / defer | Defer and remove placeholders | No named peer/consumer contract exists. | DMA can return only through a newly admitted wave. | `VERIFIED` |
| I2C-DEC-011 | Ring buffer | Peripheral submodule / generic queue above I2C | Remove from I2C | Buffer policy is not peripheral protocol. | A higher layer may queue whole public transactions. | `VERIFIED` |
| I2C-DEC-012 | Stuck BUSY recovery | Driver toggles pins / reset only / split authority | Driver reset/report; BSP/application pin recovery | Route knowledge is external and ES096 reset alone may fail. | The result distinguishes stuck BUSY and recovery failure. | `VERIFIED` |
| I2C-DEC-013 | Concurrency | Lock inside Driver / caller ownership | Explicit single owner per instance initially | No repository-wide lock service exists. | Same-instance task/ISR/DMA overlap is a contract violation. | `VERIFIED` |

## Driver -> Codec -> LL Traceability Matrix

| Public operation | Driver validation/order | Codec | LL/registers | Flags/errors | Status/result | Planned verification | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `I2C_DeConfig` | Instance, RCC gate; decode reset mask; pulse reset | N/A | RCC peer; no I2C MMIO required | N/A | success/invalid/state | peer-call trace | `CONTRACT-FROZEN` |
| `I2C_Config` | Full preflight; snapshot; all staging; dirty commit CR2/CCR/TRISE/CR1 | Extract/stage timing and baseline | read/write CR1, CR2, CCR, TRISE; read SR2 | BUSY, action state | success/null/invalid/state | timing boundaries, no-write-on-error, MMIO order | `CONTRACT-FROZEN` |
| `I2C_GetOperationState` | Validate instance/RCC; one CR1 snapshot | Extract PE | read CR1 | PE | OFF/ON/error | valid/error fake register | `CONTRACT-FROZEN` |
| `I2C_SetOperationState` | Validate state/config/BUSY; one CR1 image; stage/write | Stage PE | read/write CR1 | PE, BUSY, START/STOP | success/error | preservation/action checks | `CONTRACT-FROZEN` |
| `I2C_MasterTransmit` | Preflight; BUSY; START/address; TX loop; BTF; STOP; cleanup | Address byte, errors, CR1 actions, W0C ack | CR1, DR, SR1, SR2 | SB, ADDR, TXE, BTF; AF/BERR/ARLO/OVR/TIMEOUT | exact result/progress | full event traces and injected failures | `CONTRACT-FROZEN` |
| `I2C_MasterReceive` | Preflight; START/address; select N=1/2/>2 tail; critical sections; STOP; restore | Address, ACK/POS/action/error images | CR1, DR, SR1, SR2 | SB, ADDR, RXNE, BTF and all errors | exact result/partial RX | tail traces, interrupt-window harness, errata cases | `CONTRACT-FROZEN` |
| `I2C_MasterWriteRead` | TX without STOP; final BTF; repeated START; selected RX tail | Same reusable transforms | Same named LL paths | both phase events/errors | separate TX/RX progress | repeated-START traces at 88/100/400 kHz policies | `CONTRACT-FROZEN` |

No standalone Codec API is justified for a raw flag whose interpretation is
entirely local to one private grouped extractor. No LL access exists merely to
mirror the register map: OAR access is deferred, SR2 write is removed, and DR
has explicit data-port naming.

## Register Ownership and Access Matrix

| Register | Class | Accepted use | Required LL direction | Special rule |
| --- | --- | --- | --- | --- |
| CR1 | RW + hardware-cleared actions | Baseline, PE, ACK/POS, START/STOP/SWRST | read/write | No write while START/STOP active; stage safe full images |
| CR2 | RW | FREQ now; IRQ/DMA fields preserved/deferred | read/write | Config only while PE=0 for timing contract |
| OAR1/OAR2 | RW | None in controller-only scope | none initially/raw deferred | OAR1 bit14 must be one if later configured |
| DR | 8-bit payload in 32-bit data port | Address and data transfer | semantic read/write | No RMW; each access changes protocol state |
| SR1 | RO events + W0C errors | Event polling and selected error ack | read + semantic ack write | Event clears use ordered side effects, not generic writes |
| SR2 | RO + ADDR-clear side effect | BUSY/role snapshot and confirmed ADDR clear | read only | Read after SR1 only when intended |
| CCR | RW timing | Config | read/write | PE must be zero |
| TRISE | RW timing | Config | read/write | PE must be zero |

RM0008 requires halfword or word access. The planned LL uses one 32-bit `.REG`
transfer for every register, including DR, and narrows/positions the low byte in
software.

## Verification Matrix

| Contract/risk | Pure unit evidence | Fake-register/trace evidence | Target/hardware evidence | Status |
| --- | --- | --- | --- | --- |
| Instance/address/selector validation | all valid/invalid/boundary values | no MMIO on failure | N/A | `PLANNED` |
| Timing Codec | PCLK 2/4/36 MHz, 88/100/400 kHz, both duties, ceil/min/max/reserved | unrelated-bit preservation; all-or-nothing publication | scope-analyzer SCL where available | `PLANNED` |
| Core/LL layout | static offsets/size/qualifiers | exactly one access; forbidden directions absent | N/A | `PLANNED` |
| Configuration | Codec round trip | CR2->CCR->TRISE->CR1 order; PE/BUSY failures write nothing | verify bus rate | `PLANNED` |
| Transmit | address encoding/error extraction | EV5/EV6/EV8/EV8_2, NACK at address/data, timeout every stage | known target ACK/NACK | `PLANNED` |
| Receive N=1 | ACK/POS stages | EV6_3 order and interrupt-mask restore | 1-byte device read | `PLANNED` |
| Receive N=2 | ACK/POS stages | BTF/STOP/two DR reads uninterrupted | 2-byte read under IRQ load | `PLANNED` |
| Receive N>2 | ACK/POS stages | three-byte tail and partial timeout | lengths 3,4,large under IRQ load | `PLANNED` |
| Combined transfer | speed-policy validation | final TX BTF, repeated START, one STOP | <=88 kHz and Fast-mode device read | `PLANNED` |
| Recovery | error-to-result mapping | BERR/ARLO/AF/OVR/timeout injection; legal STOP/reset; stuck BUSY | bus fault/noise where safe | `PLANNED` |
| Interrupt model | source/event Boolean tests when admitted | event/error request and NVIC independence | deferred | `DEFERRED` |
| Build/docs | macro/function style scanners | warning-clean module/project, Doxygen, diff check | N/A | `PLANNED` |

### Executed evidence

| Evidence ID | Command/evidence | Result | Commit/date |
| --- | --- | --- | --- |
| I2C-VE-001 | Repository/source/manual/errata static audit recorded in this document | Pass for initial planning evidence | `b865ebad`, `2026-08-28` |
| I2C-VE-002 | Arm GCC `-std=gnu11 -Wall -Wextra -Werror -fsyntax-only BareMetal/Driver/I2C/Src/i2c.c` with Core/I2C/RCC includes | Fail as expected: five implicit return types and two unused parameters | `b865ebad`, `2026-08-28` |
| I2C-VE-003 | Repository-wide consumer and CMake dependency search | I2C depends on RCC; no project directly selects I2C; SSD1306 alone references legacy surface | `b865ebad`, `2026-08-28` |

Planned evidence must record exact commands, results, artifact, commit/date,
and reviewer. A passing build alone cannot close sequencing gates.

## Implementation Waves

Every wave updates this file in the same change. Driver contracts and traces
are frozen above; implementation may build the demanded foundation bottom-up.

| Wave | Accepted scope | Deviation IDs | Driver/Codec/LL/docs work | Entry condition | Exit gate | Status |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | Evidence and architecture | DOC-001 | This document only; no I2C source/header edits | Authoritative files and hardware sources available | Inventory, ledgers, contracts, traces, waves, verdict present | `COMPLETE` |
| 1 | Core, public vocabulary, legal LL | DRV-006/009, LL-001/002/003, CORE-001/002, DOC-002 | Add types/defines/config; correct Core qualifiers/masks; replace LL with demanded one-transfer surface | Wave 0 reviewed | compile/layout/access-direction/style tests pass | `PLANNED` |
| 2 | Pure Codec | CODEC-001/002 | Implement timing, baseline, operation, address, error extraction/W0C, action staging and pure tests | Wave 1 types/register contracts frozen | all selector/boundary/preservation/publication tests pass | `PLANNED` |
| 3 | Lifecycle/configuration | DRV-001/002/010, CMAKE-001 | Implement topology, RCC validation/reset, DeConfig/Config/operation pair; no transfer yet | Waves 1-2 pass | warning-clean build; no-write-on-error and MMIO order traces pass | `PLANNED` |
| 4 | Bounded transmit | DRV-003/005/007/008 | Implement common poll/error engine, START/address, transmit, STOP, progress/cleanup | Wave 3 configured-state contract passes | all EV/error/timeout traces pass | `PLANNED` |
| 5 | Receive and repeated START | DRV-004, ERR-001 | Implement N=1/N=2/N>2, critical windows, combined transfer, ES096 speed gate | Wave 4 stable; interrupt-mask primitive reviewed | tail/repeated-START/errata trace and target stress evidence pass | `PLANNED` |
| 6 | Recovery closure | DRV-003/007, ERR-001 | Complete SWRST/config restore, stuck-BUSY result, all error injection and partial-progress contracts | Waves 4-5 pass | every post-START failure reaches one proven cleanup state | `PLANNED` |
| 7 | Legacy cleanup/documentation | RB-001, DMA placeholder, IRQ placeholder, DOC findings | Remove dead legacy/placeholder files or content; full Doxygen pass; keep SSD1306 untouched | Accepted API consumers within scope build | zero unclassified I2C symbols; docs/style/build clean | `PLANNED` |
| 8 | Independent final audit | all nondeferred | Adversarial Driver->Codec->LL, errata, usability, and hardware review | Waves 1-7 evidence complete | every nondeferred completion gate verified | `PLANNED` |

IRQ, DMA, slave, 10-bit, SMBus, and device integration require new admitted
waves after Wave 8; they are not silently pulled into these waves.

## Completion Gates

The binding definitions below are copied unchanged from the canonical closure
gate registry. I2C-specific evidence belongs only in the status column.

| Gate ID | Binding gate | I2C status/evidence required |
| --- | --- | --- |
| S-01 | Concrete part, instances, capabilities, topology, reference manual, package data, errata, consumers, and non-goals are named and evidenced. | `VERIFIED` by this audit; reopen on target change |
| D-01 | Every in-scope intent has one admitted canonical Driver path, operation class, and audience; compatibility paths have migration plans. | `PLANNING-MET`; implementation evidence open |
| D-02 | Every public contract defines values, capability behavior, preconditions, resources/side effects, success/failure state, statuses, and concurrency. | `PLANNING-MET`; per-function Doxygen and tests open |
| D-03 | Group, scalar, action, data, and lifecycle granularity matches one coherent hardware transaction; no god config or split invariant exists. | `PLANNING-MET`; source audit open |
| D-04 | All validation, snapshots needed for validation, calculations, and fallible Codec staging finish before the first MMIO/peer mutation. | `PLANNED`; no-write-on-error and failure-injection traces |
| D-05 | Every post-mutation path has one guaranteed cleanup/release flow and documented rollback or irreversible failure state. | `PLANNED`; post-START cleanup traces |
| D-06 | Programmed/preload and active/effective state are distinguished; every update/commit accounts for the full preload, flag, TRGO, DMA, and synchronization domain. | `PLANNED`; timing and action-state traces |
| D-07 | Peripheral event flags, IRQ source enables, DMA source enables, and NVIC delivery state have separate types, semantics, and owners. | `PLANNING-MET`; asynchronous implementation deferred |
| D-08 | Single-owner, lock/critical-section, ISR, DMA, trigger, hardware-writer, and snapshot-coherence contracts are explicit. | `PLANNING-MET`; critical-window and stress evidence open |
| D-09 | RCC, reset, NVIC, GPIO/AFIO, DMA, cache, reference-count, lock, and shared-resource ownership use declared peer services without bypassing invariants. | `PLANNING-MET`; peer-call traces open |
| D-10 | Every private helper centralizes a reusable configuration-domain staging path, repeated policy, topology/capability, a hazardous commit sequence, calculation, or cleanup; trivial renaming and whole-public-function shadow helpers are rejected. | `PLANNED`; implementation diff review |
| D-11 | Application-facing APIs expose domain vocabulary only; any raw/diagnostic or typed peer endpoint has an explicit restricted audience. | `PLANNING-MET`; final API review open |
| C-01 | Every contract-frozen Driver transformation has exactly one Codec path or a written no-Codec reason; every exported Codec has an admitted consumer. | `PLANNING-MET`; implementation trace closure open |
| C-02 | Codec is pure: no peripheral pointer, volatile I/O, topology, sequencing, live-state policy, peer access, polling, or locking. | `PLANNING-MET`; static/source review open |
| C-03 | Persistent concepts have Extract/Stage symmetry; actions, acknowledgements, status, and consuming reads have deliberate asymmetry. | `PLANNING-MET`; API/test review open |
| C-04 | All validation precedes publication; failed grouped/multi-image operations change no caller storage and successful staging preserves unrelated bits. | `PLANNED`; pure failure tests |
| C-05 | Public domain values plus private caller-owned `reg` images form the Driver/Codec boundary; raw images never cross the application API. | `PLANNING-MET`; static review open |
| C-06 | Pure tests cover all selectors, invalid/reserved values, boundaries, lanes, preservation, atomic publication, and round trips; Doxygen/style pass. | `PLANNED`; retained Codec tests |
| L-01 | Every contract-frozen Driver observation/mutation has one legal named LL route and a register-demand trace row. | `PLANNING-MET`; implementation trace closure open |
| L-02 | Every extra complete-map accessor is classified as raw foundation, deferred, or removed; it creates no upper-layer scope. | `PLANNING-MET`; final symbol audit open |
| L-03 | Each LL function performs exactly one mechanical read, write, or address formation and contains no public validation, semantic translation, batching, or policy. | `PLANNED`; fake-MMIO tests |
| L-04 | RO/RW/WO/action/W0C/W1C/read-clear/FIFO/data/portal/shadow semantics match the manual, C qualifiers, names, and legal direction. | `PLANNING-MET`; Core/LL corrections and layout tests open |
| L-05 | W0C/W1C, action, read-clear, FIFO, data, portal, and hardware-modified state bypass unsafe generic RMW/dirty-write operations. | `PLANNING-MET`; static/trace tests open |
| L-06 | Buffered and mode-dependent registers have Driver-owned commit/mode contracts; LL never hides those side effects. | `PLANNING-MET`; source review open |
| L-07 | Layout/static checks and exactly-one-access fake-register tests pass; generic error handling never fabricates plausible data; Doxygen/style pass. | `PLANNED`; commands/artifacts open |
| X-01 | Every admitted public API has an end-to-end intent -> Driver helper -> Codec/no-Codec -> LL/peer -> resource -> status -> test trace. | `PLANNING-MET`; executable evidence open |
| X-02 | Every exported function and public semantic type/selector family is classified canonical, private, raw foundation, compatibility, deferred, misowned, or rejected. | `PLANNING-MET`; re-run after each wave |
| X-03 | Required warning-clean builds, pure tests, MMIO/peer traces, failure injection, hardware checks, examples, Doxygen, style, and `git diff --check` pass. | `PLANNED`; retained evidence open |
| X-04 | No unresolved question remains inside the selected implementation scope; deferred questions name an admission condition and owner. | `VERIFIED` for Wave 1; I2C-Q-001..003 are owned by later waves |
| X-05 | Independent evidence, architecture, and usability reviews are resolved; decisions record provenance, affected traces, and superseded guidance. | `PLANNED`; independent Wave 8 review |

### I2C-Specific Closure Objectives

These rows do not replace the canonical gates. They make the requested I2C
closure areas directly reviewable and map each one back to the binding IDs.

| Closure area | Objective exit condition | Binding gates | Status |
| --- | --- | --- | --- |
| Driver completeness | Every admitted lifecycle/operation/transfer API implements its frozen sequence, statuses, progress, concurrency, and cleanup contract. | D-01..D-11 | `PLANNED` |
| Codec completeness | Every demanded pure transform is symmetric or deliberately asymmetric, failure-atomic, preservation-tested, and has an admitted Driver consumer. | C-01..C-06 | `PLANNED` |
| LL completeness | Only demanded or classified legal directions remain; every accessor performs exactly one 32-bit transfer and special registers bypass generic RMW. | L-01..L-07 | `PLANNED` |
| Core register-map correctness | I2C layout/offsets, raw masks, OAR1 bit 14, access qualifiers, and register widths match RM0008 and pass static assertions. | S-01, L-04, L-07 | `PLANNED` |
| Error/recovery correctness | Every BERR/ARLO/AF/OVR/TIMEOUT/software-timeout/BUSY path records progress and reaches its specified STOP/reset/report state without hanging. | D-04..D-06, X-03 | `PLANNED` |
| Interrupt-model correctness | Event and error request Boolean logic, flag-clear actions, four NVIC lines, stale-live-source behavior, and ownership boundaries are correct; asynchronous service remains deferred. | D-07..D-09, X-01 | `PLANNING-MET` |
| Documentation completeness | Architecture ledgers match implementation; all admitted public/source contracts pass the repository Doxygen and comment conventions. | C-06, L-07, X-02, X-03 | `PLANNED` |
| Build/test cleanliness | Warning-clean module and consumer builds, pure tests, fake-MMIO traces, failure injection, Doxygen, style checks, and whitespace checks all pass with retained commands. | X-03 | `PLANNED` |
| Cross-layer traceability | Every admitted public API has one complete Driver -> Codec/no-Codec -> LL/peer -> register/resource -> status/result -> test row. | X-01, X-02 | `PLANNING-MET` |
| Independent final audit | Evidence, architecture/errata, and usability reviewers resolve all nondeferred findings and record provenance. | X-04, X-05 | `PLANNED` |

## Maintenance Rules

- Update this document during every implementation wave.
- Record a new deviation before or with code that differs from the plan.
- Retain resolved findings and attach evidence; never delete their history.
- Preserve superseded decisions with the reason and affected trace IDs.
- Do not create a separate I2C TODO that competes with these ledgers.
- Do not add this planning page to Doxygen until it is promoted into the
  repository's required two-part theory/implementation canonical guide.
- Do not begin SSD1306 integration or modify SSD1306 on this branch.
- Do not commit unless explicitly requested.

## Final Architecture Verdict

**READY FOR IMPLEMENTATION**

The initial Driver -> Codec -> LL audit is complete enough to start Wave 1.
This verdict does not approve the current implementation for use: it remains
nonfunctional, all non-document waves are open, and the stack is explicitly
not ready for device integration.
