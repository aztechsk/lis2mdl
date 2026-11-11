/*
 * lis2mdl.h
 *
 * Copyright (c) 2025 Jan Rusnak <jan@rusnak.sk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file lis2mdl.h
 * @brief Public register-level interface for the LIS2MDL magnetometer.
 *
 * Defines sensor register fields, raw data types, a caller-owned device
 * descriptor and synchronous I2C/SPI operations. The driver does not start
 * periodic measurements; applications configure the sensor and trigger
 * acquisitions or use one of the FreeRTOS state-machine modules.
 *
 * @note Include sysconf.h first to define LIS2MDL_I2C and LIS2MDL_SPI.
 * @note Decoded magnetic and temperature reads support only the default
 *       byte order (CFG_REG_C.BLE = 0).
 */

#ifndef LIS2MDL_H
#define LIS2MDL_H

#include <stdint.h>
#include <gentyp.h>

#ifndef LIS2MDL_I2C
#error "LIS2MDL_I2C not defined; include sysconf.h before lis2mdl.h"
#endif
#ifndef LIS2MDL_SPI
#error "LIS2MDL_SPI not defined; include sysconf.h before lis2mdl.h"
#endif

#if LIS2MDL_I2C == 1
#include "i2c.h"
#endif
#if LIS2MDL_SPI == 1
#include "spi_hal.h"
#endif

// CFG_A
#define LIS2MDL_CFG_A_COMP_TEMP_EN (1 << 7)
#define LIS2MDL_CFG_A_REBOOT (1 << 6)
#define LIS2MDL_CFG_A_SOFT_RST (1 << 5)
#define LIS2MDL_CFG_A_LP (1 << 4)

#define LIS2MDL_CFG_A_ODR_POS 2
#define LIS2MDL_CFG_A_ODR_MSK (0x03U << LIS2MDL_CFG_A_ODR_POS)
#define LIS2MDL_CFG_A_ODR(value) (LIS2MDL_CFG_A_ODR_MSK & ((value) << LIS2MDL_CFG_A_ODR_POS))

/** @brief Output data rates for continuous measurement mode. */
enum lis2mdl_cfg_a_odr {
	LIS2MDL_CFG_A_ODR_10HZ,
	LIS2MDL_CFG_A_ODR_20HZ,
	LIS2MDL_CFG_A_ODR_50HZ,
	LIS2MDL_CFG_A_ODR_100HZ
};

#define LIS2MDL_CFG_A_MD_POS 0
#define LIS2MDL_CFG_A_MD_MSK (0x03U << LIS2MDL_CFG_A_MD_POS)
#define LIS2MDL_CFG_A_MD(value) (LIS2MDL_CFG_A_MD_MSK & ((value) << LIS2MDL_CFG_A_MD_POS))

/** @brief Measurement modes for CFG_REG_A. */
enum lis2mdl_cfg_a_md {
	LIS2MDL_CFG_A_MD_CONT,
	LIS2MDL_CFG_A_MD_SINGLE,
	LIS2MDL_CFG_A_MD_IDLE,
	LIS2MDL_CFG_A_MD_IDLE_DEF
};

// CFG_B
#define LIS2MDL_CFG_B_OFF_CANC_ONE_SHOT (1 << 4)
#define LIS2MDL_CFG_B_INT_ON_DATA_OFF (1 << 3)
#define LIS2MDL_CFG_B_SET_FREQ (1 << 2)
#define LIS2MDL_CFG_B_OFF_CANC (1 << 1)
#define LIS2MDL_CFG_B_LPF (1 << 0)

// CFG_C
#define LIS2MDL_CFG_C_INT_ON_PIN (1 << 6)
#define LIS2MDL_CFG_C_I2C_DIS (1 << 5)
#define LIS2MDL_CFG_C_BDU (1 << 4)
#define LIS2MDL_CFG_C_BLE (1 << 3)
#define LIS2MDL_CFG_C_4WSPI (1 << 2)
#define LIS2MDL_CFG_C_SELFTEST (1 << 1)
#define LIS2MDL_CFG_C_DRDY_ON_PIN (1 << 0)

