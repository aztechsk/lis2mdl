# LIS2MDL Driver

Embedded C driver for the STMicroelectronics LIS2MDL three-axis digital magnetometer.

The project consists of a low-level driver for direct sensor access and three optional FreeRTOS state-machine layers for periodic magnetic-field measurements.

The low-level driver supports I2C and SPI communication and provides access to sensor configuration, status information, magnetic-field data, temperature data and offset registers.

## Features

- I2C and SPI communication with the LIS2MDL
- Sensor reset and device identification check
- Register read and write access
- Configuration register handling and verification
- Reading X, Y and Z magnetic-field data
- Reading the internal temperature sensor
- Reading and writing magnetic offset registers
- Optional hard-iron offset programming and readback verification on sensor reset
- Sensor status access
- Optional FreeRTOS state machine using status-register polling
- Optional FreeRTOS state machine using the data-ready interrupt
- Optional continuous 10 Hz state machine using the data-ready interrupt
- Periodic single-measurement and autonomous continuous acquisition
- Optional formatted measurement reporting, text callback and binary sample callback

## Files

### `lis2mdl.c`

Low-level LIS2MDL driver implementation.

It provides bus access, device identification, configuration handling, magnetic-field and temperature reads, offset register access, status reading and sensor reset.

### `lis2mdl.h`

Public interface of the low-level driver.

It contains the driver descriptor, magnetic-field data structure, register definitions, configuration constants and public API declarations.

### `lis2mdl_stm_regs.c`

FreeRTOS state-machine layer using sensor status-register polling.

It initializes and configures the sensor, periodically starts single measurements, waits for measurement completion and processes the acquired magnetic-field and temperature data.

### `lis2mdl_stm_regs.h`

Public interface of the register-polling state-machine layer.

It provides initialization and, when enabled, text and binary sample callbacks.

### `lis2mdl_stm_intr.c`

FreeRTOS state-machine layer using the LIS2MDL data-ready interrupt.

It initializes and configures the sensor, periodically starts single measurements and processes completed measurements after the sensor signals data availability.

### `lis2mdl_stm_intr.h`

Public interface of the interrupt-driven state-machine layer.

It provides initialization and, when enabled, text and binary sample callbacks.

### `lis2mdl_stm_cont.c`

FreeRTOS state-machine layer using the LIS2MDL data-ready interrupt in continuous 10 Hz mode.

It configures the sensor once and processes measurements as they become available; it does not trigger each sample.
The low-pass filter and sensor offset cancellation are independently enabled through `LIS2MDL_LPF` and
`LIS2MDL_OFFSET_CANC` in the target project's `sysconf.h`. Each option is disabled at 0 and enabled at 1.
Offset cancellation is automatic in continuous mode and does not replace hard-iron compensation.

### `lis2mdl_stm_cont.h`

Public interface and configuration documentation for the continuous-mode state machine.

It provides initialization and, when enabled, text and binary sample callbacks.

## Dependencies

The low-level driver depends on:

- a project-specific I2C implementation and/or SPI HAL
- FreeRTOS timing primitives used by sensor reset handling
- project-specific common, timing and error-handling support

The state-machine layers additionally use FreeRTOS tasks and synchronization primitives together with project-specific GPIO, pinmux, command-line and diagnostic facilities.

These dependencies are intentionally kept outside this repository and must be provided by the target project.

## Using the acquisition state machines

The application selects exactly one state machine through `LIS2MDL_SENSOR_MODE` in
its own `sysconf.h` and calls the corresponding initialization function once at
startup. All three state machines own the LIS2MDL and an acquisition FreeRTOS
task. They use I2C/TWI0 at 7-bit address `0x1E` regardless of the low-level
driver's optional SPI support.

| Selected mode | Startup function | Measurement trigger | Completion detection |
| --- | --- | --- | --- |
| `LIS2MDL_REGS_MODE` | `init_lis2mdl_stm_regs()` | Task periodically starts one conversion | Poll `STATUS_REG.ZYXDA` |
| `LIS2MDL_INTR_MODE` | `init_lis2mdl_stm_intr()` | Task periodically starts one conversion | GPIO DRDY interrupt and semaphore |
| `LIS2MDL_CONT_MODE` | `init_lis2mdl_stm_cont()` | Sensor runs autonomously at 10 Hz | GPIO DRDY interrupt and semaphore |

The parent firmware must provide the selected mode constants, a working TWI0
bus, FreeRTOS and the required platform services. The interrupt-based modes
also require board definitions `LIS2MDL_INT_CONT` and `LIS2MDL_INT_PIN` and
an initialized GPIO/interrupt backend. The state-machine APIs and their
preconditions are documented in `src/lis2mdl_stm_*.h`.

### Register-polling single mode (`LIS2MDL_REGS_MODE`)

