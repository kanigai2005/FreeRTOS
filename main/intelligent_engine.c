#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "intelligent_engine.h"

#define TAG "INTELLIGENT_ENGINE"

extern TaskStatus_t status_array[32];
extern uint32_t ulTotalRunTime;
extern int num_tasks;
extern int free_heap_array[20];
extern int z;
extern float cpu_array[32];
extern int starvation_arr[32];
extern bool is_task_idle_profile(const char *task_name);

typedef struct
{
    TickType_t timestamp;
    char type[20];
    char message[100];
} EventLog;

EventLog logs[100];
int log_index = 0;
static uint32_t stack_history[32][10];
static int stack_index = 0;

void log_event(const char *type, const char *msg)
{
    EventLog *e = &logs[log_index % 100];
    e->timestamp = xTaskGetTickCount();
    strcpy(e->type, type);
    strcpy(e->message, msg);
    log_index++;
    ESP_LOGW("EVENT", "[%s] %s", type, msg);
}

static void check_heap()
{
    // Cast unsigned array entries to signed variables to completely eliminate wrap-underflow math anomalies.
    int32_t current_sample = (int32_t)free_heap_array[z % 20];
    int32_t past_sample = 0;

    if (z < 10)
    {
        past_sample = (int32_t)free_heap_array[(20 - (10 - z)) % 20];
    }
    else
    {
        past_sample = (int32_t)free_heap_array[(z - 10) % 20];
    }

    int32_t changed_heap = past_sample - current_sample; // Positive values equal raw leakage consumption
    ESP_LOGI(TAG, "Heap changes in last 10 cpu cycles is %ld bytes", changed_heap);

    float leak_rate = (float)changed_heap / 10.0f;
    ESP_LOGI(TAG, "Leak rate %.2f bytes/cycle", leak_rate);

    if (leak_rate <= 0.0f)
        return;

    int32_t last_known_heap = (int32_t)free_heap_array[(z - 1 + 20) % 20];
    int cycles = (int)((float)last_known_heap / leak_rate);
    ESP_LOGI(TAG, "Estimated exhaustion in %d cycles", cycles);

    if (cycles < 20)
    {
        ESP_LOGW(TAG, "Memory will become low soon");
        log_event("MEMORY", "Possible memory leak detected");
    }
}

static void change_priority()
{
    for (int i = 0; i < num_tasks; i++)
    {
        if (status_array[i].xHandle == NULL)
            continue;

        const char *task_name = status_array[i].pcTaskName;

        // SAFETY SHIELD: Permanently lock IDLE threads, system daemons, and
        // critical monitoring engines out of priority modification to prevent cascading freezes.
        if (is_task_idle_profile(task_name) ||
            strcmp(task_name, "Tmr Svc") == 0 ||
            strcmp(task_name, "ipc0") == 0 ||
            strcmp(task_name, "ipc1") == 0 ||
            strcmp(task_name, "WatchdogMonitor") == 0 ||
            strcmp(task_name, "TelemetryEngine") == 0 ||
            strcmp(task_name, "StorageGuardEngine") == 0 ||
            strcmp(task_name, "TrendAnalyzer") == 0)
        {
            continue;
        }

        float cpu_cons = 0.0f;
        if (ulTotalRunTime > 0)
        {
            cpu_cons = ((float)status_array[i].ulRunTimeCounter * 100.0f) / (float)ulTotalRunTime;
        }

        if (cpu_cons > 40.0f)
        {
            UBaseType_t curr_prio = uxTaskPriorityGet(status_array[i].xHandle);
            if (curr_prio > 1)
            {
                vTaskPrioritySet(status_array[i].xHandle, curr_prio - 1);
                char msg[100];
                sprintf(msg, "Priority reduced for %s", task_name);
                log_event("PRIORITY", msg);
            }
        }

        // Only boost priority if the task is genuinely flagged as starved
        // AND its current scheduling state is eReady (actively waiting for a core execution turn).
        if (starvation_arr[i] == 1 && status_array[i].eCurrentState == eReady)
        {
            UBaseType_t curr_prio = uxTaskPriorityGet(status_array[i].xHandle);
            // Cap priorities to logical max ranges below extreme kernel drivers
            if (curr_prio < (configMAX_PRIORITIES - 3))
            {
                vTaskPrioritySet(status_array[i].xHandle, curr_prio + 1);
                char msg[100];
                sprintf(msg, "Priority boosted for %s", task_name);
                log_event("PRIORITY", msg);
            }
        }
    }
}

static void check_cpu()
{
    for (int i = 0; i < num_tasks; i++)
    {
        int curr = status_array[i].ulRunTimeCounter;
        float cpu_curr = 0.0f;

        if (ulTotalRunTime > 0)
        {
            cpu_curr = ((float)curr * 100.0f) / (float)ulTotalRunTime;
        }

        float cpu_old = cpu_array[i];

        // Ensure we check for spikes only on non-idle threads to prevent noise flags
        if ((cpu_curr - cpu_old > 40.0f) && !is_task_idle_profile(status_array[i].pcTaskName))
        {
            ESP_LOGW(TAG, "CPU anomaly detected for %s with diff %.2f", status_array[i].pcTaskName, cpu_curr - cpu_old);
            char msg[100];
            sprintf(msg, "CPU spike in %s", status_array[i].pcTaskName);
            log_event("CPU", msg);
        }
        cpu_array[i] = cpu_curr;
    }
}

static void check_stack()
{
    for (int i = 0; i < num_tasks; i++)
    {
        stack_history[i][stack_index % 10] = status_array[i].usStackHighWaterMark;
        if (stack_index < 9)
        {
            continue;
        }
        uint32_t old = stack_history[i][(stack_index + 1) % 10];
        uint32_t recent = stack_history[i][stack_index % 10];
        if (old <= recent)
            continue;

        float rate = (float)(old - recent) / 10.0f;
        if (rate <= 0.0f)
            continue;

        int cycles = (int)((float)recent / rate);
        if (cycles < 20)
        {
            char msg[100];
            sprintf(msg, "%s stack may overflow in %d cycles", status_array[i].pcTaskName, cycles);
            log_event("STACK", msg);
        }
    }
    stack_index++;
}

void intelligent_engine_run()
{
    check_heap();
    check_cpu();
    change_priority();
    check_stack();
}