/*
 * smonitor.c

 *
 *  Created on: 
 *      Author: 
 */
#ifdef USE_SYS_MONITOR

/*============================ INCLUDES ======================================*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board.h"

// FreeRTOS:
#include "FreeRTOS.h"
#include "task.h"

// Task:
#include "smonitor.h"
#include "port_smonitor.h"

/*============================ PRIVATE DEFINITIONS ===========================*/

typedef struct {
	uint32_t min;
	uint32_t aver;
	uint32_t max;
	uint32_t i;
	uint32_t percent;
} SMonitor_Cnt_t;

/*============================ TYPES =========================================*/


/*============================ VARIABLES =====================================*/
char SMBuff[SYS_MONITOR_BUFF_SIZE];
int WSize;

/*============================ PRIVATE PROTOTYPES ============================*/

void SysMonitor_Task(void *pvParameters);

uint16_t SysMonitor_TimeStats(char *pcWriteBuffer);

/*============================ IMPLEMENTATION (PRIVATE FUNCTIONS) ============*/
//signed char pcWriteBuffer[1024];

void SysMonitor_Task(void *pvParameters) {
portTickType xLastWakeTime;

	// Инициализация:
	xLastWakeTime = xTaskGetTickCount();

	while(1){

		//--
		WSize = sprintf( (char*)SMBuff, CLEARSCR );
		WSize += sprintf( &SMBuff[WSize], (const char*)GOTOYX, 0, 0 );
		WSize += SysMonitor_TimeStats( &SMBuff[WSize] );

		#if SYS_MONITOR_VIEW_FREE_HEAP == 1
			WSize += sprintf( &SMBuff[WSize], "FREE HEAP:\t%u\r\n",  xPortGetFreeHeapSize() );
		#endif

		#if SYS_MONITOR_VIEW_MIN_HEAP == 1
			WSize += sprintf( &SMBuff[WSize], "MIN HEAP:\t%u\r\n",  xPortGetMinimumEverFreeHeapSize() );
		#endif

		#if SYS_MONITOR_VIEW_BUFF == 1
			WSize += sprintf( &SMBuff[WSize], "FREE BUFF:\t%u\r\n", SYS_MONITOR_BUFF_SIZE - WSize );
		#endif

		//--
		if (WSize > SYS_MONITOR_BUFF_SIZE){
			WSize = SYS_MONITOR_BUFF_SIZE;
			SMBuff[SYS_MONITOR_BUFF_SIZE-1] = '!';
		}//

		portSysMonitor_TxBuff(WSize);
		vTaskDelayUntil(&xLastWakeTime, SYS_MONITOR_TIME_UPDATE);
	}//while(1)
}//SysMonitor_Task

