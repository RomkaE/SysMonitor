/*
 * port_smonitor.c
 *
 *  Created on: 2020-01-07
 *      Author: Roman Egoshin
 */

#if USE_SYS_MONITOR == 1

/*============================ INCLUDES ======================================*/

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include "../common/custom_board.h"
#include "../../config/config.h"
#include "port_smonitor.h"

// ESP-IDF:
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "app_error_check.h"

/*============================ PRIVATE DEFINITIONS ===========================*/

#define LOG_TAG   "SMON_PORT"

/*============================ TYPES =========================================*/


/*============================ VARIABLES =====================================*/

extern char SMBuff[SYS_MONITOR_BUFF_SIZE];

/*============================ PRIVATE PROTOTYPES ============================*/


/*============================ IMPLEMENTATION (PRIVATE FUNCTIONS) ============*/


/*============================ IMPLEMENTATION (PUBLIC FUNCTIONS) =============*/

void portSysMonitor_Init(void)
{
  const uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB
  };

  esp_err_t res;
  res = uart_param_config(SYSMON_UART_NUM, &uart_config);
  APP_CHECK_ARG(res == ESP_OK);
  res = uart_set_pin(SYSMON_UART_NUM, SYSMON_TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  APP_CHECK_ARG(res == ESP_OK);
  res = uart_driver_install(SYSMON_UART_NUM, 164, 0, 0, NULL, 0);
  APP_CHECK_ARG(res == ESP_OK);
}

int portSysMonitor_TxBuff(uint16_t _lenght)
{
  int txBytes = uart_write_bytes(SYSMON_UART_NUM, SMBuff, _lenght);
  return txBytes;
}

#endif /* USE_SYS_MONITOR */
