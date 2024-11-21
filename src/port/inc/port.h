
#ifndef PORT_SMONITOR_H_
#define PORT_SMONITOR_H_

#include <stdint.h>

void portSysMonitor_Init(void);

void portSysMonitor_TxBuff(const void *_buff, uint16_t _lenght);

#endif /* PORT_SMONITOR_H_ */
