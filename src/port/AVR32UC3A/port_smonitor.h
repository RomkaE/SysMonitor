/*
 * Company:
 * Engineer:		Egoshin Roman
 * Create Date:		2015/07/10
 * Design Name:
 * Module Name:
 * Project Name:
 * Target Devices:
 * Tool versions:
 * Dependencies:
 * Description:
 *
 * Date Revision:
 * Engineer:
 * Description:
 *
 */

#ifndef PORT_SMONITOR_H_
#define PORT_SMONITOR_H_

/*============================ INCLUDES ======================================*/
#include <stdint.h>

#ifdef USE_SYS_MONITOR

/*============================ TYPES =========================================*/


/*============================ DEFINITIONS ===================================*/


/*============================ VARIABLES =====================================*/


/*============================ PROTOTYPES ====================================*/

void portSysMonitor_Init(void);

void portSysMonitor_TxBuff(uint16_t lenght);

void portSysMonitor_Init(void);

void portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS(void);

unsigned long portSysMonitor_GetRunTimeCounterValue(void);

#endif /* USE_SYS_MONITOR */
#endif /* PORT_SMONITOR_H_ */
