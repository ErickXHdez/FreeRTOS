#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define PIN_PULSADOR 13

#define BLINK_GPIO 2
#define TAM_COLA 20
#define TAM_MSG 7

SemaphoreHandle_t xSemaphore = NULL;
xQueueHandle Cola_Msg;

void IRAM_ATTR pulsador_isr_handler(void* arg);
void init_GPIO();
void blink_task(void *pvParameter);
void task_pulsador(void *arg);

void app_main(void)
{
    Cola_Msg = xQueueCreate(TAM_COLA,TAM_MSG);

    xSemaphore= xSemaphoreCreateBinary();

    init_GPIO();

    xTaskCreate(&task_pulsador,"pulsador",2048, NULL, 5, NULL);
    xTaskCreate(&blink_task, "blink_task", 2048, NULL, 3, NULL);//Crear tarea
    

}


void IRAM_ATTR pulsador_isr_handler(void* arg){
    xSemaphoreGiveFromISR(xSemaphore,NULL);
}

void init_GPIO(){
    
    gpio_config_t intrconfig;
    intrconfig.pin_bit_mask = (1<<PIN_PULSADOR);    //Seteando el pin a utilizar
    intrconfig.mode = GPIO_MODE_INPUT;              //Pin de entrada o salida
    intrconfig.pull_up_en = 1;                      
    intrconfig.pull_down_en = 0;
    intrconfig.intr_type = GPIO_INTR_NEGEDGE;       //Flanco de bajada
	gpio_config(&intrconfig);
    
    //Instala el servicio ISR con la configuracion por defecto
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //Instala el servicio GPIO ISR handler
    gpio_isr_handler_add(PIN_PULSADOR,pulsador_isr_handler,(void*)PIN_PULSADOR);
}

void task_pulsador(void *arg){
    int pulsador_state=0;
    while (1)
    {
        //printf("LLego aki\n");
        if (xSemaphoreTake(xSemaphore,portMAX_DELAY)==pdTRUE)
        {
            printf("Pulsador Presionado\n");
            
            if(pulsador_state==0)
                pulsador_state = 1;                
            else
                pulsador_state = 0;
                
            if (xQueueSendToBack(Cola_Msg, &pulsador_state,2000/portTICK_RATE_MS)!=pdTRUE)
            {
                printf("error escribiendo");
            }
        } 
    }
}

void blink_task(void *pvParameter)
{
    int led_state_recibido=0;
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while(1)
    {   
        if(xQueueReceive(Cola_Msg,&led_state_recibido,2000/portTICK_RATE_MS)==pdTRUE){
            if (led_state_recibido==1){
                printf("led encendido\n");
                gpio_set_level(BLINK_GPIO, led_state_recibido);
            }else{
                printf("led apagado\n");
                gpio_set_level(BLINK_GPIO, led_state_recibido);
            }
        }else
            printf("No hay dato en la cola\n");     
        
       //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
