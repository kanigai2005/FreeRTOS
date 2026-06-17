#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "system_health.h"
#include "storage_guard.h"
#include "watchdog_manager.h"

static const char *TAG = "OS_CORE_INIT";

SemaphoreHandle_t system_mutex = NULL;
float cpu_stability_score = 100.0f;

int free_heap_array[20] = {0};
int task_count[20] = {0};
float cpu_load_array[20] = {0.0};

int z = 0;
int h = 0;
int cl = 0;

void vHistoricalTrendTask(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(10000));
    while (1)
    {
        if (xSemaphoreTake(system_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            printf("\n=================== OS HISTORICAL TREND REPORT ===================\n");
            printf("Last Heap Records (Bytes):   [");
            for (int i = 0; i < 10; i++)
            {
                printf(" %d ", free_heap_array[i % 20]);
            }
            printf("...]\n");

            printf("Last Active Task Counts:     [");
            for (int i = 0; i < 10; i++)
            {
                printf(" %d ", task_count[i % 20]);
            }
            printf("]\n");

            printf("Last CPU Load Curves (%%):    [");
            for (int i = 0; i < 10; i++)
            {
                printf(" %.1f%% ", cpu_load_array[i % 20]);
            }
            printf("]\n");

            printf("Current Live Stability Core: %.2f%%\n", cpu_stability_score);
            printf("==================================================================\n\n");

            xSemaphoreGive(system_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}

void vTelemetryCoreTask(void *pvParameters)
{
    ESP_LOGI("CORE_1", "System Health Monitoring Thread Online on CPU Core 1.");
    const TickType_t xDelay = pdMS_TO_TICKS(20000);
    while (1)
    {
        compile_system_metrics();
        calculate_system_load();
        vTaskDelay(xDelay);
    }
}

void vStorageGuardCoreTask(void *pvParameters)
{
    ESP_LOGI("CORE_0", "Defensive Storage Guard Engine Running on CPU Core 0.");
    init_storage_guard();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Bootloader Hand-off complete. Constructing Custom OS Core Kernel Architecture...");

    system_mutex = xSemaphoreCreateMutex();

    if (system_mutex == NULL)
    {
        ESP_LOGE(TAG, "CRITICAL ERROR: Kernel Primitive Mutex Allocation Failure.");
        return;
    }

    TaskHandle_t storage_handle = NULL;
    TaskHandle_t telemetry_handle = NULL;
    TaskHandle_t trend_handle = NULL;
    TaskHandle_t watchdog_handle = NULL;

    xTaskCreatePinnedToCore(
        vStorageGuardCoreTask,
        "StorageGuardEngine",
        4096,
        NULL,
        3,
        &storage_handle,
        0);

    watchdog_register_task(
        storage_handle,
        "StorageGuardEngine");

    register_recoverable_task(
        "StorageGuardEngine",
        vStorageGuardCoreTask,
        4096,
        3,
        0);

    xTaskCreatePinnedToCore(
        vTelemetryCoreTask,
        "TelemetryEngine",
        8192,
        NULL,
        2,
        &telemetry_handle,
        1);

    watchdog_register_task(
        telemetry_handle,
        "TelemetryEngine");

    register_recoverable_task(
        "TelemetryEngine",
        vTelemetryCoreTask,
        8192,
        2,
        1);

    xTaskCreate(
        vHistoricalTrendTask,
        "TrendAnalyzer",
        3072,
        NULL,
        1,
        &trend_handle);

    watchdog_register_task(
        trend_handle,
        "TrendAnalyzer");

    register_recoverable_task(
        "TrendAnalyzer",
        vHistoricalTrendTask,
        3072,
        1,
        tskNO_AFFINITY);

    xTaskCreate(
        watchdog_monitor_task,
        "WatchdogMonitor",
        4096,
        NULL,
        4,
        &watchdog_handle);

    watchdog_register_task(
        watchdog_handle,
        "WatchdogMonitor");

    register_recoverable_task(
        "WatchdogMonitor",
        watchdog_monitor_task,
        4096,
        4,
        tskNO_AFFINITY);

    ESP_LOGI(TAG, "All Custom OS tasks deployed across dual-core processing lines successfully.");
}