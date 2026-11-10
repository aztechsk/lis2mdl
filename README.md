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
- Sensor status access
- Optional FreeRTOS state machine using status-register polling
- Optional FreeRTOS state machine using the data-ready interrupt
- Optional continuous 10 Hz state machine using the data-ready interrupt
- Periodic single-measurement and autonomous continuous acquisition
- Optional formatted measurement reporting and report callback

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

It provides initialization and, when enabled, registration of a measurement report callback.

### `lis2mdl_stm_intr.c`

FreeRTOS state-machine layer using the LIS2MDL data-ready interrupt.

It initializes and configures the sensor, periodically starts single measurements and processes completed measurements after the sensor signals data availability.

### `lis2mdl_stm_intr.h`

Public interface of the interrupt-driven state-machine layer.

It provides initialization and, when enabled, registration of a measurement report callback.

### `lis2mdl_stm_cont.c`

FreeRTOS state-machine layer using the LIS2MDL data-ready interrupt in continuous 10 Hz mode.

It configures the sensor once and processes measurements as they become available; it does not trigger each sample.
The low-pass filter and sensor offset cancellation are independently enabled through `LIS2MDL_LPF` and
`LIS2MDL_OFFSET_CANC` in the target project's `sysconf.h`. Each option is disabled at 0 and enabled at 1.
Offset cancellation is automatic in continuous mode and does not replace hard-iron compensation.

### `lis2mdl_stm_cont.h`

Public interface and configuration documentation for the continuous-mode state machine.

It provides initialization and, when enabled, registration of a measurement report callback.

## Dependencies

The low-level driver depends on:

- a project-specific I2C implementation and/or SPI HAL
- FreeRTOS timing primitives used by sensor reset handling
- project-specific common, timing and error-handling support

The state-machine layers additionally use FreeRTOS tasks and synchronization primitives together with project-specific GPIO, pinmux, command-line and diagnostic facilities.

These dependencies are intentionally kept outside this repository and must be provided by the target project.

## Usage

An application may use the low-level driver directly when it needs explicit control over sensor configuration and data acquisition.

For periodic measurements, one of the supplied state-machine layers can be selected according to the application requirements. The register-polling and interrupt-driven single-measurement variants trigger samples at a task-defined interval. The continuous variant uses the sensor's own 10 Hz ODR and a data-ready interrupt; it requires `LIS2MDL_CONT_MODE` to be selected in the target project's `sysconf.h` and a call to `init_lis2mdl_stm_cont()` during startup.

Bus selection, sensor timing, GPIO assignment and optional reporting are target-specific and are expected to be configured by the surrounding project.

## License

See the license information in the source files.

## Author

Jan Rusnak  
AZTech
