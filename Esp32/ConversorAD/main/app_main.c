#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "sdkconfig.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define PIN_PULSADOR 13

#define BLINK_GPIO 2
#define TAM_COLA 20
#define TAM_MSG 7
#define ADC1_TEST_CHANNEL 4//Pin 32

xQueueHandle Cola_Msg;

void ADC1_task(void *pvParameter);


void app_main(void)
{
    Cola_Msg = xQueueCreate(TAM_COLA,TAM_MSG);
    
    xTaskCreate(&ADC1_task, "ADC1_task", 2048, NULL, 3, NULL);//Crear tarea

}

void ADC1_task(void *pvParameter)
{
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_TEST_CHANNEL,ADC_ATTEN_11db);
    int valor;
    while (1)
    {
        valor= adc1_get_raw(ADC1_TEST_CHANNEL);
        //valorADC=adc1_get_voltage(ADC1_TEST_CHANNEL);
        printf("Valor leído: %d\n",valor);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}