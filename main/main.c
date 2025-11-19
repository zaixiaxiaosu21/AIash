#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task_runner.h"

void task1(void *arg)
{
    char *str = (char *)arg;
    printf("Task1: %s\n", str);
}
void app_main(void)
{
    
   task_runner_handle_t  task_runner= task_runner_init();
   task_runner_add_task(task_runner, task1, "Hello World");
   task_runner_add_task(task_runner, task1, "Hello ESP32");
   task_runner_start(task_runner);
   vTaskDelay(pdMS_TO_TICKS(5000));
   task_runner_stop(task_runner);
   task_runner_deinit(task_runner);
    
}