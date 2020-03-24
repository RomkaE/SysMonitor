/*
 * Company:
 * Engineer:
 * Create Date:
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

/*============================ TYPES =========================================*/


/*============================ DEFINITIONS ===================================*/


/*============================ VARIABLES =====================================*/


/*============================ PROTOTYPES ====================================*/

void portSysMonitor_Init(void);

void portSysMonitor_TxBuff(uint16_t lenght);

void portSysMonitor_Init(void);

void portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS(void);

uint32_t portSysMonitor_GetRunTimeCounterValue(void);

#endif /* PORT_SMONITOR_H_ */
