 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "sys_monitor_cfg.h"
#include "smonitor.h"
#include "inc/terminal.h"
#include "port/inc/port.h"

// FreeRTOS:
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PERCENT_ACCURACY    1000

char SMBuff[SYS_MONITOR_BUFF_SIZE];

#if !SYS_MONITOR_USE_FLOAT

// Check configuration:
// TODO - check and fix
#if ( (SYS_MONITOR_UPDATE_PERIOD_SEC * (SYS_MONITOR_STAT_TIMER_FREQ_HZ / SYS_MONITOR_TIME_CMN_DIV) * PERCENT_ACCURACY) >= UINT32_MAX )
#error "CHECK sys monitor config"
#endif

// TODO - check and fix
#if ( (SYS_MONITOR_UPDATE_PERIOD_SEC * SYS_MONITOR_STAT_TIMER_FREQ_HZ) <  0 * (PERCENT_ACCURACY * 100) )
#error "CHECK sys monitor config"
#endif

#endif /* SYS_MONITOR_USE_FLOAT */

static void Thread(void *pvParameters);

static int TimeStatistic(char *_buff, int _size);

void Thread(void *pvParameters)
{
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

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

      res = snprintf(&buff[len], size - len, "FREE HEAP:\t%u\r\n",
      xPortGetFreeHeapSize());
      if (res <= 0 || res >= size - len)
        break;
      len += res;

      res = snprintf(&buff[len], size - len, "MIN HEAP:\t%u\r\n",
      xPortGetMinimumEverFreeHeapSize());
      if (res <= 0 || res >= size - len)
        break;
      len += res;

      #if SYS_MONITOR_VIEW_DEBUG_INFO
      {
        res = snprintf(&buff[len], size - len, "FREE BUFF:\t%u\r\n", size - len);
        if (res <= 0 || res >= size - len)
          break;
        len += res;
      }
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

    portSysMonitor_TxBuff(SMBuff, len);

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SYS_MONITOR_UPDATE_PERIOD_SEC * 1000));
  }
}

