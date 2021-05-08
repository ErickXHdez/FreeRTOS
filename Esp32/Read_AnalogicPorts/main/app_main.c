#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "sdkconfig.h"
//#include "printf-stdarg.c"

#define SIZE_COLA 20
#define SIZE_DATA sizeof(float)

#define ADC1_TEST_CHANNEL 4//Pin 32
#define ADC2_TEST_CHANNEL 5//Pin 33
#define ADC3_TEST_CHANNEL 6//Pin 33
#define ADC4_TEST_CHANNEL 7//Pin 33

xQueueHandle ReadData;
const TickType_t xDelay100ms = pdMS_TO_TICKS(1000);

void analogConfig();
void read_AnalogicPorts(void *arg);
void write_AnalogicPort(void *arg);
void convertData(void *arg);

void app_main(void){

    ReadData = xQueueCreate(SIZE_COLA,SIZE_DATA);
    if(ReadData==NULL)
        printf("No se pudo crear la cola ReadData\n");
    else{
        xTaskCreate(read_AnalogicPorts,"readPorts",2048, NULL, 5, NULL);
        xTaskCreate(write_AnalogicPort,"writePorts",2048, NULL, 2, NULL);
        //xTaskCreate(convertData,"convertData",2048, NULL, 2, NULL);
        //vTaskStartScheduler();
    }
    
    
}

void analogConfig(){
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_TEST_CHANNEL,ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC2_TEST_CHANNEL,ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC3_TEST_CHANNEL,ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC4_TEST_CHANNEL,ADC_ATTEN_11db);
    printf("ADs configurados\n");
    
}

void read_AnalogicPorts(void *arg){
    
    analogConfig();

    TickType_t xLastWakeUpTime;

    int analog_port[4]={ADC1_TEST_CHANNEL,ADC2_TEST_CHANNEL,ADC3_TEST_CHANNEL,ADC4_TEST_CHANNEL};
    float valorAD[4];

    while (1){
        for (size_t i = 0; i < 4; i++)
            valorAD[i] = adc1_get_raw(analog_port[i])*3.3/4095;

        printf("Valor leído 1: %f\n",valorAD[0]);
        //printf("Valor leído 2: %f\n",valorAD[1]);
        //printf("Valor leído 3: %f\n",valorAD[2]);
        //printf("Valor leído 4: %f\n",valorAD[3]);

        if(xQueueSendToBack( ReadData, &valorAD, 0 )!=pdTRUE){
            printf("No se pudo mandar el dato\n");    
        }

        vTaskDelayUntil(&xLastWakeUpTime, xDelay100ms);
    }
}

void write_AnalogicPort(void *arg){
    TickType_t xLastWakeUpTime;
    float valorAD[4];
    float voltage_needed=1.0;

    while (1){
        if( xQueueReceive( ReadData, &valorAD, 100 ) == pdPASS ){
            dac_output_enable(DAC_CHANNEL_1);//GPIO25(ESP32)
            dac_output_voltage(DAC_CHANNEL_1, valorAD[0]*255/3.3);//voltage to approx 0.78 of VDD_A voltage (VDD * 200 / 255). For VDD_A 3.3V, this is 2.59V
        }
        else{
            printf("No se han recibido datos\n");
        }
        //vTaskDelayUntil(&xLastWakeUpTime, xDelay100ms);
    }
}

void convertData(void *arg){
    
    int valorAD[4];

    while (1){
        if( xQueueReceive( ReadData, &valorAD, 100 ) == pdPASS ){
            printf("Dato %d\n",valorAD[0]);
        }
        else{
            printf("No se han recibido datos\n");
        }
        
    }
}
