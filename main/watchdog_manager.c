#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "watchdog_manager.h"

WatchdogEntry watchdog_table[32];
RecoveryEntry recovery_table[32];

static int wc = 0;
static int rc = 0;

void register_recoverable_task(const char *name, TaskFunction_t func, uint16_t stack, UBaseType_t prio, BaseType_t core)
{
    strcpy(recovery_table[rc].name, name);
    recovery_table[rc].function = func;
    recovery_table[rc].stack = stack;
    recovery_table[rc].priority = prio;
    recovery_table[rc].core = core;
    rc++;
}

void recover_task(const char *name)
{
    for (int i = 0; i < rc; i++)
    {
        if (strcmp(recovery_table[i].name, name) == 0)
        {
            TaskHandle_t nh;
            TaskHandle_t handle = NULL;
            int j = 0;
            for (j = 0; j < wc; j++)
            {
                if (strcmp(watchdog_table[j].name, name) == 0)
                {
                    handle = watchdog_table[j].handle;
                    break;
                }
            }
            if (handle != NULL)
                vTaskDelete(handle);
            xTaskCreatePinnedToCore(
                recovery_table[i].function,
                recovery_table[i].name,
                recovery_table[i].stack,
                NULL,
                recovery_table[i].priority,
                &nh,
                recovery_table[i].core);
            ESP_LOGI("RECOVERY", "Restarted %s", name);
            watchdog_table[j].handle = nh;
            watchdog_table[j].last_seen = xTaskGetTickCount();
            watchdog_table[j].heartbeat = 0;
            break;
        }
    }
}

void watchdog_register_task(TaskHandle_t handle, const char *name)
{
    strcpy(watchdog_table[wc].name, name);
    watchdog_table[wc].handle = handle;
    watchdog_table[wc].heartbeat = 0;
    watchdog_table[wc].last_seen = xTaskGetTickCount();
    wc++;
}

void watchdog_kick(TaskHandle_t handle)
{
    for (int i = 0; i < wc; i++)
    {
        if (watchdog_table[i].handle == handle)
        {
            watchdog_table[i].heartbeat++;
            watchdog_table[i].last_seen = xTaskGetTickCount();
        }
    }
}

void watchdog_monitor_task(void *pvParameters)
{
    while (1)
    {
        TickType_t now = xTaskGetTickCount();
        for (int i = 0; i < wc; i++)
        {
            if (now - watchdog_table[i].last_seen > pdMS_TO_TICKS(15000))
            {
                ESP_LOGE("WATCHDOG", "Task %s frozen", watchdog_table[i].name);
                recover_task(watchdog_table[i].name);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}