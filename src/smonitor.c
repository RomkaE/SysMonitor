 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../config/config.h"
#include "smonitor.h"
#include "port_smonitor.h"
#include "../utils/utils.h"
#include "esp_pm.h"
#include "WatchdogTask.h"

// FreeRTOS:
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF:
#include "soc/soc.h"
#include "esp_log.h"

#define LOG_TAG   "SMON"

#define MAX_TASK_NUMBER (100)

#define MAX_TASK_NAME (17)

typedef struct
{
  uint32_t min;
  uint32_t aver;
  uint32_t max;
  uint32_t i;
  uint32_t percent;
} SMonitor_Cnt_t;

char SMBuff[SYS_MONITOR_BUFF_SIZE];
#if CONFIG_PM_ENABLE
esp_pm_lock_handle_t m_Lock;
#endif

static void Thread(void *pvParameters);

static int TimeStatistic(char *_buff, int _size);

void Thread(void *pvParameters)
{
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
#if CONFIG_PM_ENABLE
  esp_err_t ret = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "monitor",
      &m_Lock);
  ESP_ERROR_CHECK(ret);
#endif

  ESP_LOGI(LOG_TAG, "Thread started");

  WDT_TASK_ADD(xTaskGetCurrentTaskHandle());

  while (1)
  {
    const int size = sizeof(SMBuff);
    char *buff = SMBuff;
    int res;
    int len = 0;
    bool f_oversize = true;
    do
    {
      res = snprintf(&buff[len], size - len, CLEARSCR);
      if (res <= 0 || res >= size - len)
        break;
      len += res;

      res = snprintf(&buff[len], size - len, (const char*) GOTOYX, 0, 0);
      if (res <= 0 || res >= size - len)
        break;
      len += res;

      // Time statistic:
      res = TimeStatistic(&buff[len], size - len);
      if (res <= 0 || res >= size - len)
        break;
      len += res;

#if SYS_MONITOR_VIEW_FREE_HEAP == 1
      res = snprintf(&buff[len], size - len, "FREE HEAP:\t%u\r\n",
      xPortGetFreeHeapSize());
      if (res <= 0 || res >= size - len)
        break;
      len += res;
#endif

#if SYS_MONITOR_VIEW_MIN_HEAP == 1
      res = snprintf(&buff[len], size - len, "MIN HEAP:\t%u\r\n",
      xPortGetMinimumEverFreeHeapSize());
      if (res <= 0 || res >= size - len)
        break;
      len += res;
#endif

#if SYS_MONITOR_VIEW_BUFF == 1
      res = snprintf(&buff[len], size - len, "FREE BUFF:\t%u\r\n", size - len);
      if (res <= 0 || res >= size - len)
        break;
      len += res;
#endif

      // Clear oversize status:
      f_oversize = false;
    }
    while (0);

    // Check status:
    if (f_oversize)
    {
      static const int err_sym_count = 3;
      if (size - len > err_sym_count)
      {
        memset(&buff[len], '!', err_sym_count);
        len += err_sym_count;
      }
      else
      {
        memset(&buff[size - err_sym_count], '!', err_sym_count);
        len = size;
      }
    }

#if CONFIG_PM_ENABLE
    ret = esp_pm_lock_acquire(m_Lock);
    ESP_ERROR_CHECK(ret);
#endif
    portSysMonitor_TxBuff(len);
    WDT_TASK_RST();
    vTaskDelay(2); // wait for uart flush
#if CONFIG_PM_ENABLE
    ret = esp_pm_lock_release(m_Lock);
    ESP_ERROR_CHECK(ret);
#endif
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SYS_MONITOR_TIME_UPDATE));
  }
}