The task uses `vTaskDelayUntil()` with `LIS2MDL_MEASURE_PERIOD_MS` to schedule
single-measurement triggers (`MD=SINGLE`). After each trigger it waits
`LIS2MDL_STATE_MEASURE_MS`, then polls the sensor status every 2 ms until all
three axes are ready (`ZYXDA`). `LIS2MDL_WAITRES_TMO_MS` limits this polling
phase, **not** the whole conversion time. On success the task verifies that
the sensor returned to idle, reads X/Y/Z and temperature and produces the
configured report outputs, if reporting is enabled.

Choose this mode when the application wants a configurable task-driven sample
period without using the DRDY GPIO. The configured period is a trigger
schedule, not a guaranteed timestamp or a guarantee of one successful report
per period. This mode does not use automatic sensor offset cancellation.

### Interrupt-driven single mode (`LIS2MDL_INTR_MODE`)

The task also schedules single-measurement triggers with
`LIS2MDL_MEASURE_PERIOD_MS`, but completion is signaled by the sensor's
active-high DRDY GPIO and a FreeRTOS binary semaphore. After the signal the
task checks DRDY, `STATUS_REG.ZYXDA` and the absence of overrun flags, verifies
the transition to idle, then reads X/Y/Z and temperature. The wait for DRDY
is limited by `LIS2MDL_WAIT_ISR_SIG_TMO_MS`.

Choose this mode for a configurable task-driven period when a DRDY signal is
available and status polling is undesirable. The interrupt indicates a
completed conversion; it does not make the application trigger period more
precise. This mode does not use automatic sensor offset cancellation.

### Continuous mode (`LIS2MDL_CONT_MODE`)

The task switches the sensor to continuous mode once after initialization.
The LIS2MDL then performs conversions at its fixed 10 Hz output data rate
(nominally one conversion every 100 ms). The task waits for DRDY, validates
status and overrun flags, then reads X/Y/Z and temperature. It does not
schedule or explicitly start individual conversions. Its DRDY wait timeout
is fixed at 250 ms in the implementation, not configured by
`LIS2MDL_WAIT_ISR_SIG_TMO_MS`.

Choose this mode for autonomous sampling at the sensor's nominal 10 Hz rate.
`LIS2MDL_LPF` and `LIS2MDL_OFFSET_CANC` can be switched independently.
Offset cancellation changes the sensor's output values; it is distinct from
hard-iron compensation. Neither feature performs application-side calibration
or averaging. The nominal ODR does not imply guaranteed delivery of every
conversion to the application, nor a measured 100 ms interval between reports.

## Build-time configuration (`sysconf.h`)

The STM implementations use the following settings supplied by the parent
firmware; this repository does not supply a complete `sysconf.h`.

| Setting | Register polling | Single IRQ | Continuous | Meaning |
| --- | :---: | :---: | :---: | --- |
| `LIS2MDL_SENSOR_MODE` | yes | yes | yes | Select **one** of the three STM modes |
| `LIS2MDL_MEASURE_PERIOD_MS` | yes | yes | - | Interval between software-triggered conversions (ms) |
| `LIS2MDL_STATE_MEASURE_MS` | yes | - | - | Delay before status polling (ms) |
| `LIS2MDL_WAITRES_TMO_MS` | yes | - | - | Timeout after the initial delay, while polling (ms) |
| `LIS2MDL_WAIT_ISR_SIG_TMO_MS` | - | yes | - | Timeout waiting for DRDY semaphore (ms) |
| `LIS2MDL_LPF` | yes | yes | yes | `0` disables, `1` enables the sensor's digital low-pass filter |
| `LIS2MDL_OFFSET_CANC` | - | - | yes | `0` disables, `1` enables internal offset cancellation |
| `LIS2MDL_HARDIRON_EN` | yes | yes | yes | `0` disables, `1` enables programming of hard-iron offset registers on reset |
| `LIS2MDL_HARDIRON_X/Y/Z` | yes | yes | yes | Signed 16-bit offset values in sensor LSB, used when hard-iron is enabled |
| `LIS2MDL_TASK_PRIO` | yes | yes | yes | FreeRTOS acquisition-task priority |
| `LIS2MDL_TASK_STACK_SIZE` | yes | yes | yes | Acquisition task stack depth (FreeRTOS stack units) |

All three modes enable temperature compensation, high-resolution operation
and block data update (BDU). The continuous ODR is fixed at 10 Hz; none of
the `LIS2MDL_MEASURE_PERIOD_MS` or single-mode wait settings controls it.
The two interrupt modes use an active-high, level-triggered DRDY input.

When `LIS2MDL_HARDIRON_EN == 1`, the shared `lis2mdl_soft_reset()` writes and
reads back `LIS2MDL_HARDIRON_X/Y/Z` after each reset (including recovery).
The driver does not calculate these values. With hard-iron disabled, no
offset-register writes are performed by this feature. Hard-iron compensation
is available in all three modes and is independent of the continuous-mode
`LIS2MDL_OFFSET_CANC` setting.

## Obtaining measurements

