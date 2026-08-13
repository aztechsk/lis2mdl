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

#ifndef LIS2MDL_H
#define LIS2MDL_H

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

enum lis2mdl_cfg_a_odr {
	LIS2MDL_CFG_A_ODR_10HZ,
	LIS2MDL_CFG_A_ODR_20HZ,
	LIS2MDL_CFG_A_ODR_50HZ,
	LIS2MDL_CFG_A_ODR_100HZ
};

#define LIS2MDL_CFG_A_MD_POS 0
#define LIS2MDL_CFG_A_MD_MSK (0x03U << LIS2MDL_CFG_A_MD_POS)
#define LIS2MDL_CFG_A_MD(value) (LIS2MDL_CFG_A_MD_MSK & ((value) << LIS2MDL_CFG_A_MD_POS))

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

struct lis2mdl_data {
	int16_t x_val;
	int16_t y_val;
	int16_t z_val;
};

enum lis2mdl_cfg_r {
	LIS2MDL_CFG_R_A,
	LIS2MDL_CFG_R_B,
	LIS2MDL_CFG_R_C
};

enum lis2mdl_verify_write {
	LIS2MDL_WRITE_VERIFY_NO,
	LIS2MDL_WRITE_VERIFY_YES
};

typedef struct lis2mdl_dsc *lis2mdl;

enum lis2mdl_bus {
	LIS2MDL_SPI_BUS,
	LIS2MDL_I2C_BUS
};

struct lis2mdl_dsc {
	enum lis2mdl_bus bus;
#if LIS2MDL_SPI == 1
	struct spi_hal_dev spi;
#endif
#if LIS2MDL_I2C == 1
	i2cbus i2c_bus;
	int i2c_addr;
#endif
	uint8_t cfg_r_a;
	uint8_t cfg_r_b;
	uint8_t cfg_r_c;
};

/**
 * init_lis2mdl
 */
void init_lis2mdl(lis2mdl lis);

/**
 * lis2mdl_reg_read
 */
int lis2mdl_reg_read(lis2mdl lis, enum lis2mdl_reg_adr adr, void *data, int n);

/**
 * lis2mdl_reg_write
 */
int lis2mdl_reg_write(lis2mdl lis, enum lis2mdl_reg_adr adr, void *data, int n, boolean_t ro_data);

/**
 * lis2mdl_whoami_check
 */
int lis2mdl_whoami_check(lis2mdl lis);

/**
 * lis2mdl_read_mag
 */
int lis2mdl_read_mag(lis2mdl lis, struct lis2mdl_data *data);

/**
 * lis2mdl_read_temp
 */
int lis2mdl_read_temp(lis2mdl lis, int16_t *temp);

/**
 * lis2mdl_read_cfg_r
 */
int lis2mdl_read_cfg_r(lis2mdl lis, enum lis2mdl_cfg_r r, uint8_t *v);

/**
 * lis2mdl_write_cfg_r
 */
int lis2mdl_write_cfg_r(lis2mdl lis, enum lis2mdl_cfg_r r, uint8_t v, enum lis2mdl_verify_write ver);

/**
 * lis2mdl_verify_cfg_rgs
 */
int lis2mdl_verify_cfg_rgs(lis2mdl lis);

/**
 * lis2mdl_read_offs_rgs
 */
int lis2mdl_read_offs_rgs(lis2mdl lis, struct lis2mdl_data *rgs);

/**
 * lis2mdl_write_offs_rgs
 */
int lis2mdl_write_offs_rgs(lis2mdl lis, struct lis2mdl_data *rgs);

/**
 * lis2mdl_stat
 */
int lis2mdl_stat(lis2mdl lis, uint8_t *stat);

/**
 * lis2mdl_soft_reset
 */
int lis2mdl_soft_reset(lis2mdl lis);

#endif