// Status
#define LIS2MDL_STAT_ZYXOV (1 << 7)
#define LIS2MDL_STAT_ZOV (1 << 6)
#define LIS2MDL_STAT_YOV (1 << 5)
#define LIS2MDL_STAT_XOV (1 << 4)
#define LIS2MDL_STAT_ZYXDA (1 << 3)
#define LIS2MDL_STAT_ZDA (1 << 2)
#define LIS2MDL_STAT_YDA (1 << 1)
#define LIS2MDL_STAT_XDA (1 << 0)

/** @brief LIS2MDL register addresses used by the driver. */
enum lis2mdl_reg_adr {
	LIS2MDL_OFFSX_L_REG = 0x45,
	LIS2MDL_OFFSX_H_REG,
	LIS2MDL_OFFSY_L_REG,
	LIS2MDL_OFFSY_H_REG,
	LIS2MDL_OFFSZ_L_REG,
	LIS2MDL_OFFSZ_H_REG,
	LIS2MDL_WHO_AM_I_REG = 0x4F,
	LIS2MDL_CFG_A_REG = 0x60,
	LIS2MDL_CFG_B_REG,
	LIS2MDL_CFG_C_REG,
	LIS2MDL_STATUS_REG = 0x67,
	LIS2MDL_OUTX_L_REG,
	LIS2MDL_OUTX_H_REG,
	LIS2MDL_OUTY_L_REG,
	LIS2MDL_OUTY_H_REG,
	LIS2MDL_OUTZ_L_REG,
	LIS2MDL_OUTZ_H_REG,
	LIS2MDL_TEMP_OUT_L_REG,
	LIS2MDL_TEMP_OUT_H_REG
};

/**
 * @brief Three signed raw sensor values or offset-register values.
 *
 * Magnetic measurements are in sensor LSBs (nominally 1.5 mG/LSB).
 * The same structure is used for the signed X/Y/Z offset registers.
 */
struct lis2mdl_data {
	int16_t x_val; /**< X axis. */
	int16_t y_val; /**< Y axis. */
	int16_t z_val; /**< Z axis. */
};

/** @brief Configuration registers selectable through the configuration API. */
enum lis2mdl_cfg_r {
	LIS2MDL_CFG_R_A,
	LIS2MDL_CFG_R_B,
	LIS2MDL_CFG_R_C
};

/** @brief Enable or disable readback verification after a configuration write. */
enum lis2mdl_verify_write {
	LIS2MDL_WRITE_VERIFY_NO,
	LIS2MDL_WRITE_VERIFY_YES
};

/** @brief Pointer to a caller-owned LIS2MDL device descriptor. */
typedef struct lis2mdl_dsc *lis2mdl;

/** @brief Device bus interface used when both transports are enabled. */
enum lis2mdl_bus {
	LIS2MDL_SPI_BUS,
	LIS2MDL_I2C_BUS
};

/**
 * @brief Caller-owned descriptor and configuration shadow for one LIS2MDL.
 *
 * Select and initialize the bus-related fields before using the driver.
 * cfg_r_a/b/c hold the expected current values of CFG_REG_A/B/C, not a
 * continuously synchronized copy of the hardware registers. A successful
 * lis2mdl_write_cfg_r() updates the selected shadow field. The state
 * machines also update cfg_r_a after the sensor automatically returns to
 * idle mode following a single measurement.
 *
 * lis2mdl_soft_reset() leaves the shadow unchanged. The state machines
 * write a complete configuration before using or verifying it again.
 */
struct lis2mdl_dsc {
	enum lis2mdl_bus bus; /**< Selected transport (when both are enabled). */
#if LIS2MDL_SPI == 1
	struct spi_hal_dev spi; /**< SPI HAL descriptor, configured by the caller. */
#endif
#if LIS2MDL_I2C == 1
	i2cbus i2c_bus; /**< Initialized I2C bus handle. */
	int i2c_addr; /**< Seven-bit sensor address (normally 0x1E). */
#endif
	uint8_t cfg_r_a; /**< Expected current CFG_REG_A value. */
	uint8_t cfg_r_b; /**< Expected current CFG_REG_B value. */
	uint8_t cfg_r_c; /**< Expected current CFG_REG_C value. */
};

