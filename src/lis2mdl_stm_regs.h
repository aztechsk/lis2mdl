/*
 * lis2mdl_stm_regs.h
 *
 * Autors: Jan Rusnak.
 * (c) 2025 AZTech.
 */

#ifndef LIS2MDL_STM_REGS_H
#define LIS2MDL_STM_REGS_H

/**
 * init_lis2mdl_stm_regs
 */
void init_lis2mdl_stm_regs(void);

#if LIS2MDL_REPORT_FN == 1
/**
 * lis2mdl_stm_set_report_fn
 */
void lis2mdl_stm_set_report_fn(boolean_t (*report_fn)(const char *));
#endif

#endif