uint16_t SysMonitor_TimeStats(char *pcWriteBuffer){
xTaskStatusType *pxTaskStatusArray;
volatile unsigned portBASE_TYPE uxArraySize;
uint32_t ulTotalTime;
unsigned portBASE_TYPE uxCurrentNumberOfTasks;
//--
unsigned portBASE_TYPE i, iTask, NTask;
uint16_t ulStatsAsPercentage;
#if SYS_MONITOR_VIEW_SUMM
	uint32_t SummTime = 0;
	uint16_t SumPercent;
#endif
uint32_t	iX;
uint16_t ulBuffSize = 0;
uint8_t ucSize;

	/* Make sure the write buffer does not contain a string. */
	*pcWriteBuffer = 0x00;

	/* Take a snapshot of the number of tasks in case it changes while this
	function is executing. */
	uxCurrentNumberOfTasks = uxTaskGetNumberOfTasks();
	uxArraySize = uxCurrentNumberOfTasks;

	/* Allocate an array index for each task. */
	pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks * sizeof( xTaskStatusType ) );

	if( pxTaskStatusArray != NULL ) {
		/* Generate the (binary) data. */
		uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );

		/* Avoid divide by zero errors. */
		if ( ulTotalTime > 0 )
		{

			#if SYS_MONITOR_VIEW_SUMM
				SumPercent = 0;
			#endif

			NTask = 1;
			/* Create a human readable table from the binary data. */
			for ( i = 0; i < uxArraySize; i++ ){

				// TODO - пересмотреть реализацию:
				// Сортировка по номеру задачи:
				do {
					iTask = 0xFF;
					for ( iX = 0; iX < uxArraySize; iX++ ) {
						if (pxTaskStatusArray[iX].xTaskNumber == NTask)
							iTask = iX;
					}//for(iX)
					NTask++;
				} while (iTask == 0xFF);

				ulStatsAsPercentage =  100 * ( (float)( pxTaskStatusArray[ iTask ].ulRunTimeCounter ) / ( ulTotalTime/100 ) );
				pxTaskStatusArray[iTask].ulRunTimeCounter *= SYS_MONITOR_TC_MULT;
				pxTaskStatusArray[iTask].ulRunTimeCounter /= SYS_MONITOR_TC_DIV;

				// TODO - разбить на несколько функций???
				ucSize = sprintf( pcWriteBuffer, (char*)"%s\t%lu\t%u\t%u.%02u%%\t%u\r\n",
								pxTaskStatusArray[iTask].pcTaskName,
								pxTaskStatusArray[iTask].ulRunTimeCounter,
								pxTaskStatusArray[iTask].usStackHighWaterMark,
								ulStatsAsPercentage / 100,
								ulStatsAsPercentage % 100,
								pxTaskStatusArray[iTask].uxCurrentPriority );

				pcWriteBuffer += ucSize;
				ulBuffSize += ucSize;

				#if SYS_MONITOR_VIEW_SUMM == 1
					SummTime += pxTaskStatusArray[iTask].ulRunTimeCounter;
					SumPercent += ulStatsAsPercentage;
				#endif

			}//for(i)

			#if SYS_MONITOR_VIEW_SUMM == 1

				// Percent Summ:
				ucSize = sprintf( pcWriteBuffer, "\r\n-----\r\nSPERCENT:\t%d%%\r\n", SumPercent);
				pcWriteBuffer += ucSize;
				ulBuffSize += ucSize;

				// Time Summ:
				ucSize = sprintf( pcWriteBuffer, "STIME:\t\t%lu\r\n", SummTime );
				pcWriteBuffer += ucSize;
				ulBuffSize += ucSize;

			#endif

			#if ( SYS_MONITOR_VIEW_RUN_TIME == 1 ) || (SYS_MONITOR_VIEW_HEAP == 1) || ( SYS_MONITOR_VIEW_BUFF == 1 )

				//--
				ucSize = sprintf( pcWriteBuffer, "\r\n-----\r\n" );
				pcWriteBuffer += ucSize;
				ulBuffSize += ucSize;

			#endif

			#if SYS_MONITOR_VIEW_RUN_TIME == 1

				// Run Time in Second:
				ucSize = sprintf( pcWriteBuffer, "RUN TIME:\t%lu\r\n", (ulTotalTime * SYS_MONITOR_TC_MULT) / ( SYS_MONITOR_TC_DIV) );
				pcWriteBuffer += ucSize;
				ulBuffSize += ucSize;

			#endif

			#if SYS_MONITOR_VIEW_TICK_CNT == 1

				// TickTime:
				ucSize = sprintf( pcWriteBuffer, "TICK CNT:\t%lu\r\n", xTaskGetTickCount() );
				pcWriteBuffer += ucSize;
				ulBuffSize += ucSize;

			#endif

		}//if(ulTotalTime)

		/* Free the array again. */
		vPortFree( pxTaskStatusArray );
	}
	return ulBuffSize;
}//SysMonitor_TimeStats

/*============================ IMPLEMENTATION (PUBLIC FUNCTIONS) =============*/

void SysMonitor_Init(void){
	// TODO Возможно двойное переполнение счетчика
	portSysMonitor_Init();
	xTaskCreate(SysMonitor_Task, TASK_SMONITOR_NAME, TASK_SMONITOR_STACK, NULL, TASK_SMONITOR_PRIORITY, NULL);
}//SysMonitor_Init

#endif /* USE_SYS_MONITOR */
