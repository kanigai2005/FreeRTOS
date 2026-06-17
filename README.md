# ProtoOS – Intelligent Self-Healing RTOS Monitoring Framework for ESP32

## Overview

ProtoOS is an intelligent operating-system monitoring and self-healing framework built on top of FreeRTOS for ESP32. The system continuously analyzes CPU utilization, task health, memory usage, starvation conditions, and stack behavior while automatically taking corrective actions when abnormal conditions are detected.

The framework combines:

* Real-time system telemetry
* Memory protection mechanisms
* Task recovery and watchdog services
* Predictive analytics
* Intelligent resource management
* Self-healing architecture

The goal is to create a resilient embedded operating environment capable of detecting failures, predicting resource exhaustion, and recovering from abnormal runtime conditions without manual intervention.

---

# Features

## System Health Monitoring

Continuously collects and analyzes:

* CPU utilization
* Task runtime statistics
* Task states
* Active task count
* CPU starvation conditions
* CPU hogging tasks
* System uptime

### Outputs

* CPU Load Percentage
* Top CPU-consuming tasks
* Starvation alerts
* CPU hog detection
* Stability score calculation

---

## Intelligent Engine

The Intelligent Engine acts as the AI-inspired decision layer of ProtoOS.

### Responsibilities

#### Memory Leak Prediction

Analyzes heap history and predicts:

* Memory leak trends
* Estimated exhaustion cycles
* Future heap availability

#### CPU Anomaly Detection

Detects:

* Sudden CPU spikes
* Unexpected runtime increases
* Resource abuse

#### Dynamic Priority Management

Automatically:

* Lowers priorities of CPU-hogging tasks
* Boosts priorities of starved tasks

#### Stack Overflow Prediction

Tracks historical stack consumption and predicts:

* Potential stack exhaustion
* Future stack overflow events

---

## Storage Guard Engine

Acts as the memory defense subsystem.

### Functions

* Monitors internal heap memory
* Detects critically low memory conditions
* Tracks memory allocation trends
* Identifies unhealthy tasks
* Performs cleanup operations

### Automatic Recovery

When memory becomes critical:

* Resource reallocation begins
* Unhealthy tasks are identified
* Recoverable tasks are restarted
* Stability score is recalculated

---

## Watchdog Recovery Manager

Provides fault tolerance.

### Features

* Task heartbeat tracking
* Frozen task detection
* Automatic task restart
* Recovery registration table
* Runtime task recreation

### Recovery Workflow

1. Task stops responding
2. Watchdog timeout occurs
3. Recovery Manager identifies task
4. Existing task is removed
5. Fresh task instance is launched
6. Monitoring resumes automatically

---

## Historical Trend Analyzer

Maintains system history for diagnostics.

### Stores

* Heap history
* CPU load history
* Active task history
* Stability score history

### Reports

Generates periodic reports showing:

* Memory trends
* CPU trends
* Task growth patterns
* Current system health

---

# Architecture

```text
                +----------------------+
                |      ProtoOS         |
                +----------+-----------+
                           |
        +------------------+------------------+
        |                                     |
        v                                     v

+-------------------+            +-------------------+
| System Health     |            | Storage Guard     |
| Monitoring Core   |            | Memory Defender   |
+---------+---------+            +---------+---------+
          |                                |
          |                                |
          v                                v

+-------------------+            +-------------------+
| Intelligent       |            | Watchdog Manager  |
| Analysis Engine   |            | Self-Healing Core |
+---------+---------+            +---------+---------+
          |                                |
          +---------------+----------------+
                          |
                          v

               +----------------------+
               | Trend Analyzer       |
               | Historical Reports   |
               +----------------------+
```

---

# Stability Score Model

ProtoOS maintains a dynamic system stability score.

Initial Score:

```text
100%
```

Reductions occur when:

### CPU Hogging

```text
High CPU Consumption
→ Stability Reduction
```

### Task Starvation

```text
Low CPU Allocation
→ Stability Reduction
```

### Excessive Tasks

```text
Too Many Active Tasks
→ Stability Reduction
```

### Low Heap Conditions

```text
Memory Pressure
→ Stability Reduction
```

The final stability score provides an overall health indicator for the system.

---

# Project Structure

```text
ProtoOS
│
├── main.c
│
├── system_health.c
├── system_health.h
│
├── storage_guard.c
├── storage_guard.h
│
├── intelligent_engine.c
├── intelligent_engine.h
│
├── watchdog_manager.c
├── watchdog_manager.h
│
└── README.md
```

---

# Core Tasks

| Task               | Purpose                        | Core   |
| ------------------ | ------------------------------ | ------ |
| StorageGuardEngine | Memory protection and cleanup  | Core 0 |
| TelemetryEngine    | System monitoring and analysis | Core 1 |
| TrendAnalyzer      | Historical reporting           | Any    |
| WatchdogMonitor    | Task recovery monitoring       | Any    |

---

# Technologies Used

### Platform

* ESP32
* ESP-IDF

### RTOS

* FreeRTOS

### Programming Language

* C

### Monitoring APIs

* uxTaskGetSystemState()
* uxTaskGetNumberOfTasks()
* heap_caps_get_free_size()
* xTaskGetTickCount()

---

# Build Requirements

### ESP-IDF

Version 5.x or later recommended.

### Enable Runtime Statistics

In menuconfig:

```text
Component Config
    → FreeRTOS
        → Enable Trace Facility
        → Enable Runtime Statistics
```

Equivalent configuration:

```text
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
```

---

# Sample Output

```text
=================== OS HISTORICAL TREND REPORT ===================

Last Heap Records (Bytes):
[ 248192 247880 247552 247216 ...]

Last Active Task Counts:
[ 12 12 13 13 ...]

Last CPU Load Curves (%):
[ 43.2% 41.7% 44.1% ...]

Current Live Stability Core:
91.25%

==================================================================
```

---

# Future Enhancements

* Machine Learning based anomaly prediction
* Cloud telemetry dashboard
* MQTT integration
* Web-based monitoring console
* OTA recovery support
* Automated memory optimization
* Task behavior learning engine
* Predictive fault analytics

---

# Authors

Developed as an Advanced Embedded Systems and Operating System Intelligence Project using ESP32 and FreeRTOS.

## Keywords

FreeRTOS, ESP32, Self-Healing Systems, Embedded Monitoring, RTOS Analytics, Watchdog Recovery, Predictive Maintenance, Memory Management, Embedded Intelligence, System Telemetry
