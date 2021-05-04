#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

TaskHandle_t TaskHandle1 = NULL;
TaskHandle_t TaskHandle2 = NULL;

void task1(void *arg);
void task2(void *arg);

void app_main(void){
    xTaskCreate(task1,"task1",2048, NULL, 5, &TaskHandle1);
    xTaskCreatePinnedToCore(task2,"task2",2048, NULL, 5, &TaskHandle2, 1);//Anclar la ejucucion de la tarea al segundo núcleo
}

void task1(void *arg){
    int c=0;
    while (1)
    {
        /*Estado de tareas expresadas en números
        Running = 0
        Ready = 1
        Blocked = 2
        Suspended = 3
        Deleted = 4
        Invalid = 5
        */
        c=xTaskGetTickCount();
        printf("Ejecutando %s, tiempo %d\n", pcTaskGetTaskName(TaskHandle1), c);
        if(c==300){
            vTaskSuspend(TaskHandle2);    
            printf("Task2 fue suspendida %d\n", eTaskGetState(TaskHandle2));
        }
        if(c==500){
            vTaskResume(TaskHandle2);
            printf("Task2 iniciada %d\n", eTaskGetState(TaskHandle2));
        }
        if(c==700){
            vTaskDelete(TaskHandle2);    
            printf("Task2 fue eliminada %d\n", eTaskGetState(TaskHandle2));
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task2(void *arg){
    for (int i = 0; i <= 10; i++)
    {
        printf("Ejecutando Task2\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }     
}
