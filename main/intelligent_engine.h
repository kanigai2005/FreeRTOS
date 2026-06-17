#ifndef INTELLIGENT_ENGINE_H
#define INTELLIGENT_ENGINE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void intelligent_engine_run(void);
void log_event(const char *type, const char *message);

#endif