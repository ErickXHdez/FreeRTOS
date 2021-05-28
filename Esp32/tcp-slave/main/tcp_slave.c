#include <stdio.h>

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mdns.h"
#include "nvs_flash.h"

#include "protocol_examples_common.h"
#include "mbcontroller.h"       
#include "modbus_params.h"    

#define MB_TCP_PORT_NUMBER      (CONFIG_FMB_TCP_PORT_DEFAULT)
#define MB_MDNS_PORT            (502)


#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) >> 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) >> 1))
#define MB_REG_DISCRETE_INPUT_START         (0x0000)
#define MB_REG_COILS_START                  (0x0000)
#define MB_REG_INPUT_START_AREA0            (INPUT_OFFSET(input_data0)) 
#define MB_REG_INPUT_START_AREA1            (INPUT_OFFSET(input_data4)) 
#define MB_REG_HOLDING_START_AREA0          (HOLD_OFFSET(holding_data0))
#define MB_REG_HOLDING_START_AREA1          (HOLD_OFFSET(holding_data4))

#define MB_PAR_INFO_GET_TOUT                (10) 
#define MB_CHAN_DATA_MAX_VAL                (50)
#define MB_CHAN_DATA_OFFSET                 (1.1f)

#define MB_READ_MASK                        (MB_EVENT_INPUT_REG_RD \
                                                | MB_EVENT_HOLDING_REG_RD \
                                                | MB_EVENT_DISCRETE_RD \
                                                | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK                       (MB_EVENT_HOLDING_REG_WR \
                                                | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK                  (MB_READ_MASK | MB_WRITE_MASK)

#define SLAVE_TAG "SLAVE_TEST"

static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

static void setup_reg_data(void){
    // Se escribe el estado inicial de los registros
    discrete_reg_params.discrete_input0 = 1;
    discrete_reg_params.discrete_input1 = 0;
    discrete_reg_params.discrete_input2 = 1;
    discrete_reg_params.discrete_input3 = 0;
    discrete_reg_params.discrete_input4 = 1;
    discrete_reg_params.discrete_input5 = 0;
    discrete_reg_params.discrete_input6 = 1;
    discrete_reg_params.discrete_input7 = 0;
    
    holding_reg_params.holding_data0 = 1.10;
    holding_reg_params.holding_data1 = 2.20;
    holding_reg_params.holding_data2 = 3.30;
    holding_reg_params.holding_data3 = 4.40;
    holding_reg_params.holding_data4 = 5.50;
    holding_reg_params.holding_data5 = 6.60;
    holding_reg_params.holding_data6 = 7.70;
    holding_reg_params.holding_data7 = 8.80;

    coil_reg_params.coils_port0 = 0x55;
    coil_reg_params.coils_port1 = 0xAA;

    input_reg_params.input_data0 = 1.00;
    input_reg_params.input_data1 = 2.00;
    input_reg_params.input_data2 = 3.00;
    input_reg_params.input_data3 = 4.00;
    input_reg_params.input_data4 = 5.00;
    input_reg_params.input_data5 = 6.00;
    input_reg_params.input_data6 = 7.00;
    input_reg_params.input_data7 = 8.50;
}

static void init_registers(void){
    mb_register_area_descriptor_t reg_area; // Area descriptiva de los registros de Modbus 
    
    //Holding register
    reg_area.type = MB_PARAM_HOLDING; // Tipo de area
    reg_area.start_offset = MB_REG_HOLDING_START_AREA0; // Offset de area de registro
    reg_area.address = (void*)&holding_reg_params.holding_data0; // Puntero a instancia de registro
    reg_area.size = sizeof(float) << 2; // Tamaño de registro
    ESP_LOGI(SLAVE_TAG, "registros holding size area0 %f",(float)reg_area.size);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    reg_area.type = MB_PARAM_HOLDING; 
    reg_area.start_offset = MB_REG_HOLDING_START_AREA1; 
    reg_area.address = (void*)&holding_reg_params.holding_data4; 
    reg_area.size = sizeof(float) << 2; 
    ESP_LOGI(SLAVE_TAG, "registros holding area1 %f",(float)reg_area.size);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //Input Registers 
    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = MB_REG_INPUT_START_AREA0;
    reg_area.address = (void*)&input_reg_params.input_data0;
    reg_area.size = sizeof(float) << 2;
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));
    
    //Input Registers
    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = MB_REG_INPUT_START_AREA1;
    reg_area.address = (void*)&input_reg_params.input_data4;
    reg_area.size = sizeof(float) << 2;
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //Coils register
    reg_area.type = MB_PARAM_COIL;
    reg_area.start_offset = MB_REG_COILS_START;
    reg_area.address = (void*)&coil_reg_params;
    reg_area.size = sizeof(coil_reg_params);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //Discrete Inputs register 
    reg_area.type = MB_PARAM_DISCRETE;
    reg_area.start_offset = MB_REG_DISCRETE_INPUT_START;
    reg_area.address = (void*)&discrete_reg_params;
    reg_area.size = sizeof(discrete_reg_params);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));
}

