
#include "config/config.h"
#include "../inc/port.h"

#include "../common/custom_board.h"

// ESP-IDF:
#include "driver/uart.h"
#include "driver/gpio.h"

#include "app_error_check.h"

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

int portSysMonitor_TxBuff(const void *_buff, uint16_t _lenght)
{
  int txBytes = uart_write_bytes(SYSMON_UART_NUM, _buff, _lenght);
  return txBytes;
}
