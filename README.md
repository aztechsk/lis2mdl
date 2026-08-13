# lis2mdl

Driver for the ST LIS2MDL 3-axis magnetometer.

The driver is intended for FreeRTOS-based embedded systems and supports both SPI and I2C communication. It provides low-level register access and functions for device identification, magnetic field and temperature acquisition, configuration handling and verification, offset register access, status reading, and device reset/reboot.

## Features

- SPI and I2C interface support
- WHO_AM_I device identification
- 3-axis magnetic field data acquisition
- Temperature data acquisition
- Configuration register access and verification
- Offset register read/write
- Status register access
- Software reset and device reboot
- FreeRTOS integration

## Files

- `lis2mdl.c` — driver implementation
- `lis2mdl.h` — public API, register definitions and data structures

## Author

Jan Rusnak  
Copyright (c) 2025 AZTech
