#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "storage_guard.h"
#include "watchdog_manager.h"
#include "intelligent_engine.h"

static const char *TAG = "storage_guard";
extern SemaphoreHandle_t system_mutex;
extern float cpu_stability_score;

TaskStatus_t top_over_allocated_tasks[5];
extern int task_count[20];
extern int h;
extern int free_heap_array[20];
extern int z;

#define CRITICAL_HEAP_LIMIT (180 * 1024)

bool is_critical(const char *name)
{
    const char *critical_processes[] = {
        "IDLE", "IDLE0", "IDLE1", "ipc0", "ipc1",
        "esp_timer", "tiT", "sys_evt", "main", "StorageGuardEngine", "TelemetryEngine", "TrendAnalyzer"};
    int n = sizeof(critical_processes) / sizeof(critical_processes[0]);
    for (int i = 0; i < n; i++)
    {
        if (strcmp(name, critical_processes[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

void find_and_free_resources(void)
{
    UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
    if(num_tasks > 32)
        num_tasks = 32;

    TaskStatus_t local_status_array[32];
    num_tasks = uxTaskGetSystemState(local_status_array, num_tasks, NULL);

    UBaseType_t watermark_threshold = 200;

    for(UBaseType_t i = 0; i < num_tasks; i++)
    {
        if(!is_critical(local_status_array[i].pcTaskName))
        {
            if(local_status_array[i].usStackHighWaterMark < watermark_threshold)
            {
                ESP_LOGE(TAG, "Restarting unhealthy task: %s", local_status_array[i].pcTaskName);
                recover_task(local_status_array[i].pcTaskName);
            }
        }
    }
}

int monitoring_task(TaskStatus_t task)
{
    return task.usStackHighWaterMark;
}

void is_over_allocated(TaskStatus_t task)
{
    int i = 0;
    while (i < 5)
    {
        if (top_over_allocated_tasks[i].xHandle == NULL)
        {
            top_over_allocated_tasks[i] = task;
            break;
        }
        else if (top_over_allocated_tasks[i].usStackHighWaterMark < task.usStackHighWaterMark)
        {
            TaskStatus_t temp = top_over_allocated_tasks[i];
            top_over_allocated_tasks[i] = task;
            task = temp;
        }
        i++;
    }
}

void reduce_resource_allocation(void)
{
    UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
    if (num_tasks > 32)
        num_tasks = 32;

    task_count[(h % 20)] = num_tasks;
    h++;

    TaskStatus_t local_status_array[32];
    num_tasks = uxTaskGetSystemState(local_status_array, num_tasks, NULL);

    int excess_resources[32] = {0};
    int is_excess[32] = {0};
    int threshold = 800;

    memset(top_over_allocated_tasks, 0, sizeof(top_over_allocated_tasks));

    for (int loop = 0; loop < 10; loop++)
    {
        for (UBaseType_t i = 0; i < num_tasks; i++)
        {
            int diff = monitoring_task(local_status_array[i]) - threshold;
            if (diff > 0)
            {
                excess_resources[i] += diff;
                is_excess[i] += 1;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    for (UBaseType_t i = 0; i < num_tasks; i++)
    {
        if (is_excess[i] > 6)
        {
            ESP_LOGI(TAG, "Task %s was allocated with average excess resource of %.2f bytes", local_status_array[i].pcTaskName, (float)excess_resources[i] / 10.0f);
            is_over_allocated(local_status_array[i]);
        }
    }
}

void run_storage_guard_sweep(void)
{
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    free_heap_array[z % 20] = free_heap;
    z++;

    if(free_heap < CRITICAL_HEAP_LIMIT)
    {
        ESP_LOGE(TAG, "Free heap is critically low: %d bytes", free_heap);

        log_event("MEMORY", "Critical heap threshold reached");

        float reduced_score = ((float)(CRITICAL_HEAP_LIMIT - free_heap) * 100.0f) / (float)CRITICAL_HEAP_LIMIT;

        cpu_stability_score -= reduced_score;

        reduce_resource_allocation();
        find_and_free_resources();

        size_t new_free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

        float new_reduced_score = ((float)(CRITICAL_HEAP_LIMIT - new_free_heap) * 100.0f) / (float)CRITICAL_HEAP_LIMIT;

        cpu_stability_score += (reduced_score - new_reduced_score);

        ESP_LOGI(TAG, "Post-cleanup free heap: %d bytes", new_free_heap);
    }
    else
    {
        if(xSemaphoreTake(system_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            xSemaphoreGive(system_mutex);
        }
    }
}

void init_storage_guard(void)
{
    while (1)
    {
        run_storage_guard_sweep();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}