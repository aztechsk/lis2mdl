/*
 * lis2mdl_stm_regs.c
 *
 * Autors: Jan Rusnak.
 * (c) 2025 AZTech.
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <stdio.h>
#include <gentyp.h>
#include "sysconf.h"
#include "board.h"
#include "fmalloc.h"
#include <mmio.h>
#include "msgconf.h"
#include "criterr.h"
#include "hwerr.h"
#include "cmdln.h"
#include "gpio_hal.h"
#include "lis2mdl.h"
#include "lis2mdl_stm_regs.h"

#if LIS2MDL_SENSOR_MODE == LIS2MDL_REGS_MODE

#define RESET_STM_TMO_MS 5000U
#if LIS2MDL_REPORT_FN == 1
#define LIS2MDL_REPORT_BUF_SIZE \
	sizeof("LIS2MDL si [999]: X=-2147483648 Y=-2147483648 Z=-2147483648 uT T=-2147483.648 C\n")
#endif

static TaskHandle_t tsk_hndl;
static struct lis2mdl_dsc lis2mdl_dsc;
static lis2mdl lis = &lis2mdl_dsc;
static p_stf_t stmf;
static TickType_t xLastWakeTime;
#if LIS2MDL_REPORT_FN == 1
static boolean_t (*report_clbk)(const char *);
static char *report_buf;
#endif

static struct {
	unsigned int get_data_cnt;
	unsigned int max_wait_cnt;
	unsigned int data_ovf_cnt;
	unsigned int data_tmo_cnt;
	unsigned int stm_rst_cnt;
#if LIS2MDL_REPORT_FN == 1
	unsigned int report_fn_err_cnt;
#endif
} stats;

static gfp_t state_sw_reset(void);
static gfp_t state_whoami_check(void) __attribute__((unused));
static gfp_t state_chip_config(void);
static gfp_t state_verify_chip_config(void);
static gfp_t state_measure_start(void);
static gfp_t state_measure_cycle(void);
static gfp_t state_measure(void);
static gfp_t state_wait_result(void);
static gfp_t state_get_data(void);
static gfp_t state_error(void);
static void tsk(void *p);
static void report_data(struct lis2mdl_data *mag_data, int16_t *temp);
static void cmd_liss(void);

/**
 * init_lis2mdl_stm_regs
 */
void init_lis2mdl_stm_regs(void)
{
	lis->bus = LIS2MDL_I2C_BUS;
	lis->i2c_bus = get_i2cbus_by_per_id(ID_TWI0);
	lis->i2c_addr = 0x1E;
	init_lis2mdl(lis);
#if LIS2MDL_REPORT_FN == 1
	if (NULL == (report_buf = pvPortMalloc(LIS2MDL_REPORT_BUF_SIZE))) {
		crit_err_exit(MALLOC_ERROR);
	}
#endif
	add_command_noargs("liss", cmd_liss);
        if (pdPASS != xTaskCreate(tsk, "LIS2MDL", LIS2MDL_TASK_STACK_SIZE, NULL,
                                  LIS2MDL_TASK_PRIO, &tsk_hndl)) {
                crit_err_exit(MALLOC_ERROR);
        }
}

#if LIS2MDL_REPORT_FN == 1
/**
 * lis2mdl_stm_set_report_fn
 */
void lis2mdl_stm_set_report_fn(boolean_t (*report_fn)(const char *))
{
	report_clbk = report_fn;
}
#endif

/**
 * state_sw_reset
 */