static void init_Wifi(void){
    ESP_ERROR_CHECK(example_connect());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

void init_Modbus(void){
    esp_log_level_set(SLAVE_TAG, ESP_LOG_INFO);
    void* mbc_slave_handler = NULL;

    ESP_ERROR_CHECK(mbc_slave_init_tcp(&mbc_slave_handler)); // Inicializando el controlador Modbus
    
    // Inicializando los registros de información de modbus
    mb_communication_info_t comm_info = { 0 };
    comm_info.ip_port = MB_TCP_PORT_NUMBER;
    comm_info.ip_addr_type = MB_IPV4;
    comm_info.ip_mode = MB_MODE_TCP;
    comm_info.ip_addr = NULL;
    comm_info.ip_netif_ptr = (void*)get_example_netif();

    // Configura el esclavo con los parametros anteriores
    ESP_ERROR_CHECK(mbc_slave_setup((void*)&comm_info));

    //Inicializar registros 
    init_registers();
    
    //Escribe el estado inicial de los registros
    setup_reg_data(); 

    // Inicia el controlador modbus y el stack
    ESP_ERROR_CHECK(mbc_slave_start());
    ESP_LOGI(SLAVE_TAG, "Modbus slave stack initialized.");
    ESP_LOGI(SLAVE_TAG, "Start modbus test...");
}

void mb_Task(void* arg){
    mb_param_info_t reg_info;

    while(1) {
        ESP_LOGI(SLAVE_TAG, "Esperar evento");
        //Con esta funcion se espera que ocurra el evento que solicita el maestro
        //Queda detenido todo esperando el evento
        mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);
        //Esta variables es para determinar si esta en el evento de Lectura o Escritura
        const char* rw_str = (event & MB_READ_MASK) ? "READ" : "WRITE";
        
        //A partir de aquí analizamos cada evento y las acciones que se puedan realizar        
        if(event & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD)) {
            // Tomamos las características de la transferencia
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "HOLDING %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                    rw_str,
                    (uint32_t)reg_info.time_stamp,
                    (uint32_t)reg_info.mb_offset,
                    (uint32_t)reg_info.type,
                    (uint32_t)reg_info.address,
                    (uint32_t)reg_info.size);
        } 
        else if (event & MB_EVENT_INPUT_REG_RD) {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "INPUT READ (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                    (uint32_t)reg_info.time_stamp,
                    (uint32_t)reg_info.mb_offset,
                    (uint32_t)reg_info.type,
                    (uint32_t)reg_info.address,
                    (uint32_t)reg_info.size);
        } 
        else if (event & MB_EVENT_DISCRETE_RD) {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "DISCRETE READ (%u us): ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                                (uint32_t)reg_info.time_stamp,
                                (uint32_t)reg_info.mb_offset,
                                (uint32_t)reg_info.type,
                                (uint32_t)reg_info.address,
                                (uint32_t)reg_info.size);
        } 
        else if (event & (MB_EVENT_COILS_RD | MB_EVENT_COILS_WR)) {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "COILS %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                                rw_str,
                                (uint32_t)reg_info.time_stamp,
                                (uint32_t)reg_info.mb_offset,
                                (uint32_t)reg_info.type,
                                (uint32_t)reg_info.address,
                                (uint32_t)reg_info.size);
            if (coil_reg_params.coils_port1 == 0xFF) break;
        }
    }

    //Destruye el controlador modbus si descubre una alarma
    ESP_LOGI(SLAVE_TAG,"Modbus controller destroyed.");
    vTaskDelay(100);
    ESP_ERROR_CHECK(mbc_slave_destroy());
    #if CONFIG_MB_MDNS_IP_RESOLVER
        mdns_free();
    #endif
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
  
    init_Wifi();

    init_Modbus();
    
    xTaskCreatePinnedToCore(&mb_Task, "intercambio datos modbus", 1024 * 4, NULL, 5, NULL,0);
}
