/*
 * lis2mdl.c
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

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <gentyp.h>
#include <string.h>
#include "sysconf.h"
#include <mmio.h>
#include "msgconf.h"
#include "criterr.h"
#include "dlycnt.h"
#include "hwerr.h"
#include "lis2mdl.h"

#define LIS2MDL_WHO_AM_I_ID 0x40
#define SPI_HAL_SHORT_TRANS SPI_HAL_POLL
#define SPI_HAL_LONG_TRANS SPI_HAL_DMA
#define REBOOT_DELAY_MS 25

/**
 * init_lis2mdl
 */
void init_lis2mdl(lis2mdl lis)
{
#if LIS2MDL_SPI == 1
	if (lis->bus == LIS2MDL_SPI_BUS) {
		spi_hal_dev_init(&lis->spi);
	}
#endif
}

/**
 * lis2mdl_reg_read
 */
int lis2mdl_reg_read(lis2mdl lis, enum lis2mdl_reg_adr adr, void *data, int n)
{
#if LIS2MDL_SPI == 1 && LIS2MDL_I2C == 1
	if (lis->bus == LIS2MDL_SPI_BUS) {
#endif
#if LIS2MDL_SPI == 1
		enum spi_hal_xfer_type xt = (n > 1) ? SPI_HAL_LONG_TRANS : SPI_HAL_SHORT_TRANS;
		uint8_t hdr = 0x80 | (adr & 0x7F);
		uint8_t *rx = (uint8_t *) data;
		for (int i = 0; i < n; i++) {
			 rx[i] = 0xFF;
		}
		return (spi_hal_xfer(&lis->spi, xt, &hdr, 1, data, n));
#endif
#if LIS2MDL_SPI == 1 && LIS2MDL_I2C == 1
	} else {
#endif
#if LIS2MDL_I2C == 1
		return (i2c_read(lis->i2c_bus, I2C_MODE_7BIT_ADR_IADR1, lis->i2c_addr,
			data, n, (n > 1) ? TRUE : FALSE,
			(n > 1) ? adr | 0x80 : adr));
#endif
#if LIS2MDL_SPI == 1 && LIS2MDL_I2C == 1
	}
#endif
}

/**
 * lis2mdl_reg_write
 */
int lis2mdl_reg_write(lis2mdl lis, enum lis2mdl_reg_adr adr, void *data, int n, boolean_t ro_data)
{
#if LIS2MDL_SPI == 1 && LIS2MDL_I2C == 1
	if (lis->bus == LIS2MDL_SPI_BUS) {
#endif
#if LIS2MDL_SPI == 1
		enum spi_hal_xfer_type xt = (n > 1) ? SPI_HAL_LONG_TRANS : SPI_HAL_SHORT_TRANS;
		uint8_t hdr = (adr & 0x7F);
		if (ro_data) {
			uint8_t l_data[n];
			memcpy(l_data, data, n);
			return (spi_hal_xfer(&lis->spi, xt, &hdr, 1, l_data, n));
		} else {
			return (spi_hal_xfer(&lis->spi, xt, &hdr, 1, data, n));
		}
#endif
#if LIS2MDL_SPI == 1 && LIS2MDL_I2C == 1
	} else {
#endif
#if LIS2MDL_I2C == 1
		(void) ro_data;
		return (i2c_write(lis->i2c_bus, I2C_MODE_7BIT_ADR_IADR1, lis->i2c_addr,
			data, n, (n > 1) ? TRUE : FALSE,
			(n > 1) ? adr | 0x80 : adr));
#endif
#if LIS2MDL_SPI == 1 && LIS2MDL_I2C == 1
	}
#endif
}

/**
 * lis2mdl_whoami_check
 */
int lis2mdl_whoami_check(lis2mdl lis)
{
	int ret;
	uint8_t who;

	if ((ret = lis2mdl_reg_read(lis, LIS2MDL_WHO_AM_I_REG, &who, 1))) {
		return (ret);
	}
	return ((who == LIS2MDL_WHO_AM_I_ID) ? 0 : -EHW);
}