static gfp_t state_sw_reset(void)
{
	int ret;

	ret = lis2mdl_soft_reset(lis);
	if (ret) {
		msg(INF, "LIS2MDL: Soft reset error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	msg(INF, "LIS2MDL: Soft reset done (LIS2MDL_REGS_MODE)\n");
	return ((gfp_t) state_chip_config);
}

/**
 * state_whoami_check
 */
static gfp_t state_whoami_check(void)
{
	int ret;

	ret = lis2mdl_whoami_check(lis);
	if (ret) {
		msg(INF, "LIS2MDL: WHOAMI check error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	vTaskDelay(ms_to_os_ticks(50));
	return ((gfp_t) state_chip_config);
}

/**
 * state_chip_config
 */
static gfp_t state_chip_config(void)
{
	uint8_t r;
	int ret;

	r = LIS2MDL_CFG_A_COMP_TEMP_EN | LIS2MDL_CFG_A_MD(LIS2MDL_CFG_A_MD_IDLE);
	ret = lis2mdl_write_cfg_r(lis, LIS2MDL_CFG_R_A, r, LIS2MDL_WRITE_VERIFY_YES);
	if (ret) {
		msg(INF, "LIS2MDL: CFG_R_A write error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	r = 0;
	ret = lis2mdl_write_cfg_r(lis, LIS2MDL_CFG_R_B, r, LIS2MDL_WRITE_VERIFY_YES);
	if (ret) {
		msg(INF, "LIS2MDL: CFG_R_B write error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	r = LIS2MDL_CFG_C_BDU;
	ret = lis2mdl_write_cfg_r(lis, LIS2MDL_CFG_R_C, r, LIS2MDL_WRITE_VERIFY_YES);
	if (ret) {
		msg(INF, "LIS2MDL: CFG_R_C write error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	return ((gfp_t) state_verify_chip_config);
}

/**
 * state_verify_chip_config
 */
static gfp_t state_verify_chip_config(void)
{
	int ret;

	ret = lis2mdl_verify_cfg_rgs(lis);
	if (ret) {
		msg(INF, "LIS2MDL: verify_cfg_rgs() error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	return ((gfp_t) state_measure_start);
}

/**
 * state_measure_start
 */
static gfp_t state_measure_start(void)
{
	struct lis2mdl_data mag_data;
	int16_t temp;
	uint8_t stat;
	int ret;

	ret = lis2mdl_stat(lis, &stat);
	if (ret) {
		msg(INF, "LIS2MDL: stat() error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	ret = lis2mdl_read_mag(lis, &mag_data);
	if (ret) {
		msg(INF, "LIS2MDL: read_mag() error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	ret = lis2mdl_read_temp(lis, &temp);
	if (ret) {
		msg(INF, "LIS2MDL: read_temp() error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	xLastWakeTime = xTaskGetTickCount() - ms_to_os_ticks(LIS2MDL_MEASURE_PERIOD_MS);
	return ((gfp_t) state_measure_cycle);
}

/**
 * state_measure_cycle
 */
static gfp_t state_measure_cycle(void)
{
	int ret;
	uint8_t r;

	vTaskDelayUntil(&xLastWakeTime, ms_to_os_ticks(LIS2MDL_MEASURE_PERIOD_MS));
	r = (lis->cfg_r_a & ~LIS2MDL_CFG_A_MD_MSK) | LIS2MDL_CFG_A_MD(LIS2MDL_CFG_A_MD_SINGLE);
	ret = lis2mdl_write_cfg_r(lis, LIS2MDL_CFG_R_A, r, LIS2MDL_WRITE_VERIFY_NO);
	if (ret) {
		msg(INF, "LIS2MDL: CFG_R_A write error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	return ((gfp_t) state_measure);
}

/**
 * state_measure
 */
static gfp_t state_measure(void)
{
	vTaskDelay(ms_to_os_ticks(LIS2MDL_STATE_MEASURE_MS));
	return ((gfp_t) state_wait_result);
}

/**
 * state_wait_result
 */
static gfp_t state_wait_result(void)
{
	uint8_t stat;
	int ret;
	unsigned int cnt;
	TickType_t tmo, ts;

	cnt = 0;
	tmo = ms_to_os_ticks(LIS2MDL_WAITRES_TMO_MS);
	ts = xTaskGetTickCount();
	for (;;) {
		ret = lis2mdl_stat(lis, &stat);
		if (ret) {
			msg(INF, "LIS2MDL: stat() error (%s)\n", hwerr_str(ret));
			return ((gfp_t) state_error);
		}
#if LIS2MDL_DEBUG_STAT == 1
		msg(INF, "LIS2MDL: LIS2MDL_STATUS_REG=%02hhX\n", stat);
#endif
		if (stat & LIS2MDL_STAT_ZYXDA) {
			if (stat & (LIS2MDL_STAT_ZYXOV | LIS2MDL_STAT_ZOV | LIS2MDL_STAT_YOV | LIS2MDL_STAT_XOV)) {
				stats.data_ovf_cnt++;
				msg(INF, "LIS2MDL: unexpected data overflow\n");
				return ((gfp_t) state_error);
			}
			if (cnt > stats.max_wait_cnt) {
				stats.max_wait_cnt = cnt;
			}
			return ((gfp_t) state_get_data);
		}
		if ((xTaskGetTickCount() - ts) > tmo) {
			stats.data_tmo_cnt++;
			msg(INF, "LIS2MDL: wait DRDY timeout\n");
			return ((gfp_t) state_error);
		}
		vTaskDelay(ms_to_os_ticks(2));
		cnt++;
	}
}

/**
 * state_get_data
 */
static gfp_t state_get_data(void)
{
	int ret;
	uint8_t r, exp;
	struct lis2mdl_data mag_data;
	int16_t temp;

	ret = lis2mdl_read_cfg_r(lis, LIS2MDL_CFG_R_A, &r);
	if (ret) {
		msg(INF, "LIS2MDL: CFG_R_A read error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	exp = (lis->cfg_r_a & ~LIS2MDL_CFG_A_MD_MSK) | LIS2MDL_CFG_A_MD(LIS2MDL_CFG_A_MD_IDLE_DEF);
	if (r != exp) {
		msg(INF, "LIS2MDL: unexpected CFG_R_A after measurement\n");
		return ((gfp_t) state_error);
	}
	lis->cfg_r_a = r;
	ret = lis2mdl_read_mag(lis, &mag_data);
	if (ret) {
		msg(INF, "LIS2MDL: read_mag() error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	ret = lis2mdl_read_temp(lis, &temp);
	if (ret) {
		msg(INF, "LIS2MDL: read_temp() error (%s)\n", hwerr_str(ret));
		return ((gfp_t) state_error);
	}
	stats.get_data_cnt++;
	report_data(&mag_data, &temp);
	return ((gfp_t) state_measure_cycle);
}

/**
 * state_error
 */
static gfp_t state_error(void)
{
	msg(INF, "LIS2MDL: resetting state machine after error\n");
	vTaskDelay(ms_to_os_ticks(RESET_STM_TMO_MS));
	stats.stm_rst_cnt++;
	return ((gfp_t) state_sw_reset);
}

/**
 * tsk
 */
static void tsk(void *p)
{
	stmf = state_sw_reset;
	while (TRUE) {
		stmf = (p_stf_t) (*stmf)();
	}
}

/**
 * report_data
 */
static void report_data(struct lis2mdl_data *mag_data, int16_t *temp)
{
#if (LIS2MDL_REPORT_LSB == 1 || LIS2MDL_REPORT_MG == 1 || LIS2MDL_REPORT_SI == 1 || LIS2MDL_REPORT_FN == 1) && \
     LIS2MDL_REPORT_MSG_NUM == 1
	static unsigned int msg_num;
#endif
#if LIS2MDL_REPORT_LSB == 1
#if LIS2MDL_REPORT_MSG_NUM == 1
	msg(INF, "LIS2MDL raw [%03u]: x=%+06d y=%+06d z=%+06d t: %+06d\n",
	    msg_num, (int) mag_data->x_val, (int) mag_data->y_val, (int) mag_data->z_val, (int) *temp);
#else
	msg(INF, "LIS2MDL raw: x=%+06d y=%+06d z=%+06d t: %+06d\n",
	    (int) mag_data->x_val, (int) mag_data->y_val, (int) mag_data->z_val, (int) *temp);
#endif
#endif
#if LIS2MDL_REPORT_MG == 1
	float sens_mg = 1.5f; /* 1.5 mG/LSB */
	float x_mg = mag_data->x_val * sens_mg;
	float y_mg = mag_data->y_val * sens_mg;
	float z_mg = mag_data->z_val * sens_mg;
	float t_c  = 25.0f + (*temp / 8.0f); /* 8 LSB/C, 25C offset */
#if LIS2MDL_REPORT_MSG_NUM == 1
	msg(INF, "LIS2MDL mg [%03u]: % 8.2f % 8.2f % 8.2f mG % 6.2f degC\n",
	    msg_num, x_mg, y_mg, z_mg, t_c);
#else
	msg(INF, "LIS2MDL mg: % 8.2f % 8.2f % 8.2f mG % 6.2f degC\n",
	    x_mg, y_mg, z_mg, t_c);
#endif
#endif
#if LIS2MDL_REPORT_SI == 1 || LIS2MDL_REPORT_FN == 1
	enum {
		FIELD_BUF_M = 12,
		FIELD_BUF_T = 13
	};
	int32_t x_tmp, y_tmp, z_tmp;
	int32_t x_ut, y_ut, z_ut, t_mc;
	char xs[FIELD_BUF_M], ys[FIELD_BUF_M], zs[FIELD_BUF_M], ts[FIELD_BUF_T];

	x_tmp = (int32_t) mag_data->x_val * 15;
	y_tmp = (int32_t) mag_data->y_val * 15;
	z_tmp = (int32_t) mag_data->z_val * 15;
	if (x_tmp < 0) {
		x_ut = -((-x_tmp + 50) / 100);
	} else {
		x_ut = (x_tmp + 50) / 100;
	}
	if (y_tmp < 0) {
		y_ut = -((-y_tmp + 50) / 100);
	} else {
		y_ut = (y_tmp + 50) / 100;
	}
	if (z_tmp < 0) {
		z_ut = -((-z_tmp + 50) / 100);
	} else {
		z_ut = (z_tmp + 50) / 100;
	}
	t_mc = 25000 + (int32_t) *temp * 125;
	snprintf(xs, sizeof(xs), "%+ld", (long) x_ut);
	snprintf(ys, sizeof(ys), "%+ld", (long) y_ut);
	snprintf(zs, sizeof(zs), "%+ld", (long) z_ut);
	{
		char sign = '+';
		int32_t t_abs = t_mc;
		int32_t t_c0;
		int32_t t_frac;

		if (t_mc < 0) {
			sign = '-';
			t_abs = -t_mc;
		}
		t_c0 = t_abs / 1000;
		t_frac = t_abs % 1000;
		snprintf(ts, sizeof(ts), "%c%ld.%03ld", sign, (long) t_c0, (long) t_frac);
	}
#if LIS2MDL_REPORT_FN == 1
#if LIS2MDL_REPORT_MSG_NUM == 1
	snprintf(report_buf, LIS2MDL_REPORT_BUF_SIZE,
	         "LIS2MDL si [%03u]: X=%s Y=%s Z=%s uT T=%s C\n", msg_num, xs, ys, zs, ts);
#else
	snprintf(report_buf, LIS2MDL_REPORT_BUF_SIZE,
	         "LIS2MDL si: X=%s Y=%s Z=%s uT T=%s C\n", xs, ys, zs, ts);
#endif
#if LIS2MDL_REPORT_SI == 1
	msg(INF, "%s", report_buf);
#endif
	if (report_clbk && !report_clbk(report_buf)) {
		stats.report_fn_err_cnt++;
	}
#else
#if LIS2MDL_REPORT_MSG_NUM == 1
	msg(INF, "LIS2MDL si [%03u]: X=%s Y=%s Z=%s uT T=%s C\n", msg_num, xs, ys, zs, ts);
#else
	msg(INF, "LIS2MDL si: X=%s Y=%s Z=%s uT T=%s C\n", xs, ys, zs, ts);
#endif
#endif
#endif
#if LIS2MDL_REPORT_MSG_NUM == 1 && \
    (LIS2MDL_REPORT_LSB == 1 || LIS2MDL_REPORT_MG == 1 || LIS2MDL_REPORT_SI == 1 || LIS2MDL_REPORT_FN == 1)
	if (++msg_num > 999) {
		msg_num = 0;
	}
#endif
}

/**
 * cmd_liss
 */
static void cmd_liss(void)
{
	UBaseType_t pr;

	pr = uxTaskPriorityGet(NULL);
	vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
	msg(INF, cmd_accp);
	msg(INF, "LIS2MDL cnt: get_data=%u max_wait=%u data_ovf=%u data_tmo=%u\n",
	    stats.get_data_cnt, stats.max_wait_cnt, stats.data_ovf_cnt, stats.data_tmo_cnt);
#if LIS2MDL_REPORT_FN == 1
	msg(INF, "LIS2MDL cnt: stm_rst=%u report_fn_err=%u\n",
	    stats.stm_rst_cnt, stats.report_fn_err_cnt);
#else
	msg(INF, "LIS2MDL cnt: stm_rst=%u\n", stats.stm_rst_cnt);
#endif
	vTaskPrioritySet(NULL, pr);
}
#endif