/**
 * @brief Initialize the selected SPI HAL device, when SPI is enabled.
 *
 * Calls spi_hal_dev_init() for an SPI device. For I2C, this function does
 * not initialize the bus or communicate with the sensor. Sensor reset and
 * register configuration are separate operations.
 *
 * @param[in,out] lis Caller-owned descriptor with bus and bus configuration set.
 * @pre The required bus has been configured by the caller.
 * @note Initialization is intended for startup/task context, not an ISR.
 */
void init_lis2mdl(lis2mdl lis);

/**
 * @brief Read one or more consecutive LIS2MDL registers.
 *
 * Performs a synchronous bus transfer. A multibyte read uses register
 * address auto-increment. The destination must be writable; SPI fills it
 * with dummy bytes before the full-duplex transfer.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[in] adr Address of the first register.
 * @param[out] data Buffer receiving n bytes.
 * @param[in] n Number of bytes to read; must be positive.
 * @return 0 on success or a negative bus error code.
 * @pre data references at least n writable bytes.
 * @note Task-context blocking call; synchronization is delegated to the bus.
 */
int lis2mdl_reg_read(lis2mdl lis, enum lis2mdl_reg_adr adr, void *data, int n);

/**
 * @brief Write one or more consecutive LIS2MDL registers.
 *
 * Performs a synchronous bus transfer. A multibyte write uses register
 * address auto-increment. On SPI, the full-duplex transfer may overwrite
 * data unless ro_data is TRUE; in that case the driver copies the data to
 * a temporary buffer. The I2C implementation ignores ro_data.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[in] adr Address of the first register.
 * @param[in,out] data Buffer containing n bytes (may be modified on SPI).
 * @param[in] n Number of bytes to write; must be positive.
 * @param[in] ro_data TRUE to preserve the caller's buffer on SPI.
 * @return 0 on success or a negative bus error code.
 * @pre data references at least n valid bytes.
 * @note Task-context blocking call; synchronization is delegated to the bus.
 */
int lis2mdl_reg_write(lis2mdl lis, enum lis2mdl_reg_adr adr, void *data, int n, boolean_t ro_data);

/**
 * @brief Check the sensor identification register.
 *
 * Reads WHO_AM_I and compares it with the expected value 0x40.
 *
 * @param[in] lis Initialized bus descriptor.
 * @return 0 when the identifier matches, -EHW when it does not, or a
 *         negative bus error code if the register read fails.
 */
int lis2mdl_whoami_check(lis2mdl lis);

/**
 * @brief Read the latest three-axis magnetic measurement.
 *
 * Reads six bytes starting at OUTX_L and decodes signed, little-endian
 * 16-bit X/Y/Z register values without conversion to field units. These
 * values may already include sensor-internal offset cancellation and
 * compensation using the programmed hard-iron offset registers.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[out] data Caller-owned structure receiving X, Y and Z output values.
 * @return 0 on success or a negative bus error code.
 * @pre CFG_REG_C.BLE = 0; BLE = 1 decoding is not supported.
 * @note This function does not wait for a new sample or check DRDY.
 */
int lis2mdl_read_mag(lis2mdl lis, struct lis2mdl_data *data);

/**
 * @brief Read the latest raw internal temperature measurement.
 *
 * Reads TEMP_OUT_L/H and decodes a signed 16-bit value. The reporting
 * layer uses T [degC] = 25 + raw/8 for temperature conversion.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[out] temp Caller-owned location receiving the raw temperature.
 * @return 0 on success or a negative bus error code.
 * @pre CFG_REG_C.BLE = 0; BLE = 1 decoding is not supported.
 * @note This function does not wait for a new temperature sample.
 */
int lis2mdl_read_temp(lis2mdl lis, int16_t *temp);

/**
 * @brief Read CFG_REG_A, CFG_REG_B or CFG_REG_C.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[in] r Configuration register selector.
 * @param[out] v Caller-owned location receiving the register byte.
 * @return 0 on success or a negative bus error code.
 * @note An invalid selector triggers crit_err_exit(BAD_PARAMETER).
 */
int lis2mdl_read_cfg_r(lis2mdl lis, enum lis2mdl_cfg_r r, uint8_t *v);

