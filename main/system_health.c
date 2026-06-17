#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "system_health.h"
#include "intelligent_engine.h"

static const char *TAG = "system_health";
extern SemaphoreHandle_t system_mutex;

static uint32_t f = 0, s = 0, t = 0;
static const char *first = "None";
static const char *second = "None";
static const char *third = "None";

static int rn = 0, rd = 0, bl = 0, sp = 0, dl = 0;
extern float cpu_stability_score;
static int cpu_hog = 0;
static int starvation_count = 0;

// Global metrics buffers to link with analytics architecture
TaskStatus_t status_array[32];
uint32_t ulTotalRunTime = 0;
int num_tasks = 0;

extern int free_heap_array[20];
extern float cpu_load_array[20];
extern int cl;
int starvation_arr[32] = {0};
float cpu_array[32] = {0.0f};

/**
 * Validates if a task is a system idle thread by name strings
 */
bool is_task_idle_profile(const char *task_name)
{
    if (task_name == NULL)
        return false;
    return (strcmp(task_name, "IDLE") == 0 ||
            strcmp(task_name, "IDLE0") == 0 ||
            strcmp(task_name, "IDLE1") == 0);
}

void is_starvation(TaskStatus_t task, uint32_t total_run_time, int i)
{
    starvation_arr[i] = 0;
    if (total_run_time > 0)
    {
        uint32_t pct = (task.ulRunTimeCounter * 100) / total_run_time;

        // CORRECTION: Starvation logic must ignore IDLE loops entirely
        if (pct < 5 && task.eCurrentState == eReady && !is_task_idle_profile(task.pcTaskName))
        {
            starvation_arr[i] = 1;
            starvation_count++;
            ESP_LOGW(TAG, "Task %s is experiencing CPU starvation with only %lu percent of CPU time", task.pcTaskName, pct);
            char msg[100];
            sprintf(msg, "Starvation detected in %s", task.pcTaskName);
            log_event("STARVATION", msg);
        }
    }
}

void log_task_state(TaskStatus_t task)
{
    switch (task.eCurrentState)
    {
    case eRunning:
        ESP_LOGI(TAG, "Task = %s State = Running", task.pcTaskName);
        rn++;
        break;
    case eReady:
        // Log clarity optimization to express ready context state
        ESP_LOGI(TAG, "Task = %s State = Ready (Awaiting Cycle)", task.pcTaskName);
        rd++;
        break;
    case eBlocked:
        ESP_LOGI(TAG, "Task = %s State = Blocked", task.pcTaskName);
        bl++;
        break;
    case eSuspended:
        ESP_LOGI(TAG, "Task = %s State = Suspended", task.pcTaskName);
        sp++;
        break;
    case eDeleted:
        ESP_LOGI(TAG, "Task = %s State = Deleted", task.pcTaskName);
        dl++;
        break;
    default:
        ESP_LOGI(TAG, "Task = %s State = Unknown State", task.pcTaskName);
    }
}

void state_task(void)
{
    UBaseType_t tasks_found = uxTaskGetNumberOfTasks();
    if (tasks_found > 32)
        tasks_found = 32;

    tasks_found = uxTaskGetSystemState(status_array, tasks_found, NULL);
    for (UBaseType_t i = 0; i < tasks_found; i++)
    {
        log_task_state(status_array[i]);
    }
    ESP_LOGI(TAG, "Total tasks:%d \nRunning:%d \nReady:%d \nBlocked:%d \nSuspended:%d \nDeleted:%d", num_tasks, rn, rd, bl, sp, dl);
}

void arrange_top_three_tasks(TaskStatus_t task)
{
    if (task.ulRunTimeCounter > f)
    {
        t = s;
        s = f;
        f = task.ulRunTimeCounter;
        third = second;
        second = first;
        first = task.pcTaskName;
    }
    else if (task.ulRunTimeCounter > s)
    {
        t = s;
        s = task.ulRunTimeCounter;
        third = second;
        second = task.pcTaskName;
    }
    else if (task.ulRunTimeCounter > t)
    {
        t = task.ulRunTimeCounter;
        third = task.pcTaskName;
    }
}

void is_cpu_hog(TaskStatus_t task, uint32_t total_run_time)
{
    if (total_run_time > 0)
    {
        uint32_t cpu_pct = (task.ulRunTimeCounter * 100) / total_run_time;

        // CORRECTION: Exclude IDLE task limits from registering as CPU hogs
        if (cpu_pct > 40 && task.eCurrentState == eRunning && !is_task_idle_profile(task.pcTaskName))
        {
            ESP_LOGI(TAG, "Task %s is consuming more cpu with %lu percent", task.pcTaskName, cpu_pct);
            cpu_hog++;
            char msg[100];
            sprintf(msg, "CPU hog detected in %s", task.pcTaskName);
            log_event("CPU", msg);
        }
    }
}

