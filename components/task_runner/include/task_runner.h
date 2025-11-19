#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
typedef  void *task_runner_handle_t;
task_runner_handle_t task_runner_init();
void task_runner_deinit(task_runner_handle_t task_runner);
void task_runner_start(task_runner_handle_t task_runner);

void task_runner_stop(task_runner_handle_t task_runner);

void task_runner_add_task(task_runner_handle_t task_runner, void (*task)(void *), void *arg);