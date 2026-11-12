/*
 * lis2mdl_stm_regs.h
 *
 * Autors: Jan Rusnak.
 * (c) 2025 AZTech.
 */

/**
 * @file lis2mdl_stm_regs.h
 * @brief FreeRTOS LIS2MDL single-measurement state machine with status polling.
 *
 * Owns one sensor on I2C/TWI0 (7-bit address 0x1E), creates a periodic task,
 * registers the "liss" statistics command and optionally reports samples.
 * The implementation is compiled only for LIS2MDL_SENSOR_MODE == LIS2MDL_REGS_MODE.
 *
 * @par Sensor configuration
 * After soft reset and reboot, the task writes and verifies these registers:
 * - CFG_REG_A = 0x82: temperature compensation enabled, high-resolution
 *   conversion (LP = 0), ODR field = 0, initially idle (MD = 2).
 * - CFG_REG_B = 0x00 or 0x01: digital low-pass filter disabled or enabled
 *   by LIS2MDL_LPF; offset cancellation and other options disabled.
 * - CFG_REG_C = 0x10: block data update (BDU) enabled; DRDY pin output,
 *   self-test and 4-wire SPI disabled.
 * The common soft-reset function programs and verifies hard-iron offset
 * registers when LIS2MDL_HARDIRON_EN is 1. The ODR field does not set
 * the rate of software-triggered single measurements.
 * CFG_REG_A/C are fixed in the .c file; the CFG_REG_B.LPF bit is
 * selected by LIS2MDL_LPF in sysconf.h.
 *
 * @par Acquisition and error handling
 * The task reads and discards initial status, field and temperature values.
 * At each scheduled measurement it sets CFG_REG_A to 0x81 (single mode)
 * and waits LIS2MDL_STATE_MEASURE_MS before polling STATUS_REG for ZYXDA
 * at 2 ms intervals.
 * The polling timeout starts after the initial wait. Data-overrun status
 * (ZYXOV/XOV/YOV/ZOV), transfer errors or timeout cause a state-machine
 * restart after a fixed 5000 ms delay. On success it verifies the automatic
 * transition to CFG_REG_A = 0x83 (idle), reads X/Y/Z and temperature, and
 * optionally reports them. The "liss" command prints acquisition statistics.
 *
 * @par Build-time settings in sysconf.h
 * - LIS2MDL_SENSOR_MODE: select LIS2MDL_REGS_MODE to build this module;
 *   LIS2MDL_DISABLED_MODE disables all three state machines.
 * - LIS2MDL_I2C / LIS2MDL_SPI: select low-level transport support; this
 *   state machine always uses I2C/TWI0, not SPI.
 * - LIS2MDL_TASK_PRIO: FreeRTOS acquisition task priority.
 * - LIS2MDL_TASK_STACK_SIZE: FreeRTOS task stack depth (StackType_t units).
 * - LIS2MDL_MEASURE_PERIOD_MS: interval between measurement triggers in ms,
 *   scheduled with vTaskDelayUntil().
 * - LIS2MDL_LPF: 0 disables the sensor digital low-pass filter (default);
 *   1 enables it in CFG_REG_B for this state machine.
 * - LIS2MDL_HARDIRON_EN: 0 skips hard-iron offset programming;
 *   1 writes and reads back LIS2MDL_HARDIRON_X/Y/Z on each reset.
 * - LIS2MDL_HARDIRON_X/Y/Z: signed 16-bit offset values (LSB),
 *   required when LIS2MDL_HARDIRON_EN is 1.
 * - LIS2MDL_STATE_MEASURE_MS: initial delay after a single-mode trigger in ms.
 * - LIS2MDL_WAITRES_TMO_MS: status-polling timeout in ms, counted after
 *   the initial delay; it is not a total measurement timeout.
 * - LIS2MDL_DEBUG_STAT: log raw STATUS_REG values while polling when set to 1.
 * - LIS2MDL_REPORT_LSB: log raw X/Y/Z and raw temperature values when set to 1.
 * - LIS2MDL_REPORT_MG: log X/Y/Z in mG and temperature in degC when set to 1.
 * - LIS2MDL_REPORT_SI: log X/Y/Z in uT with two decimal places and
 *   temperature in degC with three decimal places when set to 1.
 * - LIS2MDL_REPORT_SAMPLE_FN: enable the binary sample callback when 1
 *   (default 0); independent of text reporting.
 * - LIS2MDL_REPORT_FN: enable the SI text-report callback API when set to 1;
 *   an SI report is formatted even if LIS2MDL_REPORT_SI is disabled.
 * - LIS2MDL_REPORT_MSG_NUM: include a rolling 000..999 sample number in
 *   enabled reports (including callback text) when set to 1.
 * LIS2MDL_WAIT_ISR_SIG_TMO_MS is used only by the interrupt state machine.
 * Reporting switches do not enable acquisition or alter sensor registers.
 *
 * @note Include sysconf.h before this header to define LIS2MDL_REPORT_FN
 *       and optionally LIS2MDL_REPORT_SAMPLE_FN (default 0).
 */