void calc_cpu_stability_score(int running_tasks)
{
    if (running_tasks == 0)
        return;

    int xtra_task = running_tasks - 40;

    float reduced_score = ((float)cpu_hog / (float)running_tasks) * 100.0f;
    cpu_stability_score -= reduced_score;
    ESP_LOGI(TAG, "Due to cpu hog of %d processes the cpu stability score is reduced by %.2f percent", cpu_hog, reduced_score);

    float reduce_score = ((float)starvation_count / (float)running_tasks) * 100.0f;
    cpu_stability_score -= reduce_score;
    ESP_LOGI(TAG, "Due to CPU starvation of %d process, the cpu stability score is reduced by %.2f percent", starvation_count, reduce_score);

    if (xtra_task > 0)
    {
        cpu_stability_score -= (xtra_task * 1.5f);
        ESP_LOGI(TAG, "Due to excess number of tasks %d the cpu stability score is reduced by %.2f percent", xtra_task, (xtra_task * 1.5f));
    }

    // Safety bounding check to guarantee limits stay within 0-100%
    if (cpu_stability_score < 0.0f)
        cpu_stability_score = 0.0f;
    if (cpu_stability_score > 100.0f)
        cpu_stability_score = 100.0f;

    ESP_LOGI(TAG, "CPU stability score : %.2f percent", cpu_stability_score);

    if (cpu_stability_score < 50.0f)
    {
        ESP_LOGW(TAG, "CPU stability score is critically low at %.2f percent", cpu_stability_score);
    }
    else
    {
        ESP_LOGI(TAG, "CPU stability score is healthy with %.2f percent", cpu_stability_score);
    }
}

void generate_recommendations(int running_tasks)
{
    if (cpu_stability_score < 50.0f)
    {
        ESP_LOGW(TAG, "CPU stability score is critically low at %.2f percent. Recommendations:", cpu_stability_score);
        ESP_LOGW(TAG, "1. Optimize or refactor CPU hogging tasks to reduce their CPU usage.");
        ESP_LOGW(TAG, "2. Investigate and resolve any tasks experiencing CPU starvation.");
        ESP_LOGW(TAG, "3. Consider reducing the number of active tasks or optimizing task scheduling to improve overall CPU performance.");
    }
    else
    {
        ESP_LOGI(TAG, "CPU stability score is healthy with %.2f percent. No immediate recommendations needed.", cpu_stability_score);
    }
}

void calculate_system_load(void)
{
    rn = rd = bl = sp = dl = 0;
    cpu_hog = 0;
    starvation_count = 0;
    f = s = t = 0;
    first = "None";
    second = "None";
    third = "None";

    UBaseType_t total_tasks = uxTaskGetNumberOfTasks();
    if (total_tasks > 32)
        total_tasks = 32;

    total_tasks = uxTaskGetSystemState(status_array, total_tasks, &ulTotalRunTime);
    num_tasks = total_tasks;

    if (num_tasks > 0)
    {
        uint32_t totalruncounter = 0;

        for (UBaseType_t i = 0; i < (UBaseType_t)num_tasks; i++)
        {
            totalruncounter += status_array[i].ulRunTimeCounter;
            is_starvation(status_array[i], ulTotalRunTime, i);
            is_cpu_hog(status_array[i], ulTotalRunTime);
            arrange_top_three_tasks(status_array[i]);

            if (ulTotalRunTime > 0)
            {
                ESP_LOGI(TAG, "Task %s has run for %lu ticks %lu percent of total cpu is used",
                         status_array[i].pcTaskName,
                         status_array[i].ulRunTimeCounter,
                         (status_array[i].ulRunTimeCounter * 100) / ulTotalRunTime);
            }
        }

        state_task();

        if (ulTotalRunTime > 0)
        {
            printf("\n Top 3 CPU consuming tasks are \n 1st %s with %lu%% \n2nd %s with %lu%% \n3rd %s with %lu%%\n",
                   first, (f * 100) / ulTotalRunTime,
                   second, (s * 100) / ulTotalRunTime,
                   third, (t * 100) / ulTotalRunTime);

            float total_cpu_load = ((float)totalruncounter * 100.0f) / (float)ulTotalRunTime;
            ESP_LOGI(TAG, "Total CPU Load: %.2f percent", total_cpu_load);

            cpu_load_array[cl % 20] = total_cpu_load;
            cl++;
        }

        cpu_stability_score = 100.0f;
        calc_cpu_stability_score(num_tasks);
        generate_recommendations(num_tasks);

        // Execute Intelligent Engine Diagnoses Sequence
        intelligent_engine_run();
    }
}

void compile_system_metrics(void)
{
    uint32_t system_uptime_sec = (xTaskGetTickCount() * portTICK_PERIOD_MS) / 1000;

    if (xSemaphoreTake(system_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        ESP_LOGI(TAG, "[CORE 1] System Status: NOMINAL | Uptime: %lu seconds", system_uptime_sec);
        xSemaphoreGive(system_mutex);
    }
}