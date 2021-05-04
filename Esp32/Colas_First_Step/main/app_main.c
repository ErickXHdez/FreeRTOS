#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define BLINK_GPIO 2
#define TAM_COLA 20
#define TAM_MSG 7

xQueueHandle Cola_Msg;

void blink_task(void *pvParameter);
void print_task(void *pvParameter);

void app_main(void)
{
    Cola_Msg = xQueueCreate(TAM_COLA,TAM_MSG);
    
    xTaskCreate(&blink_task, "blink_task", 2048, NULL, 5, NULL);//Crear tarea
    xTaskCreate(&print_task, "print_task", 2048, NULL, 1, NULL);//Crear tarea

}

void blink_task(void *pvParameter)
{
    int led_state=0;
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while(1)
    {
        printf("Mandar\n");
        if(led_state==0){
            led_state = 1;
            gpio_set_level(BLINK_GPIO, led_state);
            //printf("led encendido\n");
        }else{
            led_state = 0;
            gpio_set_level(BLINK_GPIO, led_state);
            //printf("led apagado\n");
        }
        if(xQueueSendToBack(Cola_Msg, &led_state,2000/portTICK_RATE_MS)!=pdTRUE){
            printf("error escribiendo");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);

}

void print_task(void *pvParameter){
    
    int led_state_recibido=0;

    while(1){
        printf("Leer\n");
        if(xQueueReceive(Cola_Msg,&led_state_recibido,2000/portTICK_RATE_MS)==pdTRUE){
            if (led_state_recibido==1)
                printf("led encendido\n");
            else
                printf("led apagado\n");
        }else
            printf("error leyendo\n");
    }
    vTaskDelete(NULL);
    
}