int TimeStatistic(char *_buff, int _size)
{
  int len = 0;
  uint32_t ulTotalTime = 0;

#if SYS_MONITOR_VIEW_SUMM
  uint32_t SummTime[2] = { 0 };
  uint32_t SumPercent[2] = { 0 };
#endif

  /* Take a snapshot of the number of tasks in case it changes while this
   function is executing. */
  UBaseType_t task_count = uxTaskGetNumberOfTasks();

  /* Allocate an array index for each task. */
  TaskStatus_t *pTasks = pvPortMalloc(task_count * sizeof(TaskStatus_t));
  if (pTasks == NULL)
  {
    int res = snprintf(&_buff[len], _size - len, "FAIL allocate the task array in memory!");
    if (res > 0 && res < _size - len)
      len += res;
    return len;
  }

  /* Generate the (binary) data. */
  task_count = uxTaskGetSystemState(pTasks, task_count, &ulTotalTime);

  bool f_oversize = true;
  do
  {
    /* Avoid divide by zero errors. */
    if (ulTotalTime == 0)
      break;

#if SYS_MONITOR_VIEW_SUMM
    SumPercent[0] = 0;
    SumPercent[1] = 0;
#endif

    int res;

    /* Create a human readable table from the binary data. */
    for (int i_core = 0; i_core < 2; i_core++)
    {
      res = snprintf(&_buff[len], _size - len, "\r\n====== CORE %d ======\r\n", i_core);
      if (res <= 0 || res >= _size - len)
        break;
      len += res;

      UBaseType_t prev_TaskNumber = 0;
      for (int i = 0; i < task_count; i++)
      {
        // Sort list by xTaskNumber:
        UBaseType_t min_TaskNumber = UINT32_MAX;
        int i_task = -1;
        for (int j = 0; j < task_count; j++)
        {
          TaskStatus_t *task = &pTasks[j];
          if (task->xCoreID == i_core && task->xTaskNumber < min_TaskNumber
              && task->xTaskNumber > prev_TaskNumber)
          {
            min_TaskNumber = task->xTaskNumber;
            i_task = j;
          }
        }
        if (i_task < 0)
          continue;

        prev_TaskNumber = pTasks[i_task].xTaskNumber;

        char *state;
        switch (pTasks[i_task].eCurrentState)
        {
          case eRunning:
            state = "Run";
          break;
          case eReady:
            state = "Ready";
          break;
          case eBlocked:
            state = "Block";
          break;
          case eSuspended:
            state = "Suspend";
          break;
          case eDeleted:
            state = "Del";
          break;
          default:
            state = "Unknown";
          break;
        }

        res = snprintf(&_buff[len], _size - len, "%02d. %s", pTasks[i_task].xTaskNumber,
            pTasks[i_task].pcTaskName);
        if (res <= 0 || res >= _size - len)
          break;
        len += res;

        if (_size - len > MAX_TASK_NAME - res)
        {
          memset(&_buff[len], ' ', MAX_TASK_NAME - res);
          len += MAX_TASK_NAME - res;
        }
        else
          break;

        #if configCLEAR_RUN_TIME_STATS
          uint32_t ulStatsAsPercentage = ((pTasks[i_task].ulRunTimeCounter) / ( SYS_MONITOR_TIME_UPDATE/10));
        #else
          uint32_t ulStatsAsPercentage = ((pTasks[i_task].ulRunTimeCounter) / (ulTotalTime / 10000));
        #endif /* configCLEAR_RUN_TIME_STATS */
        uint32_t ulRunTimeCounter = (pTasks[i_task].ulRunTimeCounter
            * SYS_MONITOR_TC_MULT) / SYS_MONITOR_TC_DIV;

        if (ulRunTimeCounter > 9999)
          res = snprintf(&_buff[len], _size - len, "%uk\t", ulRunTimeCounter / 1000);
        else res = snprintf(&_buff[len], _size - len, "%u \t", ulRunTimeCounter);
        if (res <= 0 || res >= _size - len)
          break;
        len += res;

        res = snprintf(&_buff[len], _size - len, "%u\t%2u.%02u%%\t%u\t:%s\r\n",
            pTasks[i_task].usStackHighWaterMark, ulStatsAsPercentage / 100,
            ulStatsAsPercentage % 100, pTasks[i_task].uxCurrentPriority, state);
        if (res <= 0 || res >= _size - len)
          break;
        len += res;

#if SYS_MONITOR_VIEW_SUMM == 1
        if (pTasks[i_task].uxCurrentPriority != 0)
        {
          SummTime[i_core] += pTasks[i_task].ulRunTimeCounter;
          SumPercent[i_core] += ulStatsAsPercentage;
        }
#endif
      } // for(i)

#if SYS_MONITOR_VIEW_SUMM == 1
      // Percent Summ:
      res = snprintf(&_buff[len], _size - len, "\r\nLoad core %d:\t%u.%02u%%\r\n", i_core,
          SumPercent[i_core] / 100, SumPercent[i_core] % 100);
      if (res <= 0 || res >= _size - len)
        break;
      len += res;

      // Time Summ:
      res = snprintf(&_buff[len], _size - len, "Time core %d:\t%u.%03u sec.\r\n", i_core,
          SummTime[i_core] / SYS_MONITOR_TC_DIV / 1000,
          (SummTime[i_core] % ( SYS_MONITOR_TC_DIV * 1000) / 1000));
      if (res <= 0 || res >= _size - len)
        break;
      len += res;
#endif

      f_oversize = false;
    } // for(i)

    if (f_oversize)
      break;

#if ( SYS_MONITOR_VIEW_RUN_TIME == 1 ) || (SYS_MONITOR_VIEW_HEAP == 1) || ( SYS_MONITOR_VIEW_BUFF == 1 )
    res = snprintf(&_buff[len], _size - len, "\r\n====================\r\n\r\n");
    if (res <= 0 || res >= _size - len)
      break;
    len += res;
#endif

#if SYS_MONITOR_VIEW_RUN_TIME == 1
    // Run Time in Second:
    uint32_t run_time = (ulTotalTime * SYS_MONITOR_TC_MULT) / SYS_MONITOR_TC_DIV;
    res = snprintf(&_buff[len], _size - len, "RUN TIME:\t%u.%03u sec.\r\n", run_time / 1000,
        run_time % 1000);
    if (res <= 0 || res >= _size - len)
      break;
    len += res;
#endif

#if SYS_MONITOR_VIEW_TICK_CNT == 1
    // TickTime:
    uint32_t rtos_time = (1000 * xTaskGetTickCount()) / pdMS_TO_TICKS(1000);
    res = snprintf(&_buff[len], _size - len, "RTOS TIME:\t%u.%03u sec.\r\n",
        rtos_time / 1000, rtos_time % 1000);
    if (res <= 0 || res >= _size - len)
      break;
    len += res;
#endif
  } while (0);

  /* Free the array again. */
  vPortFree(pTasks);

  return len;
} //SysMonitor_TimeStats

void smonitor_Init(void)
{
  portSysMonitor_Init();
  xTaskCreatePinnedToCore(Thread, TASK_NAME_SMONITOR, TASK_STACK_SMONITOR, NULL,
  TASK_PRIO_SMONITOR, NULL, TASK_CPU_NUM);
}