int TimeStatistic(char *_buff, int _size)
{
  int len = 0;
  uint32_t ulTotalTime = 0;
  uint32_t LoadCoreTime[2] = { 0 };
  #if SYS_MONITOR_VIEW_DEBUG_INFO
    uint32_t SummRunTime[2] = { 0 };
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

  uint32_t timestamp;
  #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
    portALT_GET_RUN_TIME_COUNTER_VALUE(timestamp);
  #else
    timestamp = portGET_RUN_TIME_COUNTER_VALUE();
  #endif

  static uint32_t s_timestamp;
  uint32_t meas_period = timestamp - s_timestamp;
  s_timestamp = timestamp;

  /* Generate the (binary) data. */
  task_count = uxTaskGetSystemState(pTasks, task_count, &ulTotalTime);

  bool f_oversize = true;
  do
  {
    /* Avoid divide by zero errors. */
    if (ulTotalTime == 0)
      break;

    #if SYS_MONITOR_VIEW_DEBUG_INFO
      SumPercent[0] = 0;
      SumPercent[1] = 0;
    #endif

    int res;

    /* Create a human readable table from the binary data. */
    for (int i_core = 0; i_core < 1; i_core++) // TODO
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
          // TODO
//          if (task->xCoreID == i_core && task->xTaskNumber < min_TaskNumber
//              && task->xTaskNumber > prev_TaskNumber)
          if (0 == i_core && task->xTaskNumber < min_TaskNumber
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

        if (_size - len > configMAX_TASK_NAME_LEN - res)
        {
          memset(&_buff[len], ' ', configMAX_TASK_NAME_LEN - res);
          len += configMAX_TASK_NAME_LEN - res;
        }
        else
          break;

        #if SYS_MONITOR_VIEW_DEBUG_INFO == 1
        {
          uint32_t ulRunTimeCounter = pTasks[i_task].ulRunTimeCounter;
          if (ulRunTimeCounter > 9999)
            res = snprintf(&_buff[len], _size - len, "%luk\t", ulRunTimeCounter / 1000);
          else
            res = snprintf(&_buff[len], _size - len, "%lu \t", ulRunTimeCounter);
          if (res <= 0 || res >= _size - len)
            break;
          len += res;
        }
        #endif

        uint32_t percent;
        #if configCLEAR_RUN_TIME_STATS
        {
          #if SYS_MONITOR_USE_FLOAT
          {
            float f_percent = 100 * (float)pTasks[i_task].ulRunTimeCounter;
            f_percent /= meas_period;
            percent = (uint32_t)(f_percent * PERCENT_ACCURACY);
          }
          #else
          {
            #error "NOT IMPLEMENTED"
            percent = (pTasks[i_task].ulRunTimeCounter / SYS_MONITOR_TIME_CMN_DIV) * PERCENT_ACCURACY;
            percent /= (meas_period / 100);
            percent *= SYS_MONITOR_TIME_CMN_DIV;
          }
          #endif /* SYS_MONITOR_USE_FLOAT */
        }
        #else
        {
          #if SYS_MONITOR_USE_FLOAT
          {
            #error "NOT IMPLEMENTED"
          }
          #else
          {
            #error "NOT IMPLEMENTED"
            uint32_t ulStatsAsPercentage = pTasks[i_task].ulRunTimeCounter / (ulTotalTime / (PERCENT_ACCURACY * 100));
          }
          #endif /*  SYS_MONITOR_USE_FLOAT */
        }
        #endif /* configCLEAR_RUN_TIME_STATS */

        uint8_t int_part = percent / PERCENT_ACCURACY;
        uint16_t fract_part = percent % PERCENT_ACCURACY;
        res = snprintf(&_buff[len], _size - len, "%lu\t%2lu.%03lu%%\t%u\t:%s\r\n",
            pTasks[i_task].usStackHighWaterMark, int_part, fract_part, pTasks[i_task].uxCurrentPriority, state);
        if (res <= 0 || res >= _size - len)
          break;
        len += res;

        if (pTasks[i_task].uxCurrentPriority != 0)
          LoadCoreTime[i_core] += pTasks[i_task].ulRunTimeCounter;

        #if SYS_MONITOR_VIEW_DEBUG_INFO == 1
          SummRunTime[i_core] += pTasks[i_task].ulRunTimeCounter;
          SumPercent[i_core] += percent;
        #endif
      } // for(i)

      // Core Load:
      #if SYS_MONITOR_USE_FLOAT
        float load_core = (100 * (float)LoadCoreTime[i_core]) / meas_period;
        uint8_t int_part = (uint8_t)load_core;
        uint16_t fract_part = (int)(load_core * PERCENT_ACCURACY) % PERCENT_ACCURACY;
      #else
        uint32_t load_core = SummRunTime[i_core] * PERCENT_ACCURACY / (meas_period /100);
        uint8_t int_part = load_core / PERCENT_ACCURACY;
        uint16_t fract_part = load_core % PERCENT_ACCURACY;
      #endif

      res = snprintf(&_buff[len], _size - len, "\r\nLoad core:\t%lu.%03lu%%\r\n",
          int_part, fract_part);
      if (res <= 0 || res >= _size - len)
        break;
      len += res;

      #if SYS_MONITOR_VIEW_DEBUG_INFO == 1
      {
        // Percent Summ:
        uint32_t sum_percent = SumPercent[i_core];
        res = snprintf(&_buff[len], _size - len, "\r\nSum percent:\t%lu.%03lu%%\r\n",
            sum_percent / PERCENT_ACCURACY,
            sum_percent % PERCENT_ACCURACY);
        if (res <= 0 || res >= _size - len)
          break;
        len += res;

        // meas_period:
        res = snprintf(&_buff[len], _size - len, "Meas period:\t%lu\r\n", meas_period);
        if (res <= 0 || res >= _size - len)
          break;
        len += res;

        // Run Time Summ:
        res = snprintf(&_buff[len], _size - len, "Summ run time:\t%lu\r\n",
            SummRunTime[i_core]);
        if (res <= 0 || res >= _size - len)
          break;
        len += res;
      }
      #endif

      f_oversize = false;
    } // for(i)

    if (f_oversize)
      break;

    // Separator:
    res = snprintf(&_buff[len], _size - len, "\r\n====================\r\n\r\n");
    if (res <= 0 || res >= _size - len)
      break;
    len += res;

    #if SYS_MONITOR_VIEW_DEBUG_INFO == 1
    {
      // Run Time in Second:
      uint32_t run_time = ulTotalTime / SYS_MONITOR_STAT_TIMER_FREQ_HZ;
      res = snprintf(&_buff[len], _size - len, "Total time:\t%lu sec.\r\n", run_time);
      if (res <= 0 || res >= _size - len)
        break;
      len += res;
    }
    #endif

    #if SYS_MONITOR_VIEW_RTOS_TIME == 1
    {
      // TickTime:
      uint32_t rtos_time = xTaskGetTickCount() / configTICK_RATE_HZ;
      res = snprintf(&_buff[len], _size - len, "RTOS time:\t%lu sec.\r\n", rtos_time);
      if (res <= 0 || res >= _size - len)
        break;
      len += res;
    }
    #endif
  } while (0);

  /* Free the array again. */
  vPortFree(pTasks);

  return len;
}

void smonitor_Init(void)
{
  portSysMonitor_Init();
  xTaskCreatePinnedToCore(Thread, "SMON", 1024 + 512, NULL, 15, NULL, 0);
}
