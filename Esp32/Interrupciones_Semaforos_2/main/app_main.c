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

SemaphoreHandle_t xSemaphore = NULL;

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
    while (1)
    {
        printf("LLego aki\n");
        if (xSemaphoreTake(xSemaphore,portMAX_DELAY)==pdTRUE)
        {
            printf("Pulsador Presionado\n");
        } 
    }
    
}

void app_main(void)
{
    
    xSemaphore= xSemaphoreCreateBinary();
    init_GPIO();
    xTaskCreate(&task_pulsador,"pulsador",2048, NULL, 5, NULL);
    
}