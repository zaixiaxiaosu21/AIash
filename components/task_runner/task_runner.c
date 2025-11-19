#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task_runner.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "Task_runner"




static void task_runner_task(void *arg)
{
    task_runner_t *task_runner= (task_runner_t *)arg;
    while (task_runner->task_runner_running) {
        // 执行所有添加的任务
        for (int i = 0; i < task_runner->task_runner_task_count; i++) {
            task_item_t *item = &task_runner->task_runner_tasks[i];
           if (item->task == NULL) 
           {
             continue;
           }
           item->task(item->arg); // 执行任务
          
        }
         vTaskDelay(pdMS_TO_TICKS(1000)); // 延时1s
}       
        task_runner->task_runner_task_handle = NULL; // 任务结束，设置句柄为NULL
        vTaskDelete(NULL); // 删除任务
}

task_runner_handle_t task_runner_init()
{
        task_runner_t *task_runner = (task_runner_t *)malloc(sizeof(task_runner_t));
        if (task_runner == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for task runner");
            return NULL;
        }
        memset(task_runner, 0, sizeof(task_runner_t));
        
        task_runner->task_runner_tasks = (task_item_t *)malloc(sizeof(task_item_t) * 10); // 初始容量为10
        if (task_runner->task_runner_tasks == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for task items");
            free(task_runner);
            return NULL;
        }
        task_runner->task_runner_task_capacity = 10; // 初始容量
        task_runner->task_runner_task_count = 0; // 初始任务数量为0
        return task_runner;
}

void task_runner_deinit(task_runner_handle_t handle)
{
     task_runner_t *task_runner= (task_runner_t *)handle; //将句柄转换为task_runner_t指针
     if (task_runner == NULL)
     {
         ESP_LOGE(TAG, "Invalid task runner handle");
         return;
     }
     
     free(task_runner->task_runner_tasks); //释放任务数组内存
     free(task_runner); //释放task_runner结构体内存
}

void task_runner_start(task_runner_handle_t handle)
{    
    task_runner_t *task_runner= (task_runner_t *)handle; //将句柄转换为task_runner_t指针
    task_runner->task_runner_running = true; //设置运行状态为true

    xTaskCreate(task_runner_task, "task_runner_task", 4096, task_runner, 5, &task_runner->task_runner_task_handle);
    
}

void task_runner_stop(task_runner_handle_t handle)
{
    task_runner_t *task_runner= (task_runner_t *)handle; //将句柄转换为task_runner_t指针
    task_runner->task_runner_running = false; //设置运行状态为false

    // 等待任务结束
    while (task_runner->task_runner_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_runner_add_task(task_runner_handle_t handle, void (*task)(void *), void *arg)
{
    task_runner_t *task_runner= (task_runner_t *)handle; //将句柄转换为task_runner_t指针
    if (task_runner == NULL)
    {
        ESP_LOGE(TAG, "Invalid task runner handle");
        return;
    }
    
    if (task_runner->task_runner_task_count >= task_runner->task_runner_task_capacity) {
        // 如果任务数量超过容量，则扩展容量
        task_runner->task_runner_task_capacity *= 2;
      task_item_t * new_task  =(task_item_t *)realloc(task_runner->task_runner_tasks, sizeof(task_item_t) * task_runner->task_runner_task_capacity);
      if (new_task == NULL)
      {
        ESP_LOGE(TAG, "Failed to reallocate memory for task items");
        return;
      }
        task_runner->task_runner_tasks = new_task;
        

    }
    //执行正常逻辑
    task_runner->task_runner_tasks[task_runner->task_runner_task_count].task = task;
    task_runner->task_runner_tasks[task_runner->task_runner_task_count].arg = arg;
    task_runner->task_runner_task_count++;
}