/**
 * lis2mdl_read_mag
 */
int lis2mdl_read_mag(lis2mdl lis, struct lis2mdl_data *data)
{
	int ret;
	uint8_t reg[6];

	if ((ret = lis2mdl_reg_read(lis, LIS2MDL_OUTX_L_REG, reg, 6))) {
		return (ret);
	}
	data->x_val = (uint16_t) reg[0] | ((uint16_t) reg[1] << 8);
	data->y_val = (uint16_t) reg[2] | ((uint16_t) reg[3] << 8);
	data->z_val = (uint16_t) reg[4] | ((uint16_t) reg[5] << 8);
	return (ret);
}

/**
 * lis2mdl_read_temp
 */
int lis2mdl_read_temp(lis2mdl lis, int16_t *temp)
{
	int ret;
	uint8_t reg[2];

	if ((ret = lis2mdl_reg_read(lis, LIS2MDL_TEMP_OUT_L_REG, reg, 2))) {
		return (ret);
	}
	*temp = (uint16_t) reg[0] | ((uint16_t) reg[1] << 8);
	return (ret);
}

/**
 * lis2mdl_read_cfg_r
 */
int lis2mdl_read_cfg_r(lis2mdl lis, enum lis2mdl_cfg_r r, uint8_t *v)
{
	enum lis2mdl_reg_adr rga = LIS2MDL_CFG_A_REG;

	switch (r) {
	case LIS2MDL_CFG_R_A :
		rga = LIS2MDL_CFG_A_REG;
		break;
	case LIS2MDL_CFG_R_B :
		rga = LIS2MDL_CFG_B_REG;
		break;
	case LIS2MDL_CFG_R_C :
		rga = LIS2MDL_CFG_C_REG;
		break;
	default :
		crit_err_exit(BAD_PARAMETER);
		break;
	}
	return (lis2mdl_reg_read(lis, rga, v, 1));
}

/**
 * lis2mdl_write_cfg_r
 */
int lis2mdl_write_cfg_r(lis2mdl lis, enum lis2mdl_cfg_r r, uint8_t v, enum lis2mdl_verify_write ver)
{
	int ret;
	enum lis2mdl_reg_adr rga = LIS2MDL_CFG_A_REG;
	uint8_t rb;

	switch (r) {
	case LIS2MDL_CFG_R_A :
		rga = LIS2MDL_CFG_A_REG;
		break;
	case LIS2MDL_CFG_R_B :
		rga = LIS2MDL_CFG_B_REG;
		break;
	case LIS2MDL_CFG_R_C :
		rga = LIS2MDL_CFG_C_REG;
		break;
	default :
		crit_err_exit(BAD_PARAMETER);
		break;
	}
	if ((ret = lis2mdl_reg_write(lis, rga, &v, 1, TRUE))) {
		return (ret);
	}
	if (ver) {
		if ((ret = lis2mdl_reg_read(lis, rga, &rb, 1))) {
			return (ret);
		}
		if (v != rb) {
			return (-EDATA);
		}
	}
	switch (r) {
	case LIS2MDL_CFG_R_A :
		lis->cfg_r_a = v;
		break;
	case LIS2MDL_CFG_R_B :
		lis->cfg_r_b = v;
		break;
	case LIS2MDL_CFG_R_C :
		lis->cfg_r_c = v;
		break;
	}
	return (0);
}

/**
 * lis2mdl_verify_cfg_rgs
 */
