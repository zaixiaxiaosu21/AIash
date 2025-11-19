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
  task_runner_handle_t  task_runner=  task_runner_init();
  task_runner_handle_t  task_runner2={0};
  task_runner2=task_runner_init();
    task_runner_add_task(task_runner, task1, "Hello from task_runner");
    task_runner_add_task(task_runner2, task1, "Hello from task_runner2");
    task_runner_start(task_runner);
    task_runner_start(task_runner2);
    vTaskDelay(pdMS_TO_TICKS(4000)); // 运行4秒
    task_runner_stop(task_runner);
     task_runner_stop(task_runner2);
    task_runner_deinit(task_runner);
    task_runner_deinit(task_runner2);
   
}