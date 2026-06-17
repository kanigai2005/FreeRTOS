#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct
{
    TaskHandle_t handle;
    char name[32];
    uint32_t heartbeat;
    TickType_t last_seen;
} WatchdogEntry;

typedef struct
{
    char name[32];
    TaskFunction_t function;
    uint16_t stack;
    UBaseType_t priority;
    BaseType_t core;
} RecoveryEntry;

void watchdog_register_task(TaskHandle_t handle, const char *name);
void watchdog_kick(TaskHandle_t handle);
void watchdog_monitor_task(void *pvParameters);
void register_recoverable_task(const char *name, TaskFunction_t func, uint16_t stack, UBaseType_t prio, BaseType_t core);
void recover_task(const char *name);

#endif // WATCHDOG_MANAGER_H