int lis2mdl_verify_cfg_rgs(lis2mdl lis)
{
	uint8_t ra, rb, rc;
	int ret;

	if ((ret = lis2mdl_reg_read(lis, LIS2MDL_CFG_A_REG, &ra, 1))) {
		return (ret);
	}
	if ((ret = lis2mdl_reg_read(lis, LIS2MDL_CFG_B_REG, &rb, 1))) {
		return (ret);
	}
	if ((ret = lis2mdl_reg_read(lis, LIS2MDL_CFG_C_REG, &rc, 1))) {
		return (ret);
	}
	if (ra != lis->cfg_r_a || rb != lis->cfg_r_b || rc != lis->cfg_r_c)  {
		return (-EDATA);
	}
	return (0);
}

/**
 * lis2mdl_read_offs_rgs
 */
int lis2mdl_read_offs_rgs(lis2mdl lis, struct lis2mdl_data *rgs)
{
	int ret;
	uint8_t reg[6];

	if ((ret = lis2mdl_reg_read(lis, LIS2MDL_OFFSX_L_REG, reg, 6))) {
		return (ret);
	}
	rgs->x_val = (uint16_t) reg[0] | ((uint16_t) reg[1] << 8);
	rgs->y_val = (uint16_t) reg[2] | ((uint16_t) reg[3] << 8);
	rgs->z_val = (uint16_t) reg[4] | ((uint16_t) reg[5] << 8);
	return (0);
}

/**
 * lis2mdl_write_offs_rgs
 */
int lis2mdl_write_offs_rgs(lis2mdl lis, struct lis2mdl_data *rgs)
{
	uint8_t reg[6];

	reg[0] = rgs->x_val;
	reg[1] = (uint16_t) rgs->x_val >> 8;
	reg[2] = rgs->y_val;
	reg[3] = (uint16_t) rgs->y_val >> 8;
	reg[4] = rgs->z_val;
	reg[5] = (uint16_t) rgs->z_val >> 8;
	return (lis2mdl_reg_write(lis, LIS2MDL_OFFSX_L_REG, reg, 6, FALSE));
}

/**
 * lis2mdl_stat
 */
int lis2mdl_stat(lis2mdl lis, uint8_t *stat)
{
	return (lis2mdl_reg_read(lis, LIS2MDL_STATUS_REG, stat, 1));
}

/**
 * lis2mdl_soft_reset
 */
int lis2mdl_soft_reset(lis2mdl lis)
{
	int ret;
	uint8_t r;

	r = LIS2MDL_CFG_A_SOFT_RST;
	if ((ret = lis2mdl_reg_write(lis, LIS2MDL_CFG_A_REG, &r, 1, FALSE))) {
		return (ret);
	}
	delay_us(20);
	if (lis->bus == LIS2MDL_SPI_BUS) {
		r = LIS2MDL_CFG_C_I2C_DIS | LIS2MDL_CFG_C_4WSPI;
	} else {
		r = 0;
	}
	if ((ret = lis2mdl_reg_write(lis, LIS2MDL_CFG_C_REG, &r, 1, FALSE))) {
		return (ret);
	}
	delay_us(50);
	if ((ret = lis2mdl_whoami_check(lis))) {
		return (ret);
	}
	r = LIS2MDL_CFG_A_REBOOT | LIS2MDL_CFG_A_MD(LIS2MDL_CFG_A_MD_IDLE_DEF);
	if ((ret = lis2mdl_reg_write(lis, LIS2MDL_CFG_A_REG, &r, 1, FALSE))) {
		return (ret);
	}
	vTaskDelay(ms_to_os_ticks(REBOOT_DELAY_MS));
	if (lis->bus == LIS2MDL_SPI_BUS) {
		r = LIS2MDL_CFG_C_I2C_DIS | LIS2MDL_CFG_C_4WSPI;
	} else {
		r = 0;
	}
	if ((ret = lis2mdl_reg_write(lis, LIS2MDL_CFG_C_REG, &r, 1, FALSE))) {
		return (ret);
	}
	delay_us(50);
	if ((ret = lis2mdl_whoami_check(lis))) {
		return (ret);
	}
	return (0);
}
