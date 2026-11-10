/*
 * lis2mdl_stm_intr.h
 *
 * Autors: Jan Rusnak.
 * (c) 2026 AZTech.
 */

/**
 * @file lis2mdl_stm_intr.h
 * @brief FreeRTOS LIS2MDL single-measurement state machine using GPIO DRDY.
 *
 * Owns one sensor on I2C/TWI0 (7-bit address 0x1E), creates a periodic task
 * and a binary semaphore, registers the "liss" statistics command and
 * optionally reports samples. The implementation is compiled only for
 * LIS2MDL_SENSOR_MODE == LIS2MDL_INTR_MODE.
 *
 * @par Sensor configuration
 * After soft reset and reboot, the task writes and verifies these registers:
 * - CFG_REG_A = 0x82: temperature compensation enabled, high-resolution
 *   conversion (LP = 0), ODR field = 0, initially idle (MD = 2).
 * - CFG_REG_B = 0x00 or 0x01: digital low-pass filter disabled or enabled
 *   by LIS2MDL_LPF; offset cancellation and other options disabled.
 * - CFG_REG_C = 0x11: block data update (BDU) and DRDY pin output enabled;
 *   self-test and 4-wire SPI disabled.
 * The state machine does not program hard-iron offset registers. The ODR
 * field does not set the rate of software-triggered single measurements.
 * CFG_REG_A/C are fixed in the .c file; the CFG_REG_B.LPF bit is
 * selected by LIS2MDL_LPF in sysconf.h.
 *
 * @par DRDY pin and acquisition
 * LIS2MDL_INT_CONT/LIS2MDL_INT_PIN are board-defined. The pin is configured
 * as GPIO input, without pull resistor or filter, with Schmitt trigger and
 * active-high level interrupt. The interrupt is enabled before each single
 * measurement (CFG_REG_A = 0x81). The ISR disables it and signals the task
 * through a binary semaphore; the task verifies pin HIGH, STATUS_REG ZYXDA
 * and absence of data-overrun flags (ZYXOV/XOV/YOV/ZOV). On success it checks
 * the automatic transition to CFG_REG_A = 0x83 (idle), reads X/Y/Z and
 * temperature, and optionally reports them. Sensor or transfer errors and
 * interrupt-wait timeouts restart the state machine after a fixed 5000 ms
 * delay. The "liss" command prints acquisition/interrupt statistics.
 *
 * @par Build-time settings in sysconf.h
 * - LIS2MDL_SENSOR_MODE: select LIS2MDL_INTR_MODE to build this module;
 *   LIS2MDL_DISABLED_MODE disables all three state machines.
 * - LIS2MDL_I2C / LIS2MDL_SPI: select low-level transport support; this
 *   state machine always uses I2C/TWI0, not SPI.
 * - LIS2MDL_TASK_PRIO: FreeRTOS acquisition task priority.
 * - LIS2MDL_TASK_STACK_SIZE: FreeRTOS task stack depth (StackType_t units).
 * - LIS2MDL_MEASURE_PERIOD_MS: interval between measurement triggers in ms,
 *   scheduled with vTaskDelayUntil().
 * - LIS2MDL_LPF: 0 disables the sensor digital low-pass filter (default);
 *   1 enables it in CFG_REG_B for this state machine.
 * - LIS2MDL_WAIT_ISR_SIG_TMO_MS: maximum semaphore wait for DRDY in ms.
 * - LIS2MDL_DEBUG_STAT: log raw STATUS_REG after DRDY when set to 1.
 * - LIS2MDL_REPORT_LSB: log raw X/Y/Z and raw temperature values when set to 1.
 * - LIS2MDL_REPORT_MG: log X/Y/Z in mG and temperature in degC when set to 1.
 * - LIS2MDL_REPORT_SI: log X/Y/Z in uT with two decimal places and
 *   temperature in degC with three decimal places when set to 1.
 * - LIS2MDL_REPORT_FN: enable the SI text-report callback API when set to 1;
 *   an SI report is formatted even if LIS2MDL_REPORT_SI is disabled.
 * - LIS2MDL_REPORT_MSG_NUM: include a rolling 000..999 sample number in
 *   enabled reports (including callback text) when set to 1.
 * LIS2MDL_STATE_MEASURE_MS and LIS2MDL_WAITRES_TMO_MS apply only to polling.
 * Reporting switches do not enable acquisition or alter sensor registers.
 *
 * @note Include sysconf.h before this header to define LIS2MDL_REPORT_FN.
 */

#ifndef LIS2MDL_STM_INTR_H
#define LIS2MDL_STM_INTR_H

#ifndef LIS2MDL_REPORT_FN
#error "LIS2MDL_REPORT_FN not defined; include sysconf.h before lis2mdl_stm_intr.h"
#endif

#if LIS2MDL_REPORT_FN == 1
#include <gentyp.h>
#endif

/**
 * @brief Initialize and start the interrupt-driven measurement state machine.
 *
 * Uses the initialized TWI0 bus at address 0x1E, registers the "liss"
 * diagnostic command, allocates a binary semaphore and creates the FreeRTOS
 * task. The task resets the sensor, configures the active-high DRDY GPIO
 * and sensor registers, then periodically starts single measurements. The
 * GPIO ISR signals the task, which reads and reports the measurement.
 * On failure it waits 5 seconds before restarting its reset sequence.
 *
 * @pre TWI0 and the GPIO/interrupt backend are initialized and the board
 *      provides LIS2MDL_INT_CONT and LIS2MDL_INT_PIN; FreeRTOS resources
 *      and optional report-buffer allocation must be available.
 * @note Only defined when LIS2MDL_SENSOR_MODE == LIS2MDL_INTR_MODE.
 * @note Intended as one-time initialization during system startup.
 * @note Allocation or task-creation failure invokes crit_err_exit().
 */
void init_lis2mdl_stm_intr(void);

#if LIS2MDL_REPORT_FN == 1
/**
 * @brief Install or clear the measurement-report callback.
 *
 * After a successful sample, the measurement task invokes the callback
 * with a NUL-terminated SI report (magnetic field in uT with two decimal
 * places, temperature in degC with three decimals, ending with a newline).
 * Returning FALSE increments report_fn_err_cnt; TRUE indicates success.
 *
 * @param[in] report_fn Callback to install, or NULL to disable callbacks.
 * @note The callback runs synchronously in the measurement task, not an ISR.
 *       The driver owns the text buffer and reuses it on later reports;
 *       copy the text within the callback if it must be retained.
 * @note Install/replace the callback outside concurrent report execution.
 * @note Only declared when LIS2MDL_REPORT_FN == 1.
 */
void lis2mdl_stm_set_report_fn(boolean_t (*report_fn)(const char *));
#endif

#endif
