#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task_runner.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "Task_runner"

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
static void task_runner_task(void *arg)
{
     task_runner_t *task_runner = (task_runner_t *)arg;
    while (task_runner->task_runner_running){
        for (size_t i = 0; i < task_runner->task_runner_task_count; i++)
        {
            task_item_t *item = &task_runner->task_runner_tasks[i];
            if (item->task == NULL)
            {
                continue;
            }
            item->task(item->arg);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    task_runner->task_runner_task_handle=NULL;
    vTaskDelete(NULL);

}
task_runner_handle_t task_runner_init()
{
    task_runner_t *task_runner = malloc(sizeof(task_runner_t));
    if (!task_runner ) {
        return NULL;
        ESP_LOGE("task_runner", "malloc failed");
    }
    memset(task_runner, 0, sizeof(task_runner_t));
    task_runner->task_runner_tasks = malloc(sizeof(task_item_t) * 10); // 初始容量为10
    if (!task_runner->task_runner_tasks) {
        ESP_LOGE(TAG, "Failed to allocate memory for tasks");
        free(task_runner);

        return NULL;
    }
    task_runner->task_runner_task_capacity = 10;
    task_runner->task_runner_task_count = 0;
    return task_runner;// 返回任务管理器句柄
}

void task_runner_deinit(task_runner_handle_t handle)
{
    task_runner_t *task_runner = (task_runner_t *)handle; // 将句柄转换为任务管理器结构体指针
    if (!task_runner)
    {
        return;
    }
    free(task_runner->task_runner_tasks);
    free(task_runner);
    
}

void task_runner_start(task_runner_handle_t handle)
{    
    task_runner_t *task_runner = (task_runner_t *)handle;
     if (task_runner->task_runner_task_handle != NULL)
    {
        ESP_LOGW(TAG, "Task runner already started");
        return;
    }
    task_runner->task_runner_running = true;
    xTaskCreate(&task_runner_task, "task_runner_task", 4096, task_runner, 5, &task_runner->task_runner_task_handle);
    
    
}

void task_runner_stop(task_runner_handle_t handle)
{
    task_runner_t *task_runner = (task_runner_t *)handle;
    task_runner->task_runner_running = false;
    while (task_runner->task_runner_task_handle)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_runner_add_task(task_runner_handle_t handle, void (*task)(void *), void *arg)
{
    task_runner_t *task_runner = (task_runner_t *)handle;
    if (task_runner->task_runner_running)
    {
        ESP_LOGE(TAG, "Cannot add task while task runner is running");
        return;
    }
    if (task_runner->task_runner_task_count >= task_runner->task_runner_task_capacity)
    {
        int new_capacity = task_runner->task_runner_task_capacity * 2;
        task_item_t *new_tasks = realloc(task_runner->task_runner_tasks, new_capacity * sizeof(task_item_t));
        if (!new_tasks){
            ESP_LOGE(TAG, "Failed to allocate memory for tasks");
            return;
        }
        task_runner->task_runner_tasks = new_tasks;
        task_runner->task_runner_task_capacity = new_capacity;
    }
        task_runner->task_runner_tasks[task_runner->task_runner_task_count].task = task;
        task_runner->task_runner_tasks[task_runner->task_runner_task_count].arg = arg;
        task_runner->task_runner_task_count++;
}