#ifndef LIS2MDL_STM_REGS_H
#define LIS2MDL_STM_REGS_H

#ifndef LIS2MDL_REPORT_FN
#error "LIS2MDL_REPORT_FN not defined; include sysconf.h before lis2mdl_stm_regs.h"
#endif

#ifndef LIS2MDL_REPORT_SAMPLE_FN
#define LIS2MDL_REPORT_SAMPLE_FN 0
#endif
#if LIS2MDL_REPORT_SAMPLE_FN != 0 && LIS2MDL_REPORT_SAMPLE_FN != 1
#error "LIS2MDL_REPORT_SAMPLE_FN must be 0 or 1"
#endif

#if LIS2MDL_REPORT_FN == 1
#include <gentyp.h>
#endif
#if LIS2MDL_REPORT_SAMPLE_FN == 1
#include "lis2mdl.h"
#endif

/**
 * @brief Initialize and start the register-polling measurement state machine.
 *
 * Uses the initialized TWI0 bus at address 0x1E, registers the "liss"
 * diagnostic command and creates the LIS2MDL FreeRTOS task. The task resets
 * and configures the sensor, starts single measurements periodically, polls
 * STATUS_REG for ZYXDA, then reads and reports magnetic and temperature data.
 * On failure it waits 5 seconds before restarting its reset sequence.
 *
 * @pre TWI0 and required project services are initialized; FreeRTOS task
 *      and optional report-buffer allocation must be available.
 * @note Only defined when LIS2MDL_SENSOR_MODE == LIS2MDL_REGS_MODE.
 * @note Intended as one-time initialization during system startup.
 * @note Allocation or task-creation failure invokes crit_err_exit().
 */
void init_lis2mdl_stm_regs(void);

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

#if LIS2MDL_REPORT_SAMPLE_FN == 1
/**
 * @brief Install or clear the binary measurement callback.
 *
 * After each successful magnetic and temperature read, the acquisition
 * FreeRTOS task calls sample_fn with signed register outputs and converted
 * nT/mC values (see struct lis2mdl_sample). No callback is made on a failed
 * acquisition. The callback is independent of text reporting and logging.
 * Returning FALSE increments sample_fn_err_cnt without resetting acquisition.
 *
 * @param[in] sample_fn Callback to install, or NULL to disable callbacks.
 * @note Called synchronously in task context, never in an ISR. Keep work
 *       bounded and nonblocking, for example xQueueSend(..., 0).
 * @note The pointer and sample storage are valid only during the callback;
 *       copy the data if it must outlive the call. No queue, timestamp or
 *       lossless delivery guarantee is provided by this interface.
 * @note Register during initialization; replacing the callback while the
 *       acquisition task is running is not synchronized.
 * @note Only declared when LIS2MDL_REPORT_SAMPLE_FN == 1.
 */
void lis2mdl_stm_set_sample_fn(boolean_t (*sample_fn)(const struct lis2mdl_sample *));
#endif

#endif
