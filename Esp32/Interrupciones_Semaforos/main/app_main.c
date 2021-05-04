#include <stdio.h>
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
    //printf("p");
    xSemaphoreGiveFromISR(xSemaphore,NULL);
}
void init_GPIO(){
    //Configuro el Pin_Pulsador como un pin GPIO
    gpio_pad_select_gpio(PIN_PULSADOR);
    //Selecciono el pin de entrada PIN_PULSADOR como un pin de entrada
    gpio_set_direction(PIN_PULSADOR, GPIO_MODE_INPUT);
    gpio_pullup_en(PIN_PULSADOR);
    //Instala el servicio ISR con la configuracion por defecto
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //Habilito interrupcion por flanco descendente
    gpio_set_intr_type(PIN_PULSADOR, GPIO_INTR_NEGEDGE);
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
    init_GPIO();
    xSemaphore= xSemaphoreCreateBinary();
    xTaskCreate(task_pulsador,"pulsador",2048, NULL, 5, NULL);
    gpio_isr_handler_add(PIN_PULSADOR,pulsador_isr_handler,(void*)PIN_PULSADOR);
}
