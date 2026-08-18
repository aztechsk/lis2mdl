/*
 * lis2mdl_stm_intr.h
 *
 * Autors: Jan Rusnak.
 * (c) 2026 AZTech.
 */

#ifndef LIS2MDL_STM_INTR_H
#define LIS2MDL_STM_INTR_H

/**
 * init_lis2mdl_stm_intr
 */
void init_lis2mdl_stm_intr(void);

#if LIS2MDL_REPORT_FN == 1
/**
 * lis2mdl_stm_set_report_fn
 */
void lis2mdl_stm_set_report_fn(boolean_t (*report_fn)(const char *));
#endif

#endif