Each successful STM measurement reads three signed 16-bit magnetic output
register values and one signed 16-bit temperature output. An application can
consume the results through optional diagnostic messages or a text callback:

| Setting (all modes) | Output |
| --- | --- |
| `LIS2MDL_REPORT_LSB` | X/Y/Z output-register values and raw temperature in LSB |
| `LIS2MDL_REPORT_MG` | X/Y/Z in mG and temperature in degC |
| `LIS2MDL_REPORT_SI` | X/Y/Z in uT (two decimals) and temperature in degC (three decimals) |
| `LIS2MDL_REPORT_FN` | Enable `lis2mdl_stm_set_report_fn()` with the same SI-format report text |
| `LIS2MDL_REPORT_SAMPLE_FN` | Enable `lis2mdl_stm_set_sample_fn()` with a binary `struct lis2mdl_sample` (default 0) |
| `LIS2MDL_REPORT_MSG_NUM` | Include a rolling report number `000`..`999` in enabled outputs |
| `LIS2MDL_DEBUG_STAT` | Log sensor status values during acquisition |

These switches control reporting; they do not start acquisition or change
the sensor configuration. Multiple logging formats can be enabled at once.
`LIS2MDL_REPORT_FN == 1` enables the callback even when SI console logging
is disabled. When any text reporting option is enabled, the report number
advances once per successful STM data read, not once per enabled text format.
It is not a timestamp or an independently verified sensor-conversion counter.

Nominal conversions used by the reporting code are **0.15 uT/LSB**
(**1.5 mG/LSB**) per magnetic axis and **T [degC] = 25 + raw/8**.
The LSB report gives the sensor's output-register values, which may already
include internal offset cancellation and/or configured hard-iron correction;
"raw" does not necessarily mean uncompensated field data.

When `LIS2MDL_REPORT_FN == 1`, the selected STM exposes:

```c
void lis2mdl_stm_set_report_fn(boolean_t (*report_fn)(const char *));
```

The callback receives a NUL-terminated SI report with a trailing newline.
It executes synchronously in the acquisition **task**, not in an ISR.
The STM owns and reuses the text buffer; copy the string within the callback
if it must be retained. Return `TRUE` on success; returning `FALSE` only
increments the `report_fn_err_cnt` diagnostic counter. To use this callback in continuous mode, include `sysconf.h` before
`lis2mdl_stm_cont.h`, call `init_lis2mdl_stm_cont()` during startup, and
register the callback with `lis2mdl_stm_set_report_fn()`. The two single
modes use their own initialization functions and the same callback API.

When `LIS2MDL_REPORT_SAMPLE_FN == 1`, the selected STM also exposes:

```c
void lis2mdl_stm_set_sample_fn(boolean_t (*sample_fn)(const struct lis2mdl_sample *));
```

The `struct lis2mdl_sample` type is declared in `lis2mdl.h`. It contains the
signed 16-bit magnetic output-register values in `raw.x_val/y_val/z_val`, the
signed temperature register value `temp_raw`, magnetic field `x_nt/y_nt/z_nt`
in integer nanotesla (150 nT/LSB) and `t_mc` in milli-degrees Celsius
(`25000 + temp_raw * 125`). The sensor output registers can already include
its internal offset cancellation and configured hardware hard-iron correction.
No further software calibration is performed by the STM. A sample is delivered
only after both magnetic and temperature reads succeed.

The binary callback is independent of all text reports. It runs synchronously
in the acquisition FreeRTOS task, not the ISR. The sample pointer points to
stack storage valid only until the callback returns, so the client must copy
it when retaining data. Register the callback during startup; replacement
while acquisition runs is not synchronized. `NULL` disables reporting. Return
`TRUE` for success; returning `FALSE` increments `sample_fn_err_cnt` without
resetting the state machine. Keep the callback bounded and nonblocking (for
example, `xQueueSend(..., 0)`). No extra task, queue or allocation is required.

The STM does not provide sample timestamps, a sample queue or guaranteed
lossless delivery. The binary sample also has no report sequence number.
Applications needing these facilities must implement them at the client level.

## Errors and diagnostics

All three modes reject bus/configuration failures and data-overrun flags;
the single and continuous interrupt modes also validate DRDY signaling.
An acquisition error or timeout stops normal reporting, waits approximately
5 seconds and attempts to reset and reconfigure the sensor. Hard-iron offsets
are reapplied during reset when enabled. There is no report callback for
failed measurements and no guaranteed reporting cadence during recovery.

The `liss` command prints mode-specific counters, including successful reads,
overruns, timeouts and state-machine resets; interrupt modes also track DRDY
anomalies. Text and binary callback failures have independent counters when enabled.
The report number advances on successful reads, not failed conversions;
errors and recovery pauses can occur without any sequence gap. It is not a
hardware sample counter or timing measurement. Applications requiring
continuity verification or latency measurements need external timestamps
and/or additional diagnostics.

## License

See the license information in the source files.

## Author

Jan Rusnak  
AZTech
