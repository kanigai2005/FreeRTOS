#ifndef SYSTEM_HEALTH_H
#define SYSTEM_HEALTH_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// System Health APIs
void init_system_health(void);
void compile_system_metrics(void);
void calculate_system_load(void);
void state_task(void);

#endif // SYSTEM_HEALTH_H