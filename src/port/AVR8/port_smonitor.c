/*
 * port_smonitor.c
 *
 *  Created on: 
 *      Author: 
 */

/*============================ INCLUDES ======================================*/
#include <stdint.h>
#include "board.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/*============================ PRIVATE DEFINITIONS ===========================*/


/*============================ TYPES =========================================*/


/*============================ VARIABLES =====================================*/
extern char SMBuff[SYS_MONITOR_BUFF_SIZE];
uint16_t	iBuff = 0, lenghtBuff;

/*============================ PRIVATE PROTOTYPES ============================*/


/*============================ IMPLEMENTATION (PRIVATE FUNCTIONS) ============*/
SIGNAL(USART0_UDRE_vect){
	UDR0 = SMBuff[iBuff];
	iBuff++;
	if (iBuff > lenghtBuff) {
		UCSR0B &= ~(1<<UDRIE0);
	}//if
}//USART_UDRE_vect

/*============================ IMPLEMENTATION (PUBLIC FUNCTIONS) =============*/

void portSysMonitor_Init(void){
	// USART:
	UBRR0L = 16;						// 57600 fosc = 16MHz
	UCSR0B |= (1<<TXEN0);
}//portSysMonitor_Init

void portSysMonitor_TxBuff(uint16_t lenght){
	iBuff = 0;
	lenghtBuff = lenght;
	UCSR0B |= (1<<UDRIE0);
}//portSysMonitor_TxBuff

void portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS(void){
	// TC:
	TCCR3B = 4;
}//portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS

uint32_t portSysMonitor_GetRunTimeCounterValue(void){
static uint32_t cnt = 0;
static uint16_t prevTC = 0;
uint16_t TC;
uint32_t dTC;

	TC = TCNT3;

	if (prevTC > TC)
		dTC = TC + (0xFFFF - prevTC);
	else
		dTC = TC - prevTC;

	prevTC = TC;
	cnt = cnt + dTC;

	return cnt;
}//portSysMonitor_GetRunTimeCounterValue
