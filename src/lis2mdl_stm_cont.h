/*
 * lis2mdl_stm_cont.h
 *
 * Autors: Jan Rusnak.
 * (c) 2026 AZTech.
 */

/**
 * @file lis2mdl_stm_cont.h
 * @brief FreeRTOS LIS2MDL continuous 10 Hz state machine using GPIO DRDY.
 *
 * Owns one sensor on I2C/TWI0 (7-bit address 0x1E), creates an acquisition
 * task and a binary semaphore, registers the "liss" statistics command and
 * optionally reports samples. The implementation is compiled only when
 * LIS2MDL_SENSOR_MODE == LIS2MDL_CONT_MODE.
 *
 * @par Sensor configuration
 * Following soft reset and reboot, the task writes and verifies:
 * - CFG_REG_A = 0x82: temperature compensation enabled, high-resolution
 *   mode (LP = 0), ODR = 10 Hz, initially idle (MD = 2).
 * - CFG_REG_B = 0x00..0x03: digital low-pass filter (bit 0) selected by
 *   LIS2MDL_LPF; sensor offset cancellation (bit 1) selected by
 *   LIS2MDL_OFFSET_CANC. Other bits, including OFF_CANC_ONE_SHOT, are 0.
 * - CFG_REG_C = 0x11: BDU and DRDY pin output enabled; BLE = 0, self-test
 *   and 4-wire SPI disabled.
 * After clearing stale data and arming DRDY, the task writes CFG_REG_A = 0x80
 * (continuous mode). The sensor generates samples at 10 Hz autonomously;
 * the task does not trigger individual conversions or return to idle.
 * Sensor offset cancellation is automatic in continuous mode and removes
 * intrinsic sensor offset; it is not hard-iron compensation. No additional
 * sample averaging is performed by the task. With offset cancellation, the
 * first data-ready event may take longer than one nominal 100 ms period.
 * Hard-iron offset registers are not programmed by this state machine.
 *
 * @par DRDY acquisition and recovery
 * LIS2MDL_INT_CONT/LIS2MDL_INT_PIN are board-defined. The pin is configured
 * as GPIO input without pull or filter, with Schmitt trigger and active-high
 * level interrupt. The ISR disables the interrupt and signals a binary
 * semaphore. The task verifies the pin level and STATUS_REG.ZYXDA, rejects
 * data-overrun flags, verifies continuous CFG_REG_A, and reads X/Y/Z and
 * temperature. Reading the magnetic output deasserts DRDY; the task then
 * re-enables the GPIO interrupt for the next sample. Reports and callbacks
 * run in task context. DRDY waits have a fixed 250 ms timeout, allowing
 * more than two nominal 100 ms sample periods. Errors and timeouts cause a
 * reset/reconfiguration attempt after a 5000 ms delay.
 *
 * @par Build-time settings in sysconf.h
 * - LIS2MDL_CONT_MODE: value 3, selected through LIS2MDL_SENSOR_MODE.
 * - LIS2MDL_I2C / LIS2MDL_SPI: select low-level transport support; this
 *   state machine always uses I2C/TWI0, not SPI.
 * - LIS2MDL_TASK_PRIO: FreeRTOS acquisition task priority.
 * - LIS2MDL_TASK_STACK_SIZE: FreeRTOS task stack depth (StackType_t units).
 * - LIS2MDL_LPF: 0 disables the digital low-pass filter; 1 enables it.
 * - LIS2MDL_OFFSET_CANC: 0 disables sensor offset cancellation; 1 enables
 *   automatic offset cancellation in continuous mode. Independent of LPF.
 * - LIS2MDL_DEBUG_STAT: log raw STATUS_REG after DRDY when set to 1.
 * - LIS2MDL_REPORT_LSB: log raw X/Y/Z and raw temperature when set to 1.
 * - LIS2MDL_REPORT_MG: log X/Y/Z in mG and temperature in degC when set to 1.
 * - LIS2MDL_REPORT_SI: log X/Y/Z in uT with two decimals and temperature
 *   in degC with three decimals when set to 1.
 * - LIS2MDL_REPORT_FN: enable the SI text callback when set to 1.
 * - LIS2MDL_REPORT_MSG_NUM: enable a rolling 000..999 report sequence number.
 * LIS2MDL_MEASURE_PERIOD_MS, LIS2MDL_STATE_MEASURE_MS,
 * LIS2MDL_WAITRES_TMO_MS and LIS2MDL_WAIT_ISR_SIG_TMO_MS apply only to
 * the single-measurement state machines and are not used here.
 * Reporting switches do not modify the sensor configuration.
 *
 * @note Include sysconf.h before this header to define LIS2MDL_REPORT_FN.
 */

#ifndef LIS2MDL_STM_CONT_H
#define LIS2MDL_STM_CONT_H

#ifndef LIS2MDL_REPORT_FN
#error "LIS2MDL_REPORT_FN not defined; include sysconf.h before lis2mdl_stm_cont.h"
#endif

#if LIS2MDL_REPORT_FN == 1
#include <gentyp.h>
#endif

/**
 * @brief Initialize and start the continuous 10 Hz acquisition state machine.
 *
 * Uses the initialized TWI0 bus at 7-bit address 0x1E, registers the "liss"
 * diagnostic command, creates a binary semaphore and a FreeRTOS task. The
 * task resets the sensor, configures active-high DRDY, and starts continuous
 * measurements. Each DRDY event results in one attempted data read and report.
 * On failure it disables the interrupt and retries initialization after 5 s.
 *
 * @pre TWI0 and the GPIO/interrupt backend are initialized and the board
 *      provides LIS2MDL_INT_CONT and LIS2MDL_INT_PIN; FreeRTOS resources
 *      and optional report-buffer allocation must be available.
 * @note Only defined when LIS2MDL_SENSOR_MODE == LIS2MDL_CONT_MODE.
 * @note Intended as one-time initialization during system startup.
 * @note Allocation or task-creation failure invokes crit_err_exit().
 */
void init_lis2mdl_stm_cont(void);

#if LIS2MDL_REPORT_FN == 1
/**
 * @brief Install or clear the measurement-report callback.
 *
 * After each successful sample, the task invokes the callback with a
 * NUL-terminated SI report (field in uT with two decimals, temperature
 * in degC with three decimals, ending with a newline). Returning FALSE
 * increments report_fn_err_cnt; TRUE indicates success.
 *
 * @param[in] report_fn Callback to install, or NULL to disable callbacks.
 * @note The callback runs synchronously in the acquisition task, not an ISR.
 *       The driver owns and reuses the buffer; copy the text before return
 *       if the callback needs to retain it.
 * @note Install/replace the callback outside concurrent report execution.
 * @note Only declared when LIS2MDL_REPORT_FN == 1.
 */
void lis2mdl_stm_set_report_fn(boolean_t (*report_fn)(const char *));
#endif

#endif
