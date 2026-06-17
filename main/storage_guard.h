#ifndef STORAGE_GUARD_H
#define STORAGE_GUARD_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Core Guard Hooks
void init_storage_guard(void);
void run_storage_guard_sweep(void);
void reduce_resource_allocation(void);
void find_and_free_resources(void);

#endif // STORAGE_GUARD_H