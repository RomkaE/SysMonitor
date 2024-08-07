
#ifndef PORT_SMONITOR_H_
#define PORT_SMONITOR_H_

#include <stdint.h>
#include "esp_system.h"

#define xPortGetFreeHeapSize() esp_get_free_heap_size()

#define xPortGetMinimumEverFreeHeapSize() esp_get_minimum_free_heap_size()

void portSysMonitor_Init(void);

int portSysMonitor_TxBuff(const void *_buff, uint16_t _lenght);

#endif /* PORT_SMONITOR_H_ */
