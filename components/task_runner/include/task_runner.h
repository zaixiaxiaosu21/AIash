#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
typedef struct {
    void (*task)(void *);  // 任务函数指针
    void *arg;             // 任务参数
} task_item_t;
typedef struct task_runner {
    TaskHandle_t task_runner_task_handle;    // FreeRTOS 任务句柄
    bool task_runner_running;                // 运行状态标志
    
    task_item_t *task_runner_tasks;          // 任务数组
    int task_runner_task_capacity;           // 数组容量
    int task_runner_task_count;             // 当前任务数量
} task_runner_t;
typedef  task_runner_t* task_runner_handle_t;
task_runner_handle_t task_runner_init();
void task_runner_deinit(task_runner_handle_t task_runner);
void task_runner_start(task_runner_handle_t task_runner);

void task_runner_stop(task_runner_handle_t task_runner);

void task_runner_add_task(task_runner_handle_t task_runner, void (*task)(void *), void *arg);