/**
 * @brief Write a configuration register and optionally verify it.
 *
 * Writes the selected register. With LIS2MDL_WRITE_VERIFY_YES, it reads
 * back the byte and compares it with v. After successful completion, it
 * updates the expected-value shadow field for the selected register.
 * Readback verification is suitable for stable configuration values, not
 * self-changing CFG_REG_A operations (SOFT_RST, REBOOT or MD=SINGLE).
 *
 * @param[in,out] lis Device descriptor holding configuration shadow values.
 * @param[in] r Configuration register selector (A, B or C).
 * @param[in] v Complete register byte to write.
 * @param[in] ver Select readback verification.
 * @return 0 on success, -EDATA on readback mismatch, or a negative bus
 *         error code on a transfer failure.
 * @note An invalid selector triggers crit_err_exit(BAD_PARAMETER).
 * @note On a failed write or readback the shadow remains unchanged; the
 *       hardware register may nevertheless have changed.
 * @note CFG_REG_C.BLE = 1 is not supported by decoded data reads.
 */
int lis2mdl_write_cfg_r(lis2mdl lis, enum lis2mdl_cfg_r r, uint8_t v, enum lis2mdl_verify_write ver);

/**
 * @brief Compare configuration registers with descriptor shadow values.
 *
 * Reads CFG_REG_A/B/C and compares their values with cfg_r_a/b/c stored
 * in the descriptor. The shadow must reflect the expected sensor state;
 * account for automatic single-to-idle transitions and reconfigure the
 * sensor after reset before calling this function.
 *
 * @param[in] lis Device descriptor with expected configuration bytes.
 * @return 0 when all bytes match, -EDATA on mismatch, or a negative bus
 *         error code on a transfer failure.
 */
int lis2mdl_verify_cfg_rgs(lis2mdl lis);

/**
 * @brief Read the signed X/Y/Z hard-iron offset registers.
 *
 * Reads six bytes from OFFSX_L through OFFSZ_H, combining each signed
 * 16-bit value in little-endian order without conversion to field units.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[out] rgs Caller-owned structure receiving the three offset values.
 * @return 0 on success or a negative bus error code.
 */
int lis2mdl_read_offs_rgs(lis2mdl lis, struct lis2mdl_data *rgs);

/**
 * @brief Write the signed X/Y/Z hard-iron offset registers.
 *
 * Serializes three signed 16-bit values in little-endian order and writes
 * the six consecutive offset register bytes. The input is not modified.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[in] rgs Caller-owned, read-only structure containing offsets.
 * @return 0 on success or a negative bus error code.
 */
int lis2mdl_write_offs_rgs(lis2mdl lis, const struct lis2mdl_data *rgs);

/**
 * @brief Read the raw STATUS_REG byte.
 *
 * The byte contains data-ready and data-overrun flags for the axes.
 * This function does not interpret or clear the status in software.
 *
 * @param[in] lis Initialized bus descriptor.
 * @param[out] stat Caller-owned location receiving STATUS_REG.
 * @return 0 on success or a negative bus error code.
 */
int lis2mdl_stat(lis2mdl lis, uint8_t *stat);

/**
 * @brief Soft-reset, reboot and identify the LIS2MDL.
 *
 * Triggers SOFT_RST, restores SPI interface selection when needed, checks
 * WHO_AM_I, then triggers REBOOT, waits 25 ms, restores the interface and
 * checks WHO_AM_I again. With LIS2MDL_HARDIRON_EN set to 1, it
 * then writes and reads back the configured signed X/Y/Z hard-iron
 * offsets. This is performed on every reset, including recovery.
 * It does not configure the measurement mode or update the configuration
 * shadow fields in the descriptor. Their previous contents must not be
 * treated as the reset sensor's current configuration.
 *
 * @param[in] lis Initialized bus descriptor.
 * @return 0 on success, -EHW on an unexpected identifier, -EDATA on
 *         hard-iron offset readback mismatch, or a negative bus error
 *         code on a transfer failure.
 * @note This function waits using both microsecond delays and FreeRTOS;
 *       it must run in task context, not in an ISR.
 * @note Reconfigure CFG_REG_A/B/C before using or verifying the shadow.
 */
int lis2mdl_soft_reset(lis2mdl lis);

#endif
