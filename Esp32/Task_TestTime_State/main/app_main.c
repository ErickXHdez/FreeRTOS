
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
    xTaskCreate(task2,"task2",2048, NULL, 5, &TaskHandle2);
}

void task1(void *arg){
    int c=0;
    while (1)
    {
        c++;
        printf("Ejecutando Task1\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(c==3){
            vTaskSuspend(TaskHandle2);    
            printf("Task2 fue suspendida\n");
        }
        if(c==5){
            vTaskResume(TaskHandle2);
            printf("Task2 iniciada\n");
        }
        if(c==7){
            vTaskDelete(TaskHandle2);    
            printf("Task2 fue eliminada\n");
        }
    }

}
void task2(void *arg){
    for (int i = 0; i <= 5; i++)
    {
        printf("Ejecutando Task2\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }     
}
