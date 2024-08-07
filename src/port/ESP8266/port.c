
#include "sys_monitor_cfg.h"
#include "../inc/port.h"

// FreeRTOS:
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP8266 sdk:
#include "sdkconfig.h"
#include "driver/uart.h"

#if CONFIG_PM_ENABLE
//esp_pm_lock_handle_t m_Lock;
#endif

void portSysMonitor_Init(void)
{

  #if CONFIG_PM_ENABLE
//    esp_err_t ret = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "monitor", &m_Lock);
//    ESP_ERROR_CHECK(ret);
  #endif

  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	  .rx_flow_ctrl_thresh = 0
  };

  esp_err_t res;
  res = uart_param_config(UART_NUM_1, &uart_config);
  assert(res == ESP_OK);
  res = uart_driver_install(UART_NUM_1, 0, 256, 0, NULL, 0);
  assert(res == ESP_OK);
}

int portSysMonitor_TxBuff(const void *_buff, uint16_t _lenght)
{

  #if CONFIG_PM_ENABLE
//    esp_err_t ret = esp_pm_lock_acquire(m_Lock);
//    ESP_ERROR_CHECK(ret);
  #endif

  int txBytes = uart_write_bytes(UART_NUM_1, _buff, _lenght);

  // wait for uart flush
  vTaskDelay(2);          // TODO - add wait event

  #if CONFIG_PM_ENABLE
//    ret = esp_pm_lock_release(m_Lock);
//    ESP_ERROR_CHECK(ret);
  #endif

  return txBytes;
}
