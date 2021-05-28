#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "esp_http_server.h"

#include "mbcontroller.h" // for mbcontroller defines and api

#include "modbus_params.h"      // for modbus parameters structures
#include "esp_modbus_common.h"
#include "mdns.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#define ESP_WIFI_SSID "myssid"
#define ESP_WIFI_PASS "mypassword"
#define ESP_MAXIMUM_RETRY 5

xSemaphoreHandle connectionSemaphore;

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

#define TAM_COLA 20
#define TAM_MSG 7

xQueueHandle Cola_Msg;


#define MB_TCP_PORT_NUMBER      (CONFIG_FMB_TCP_PORT_DEFAULT)


static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) //Este evento esta generado por el ESP-OK de esp_wifi_start()
        esp_wifi_connect();

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP %d", s_retry_num);
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG, "connect to the AP fail");
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ESP_WIFI_SSID,
                 ESP_WIFI_PASS);

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xSemaphoreGive(connectionSemaphore);
        //xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    
    esp_netif_create_default_wifi_sta();
    //ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
               .required = false},
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    /*EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    //xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    // * happened.
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Ready to change data");
        xSemaphoreGive(connectionSemaphore);
    }
    else if (bits & WIFI_FAIL_BIT)
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    else
        ESP_LOGE(TAG, "UNEXPECTED EVENT");

    The event will not be processed after unregister 
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);*/
}

/*static esp_err_t on_url_hit(httpd_req_t *req)
{
    char *message = "hello world";
    httpd_resp_send(req, message, strlen(message));
    return ESP_OK;
}*/

// Set register values into known state
static void setup_reg_data()
{
    // Define initial state of parameters
    // Define initial state of parameters
    discrete_reg_params.discrete_input0 = 1;
    discrete_reg_params.discrete_input1 = 0;
    discrete_reg_params.discrete_input2 = 1;
    discrete_reg_params.discrete_input3 = 0;
    discrete_reg_params.discrete_input4 = 1;
    discrete_reg_params.discrete_input5 = 0;
    discrete_reg_params.discrete_input6 = 1;
    discrete_reg_params.discrete_input7 = 0;

    holding_reg_params.holding_data0 = 1.34;
    holding_reg_params.holding_data1 = 2.56;
    holding_reg_params.holding_data2 = 3.78;
    holding_reg_params.holding_data3 = 4.90;

    holding_reg_params.holding_data4 = 5.67;
    holding_reg_params.holding_data5 = 6.78;
    holding_reg_params.holding_data6 = 7.79;
    holding_reg_params.holding_data7 = 8.80;
    coil_reg_params.coils_port0 = 0x55;
    coil_reg_params.coils_port1 = 0xAA;

    input_reg_params.input_data0 = 1.12;
    input_reg_params.input_data1 = 2.34;
    input_reg_params.input_data2 = 3.56;
    input_reg_params.input_data3 = 4.78;
    input_reg_params.input_data4 = 1.12;
    input_reg_params.input_data5 = 2.34;
    input_reg_params.input_data6 = 3.56;
    input_reg_params.input_data7 = 4.78;
}

void OnConnected(void *data)
{
    int modbus_state = 0;
    while (true)
    {
        ESP_LOGI(TAG, "task OnConnected");
        if (xSemaphoreTake(connectionSemaphore, 20000 / portTICK_RATE_MS) == pdTRUE)
        {
            
            ESP_LOGI(TAG, "y aki habilitas modbus");
            modbus_state = 1;
            
            
            if (xQueueSendToBack(Cola_Msg, &modbus_state, 2000 / portTICK_RATE_MS) != pdTRUE)
            {
                 ESP_LOGI(TAG,"error habilitando modbus");
            }
            vTaskDelete(NULL);
            /*
            httpd_handle_t server = NULL;
            httpd_config_t config = HTTPD_DEFAULT_CONFIG();
            httpd_start(&server,&config);  
            httpd_uri_t first_end_point_config = { 
                .uri = "/",
                .method = HTTP_GET,
                .handler = on_url_hit};
            httpd_register_uri_handler(server, &first_end_point_config);
            */
        }
        else
            ESP_LOGI(TAG, "modbus no habilitado");
        vTaskDelay(1000);
    }
}

void modbus_test(void *arg)
{
    int modbus_state = 0;
    while (1)
    {
        if (xQueuePeek(Cola_Msg, &modbus_state, 2000 / portTICK_RATE_MS) == pdTRUE)
        {
            if (modbus_state == 1)
            {
                ESP_LOGI(TAG,"modbus habilitado");
                
                esp_log_level_set(TAG, ESP_LOG_INFO);

                void* mbc_slave_handler = NULL;
                ESP_ERROR_CHECK(mbc_slave_init_tcp(&mbc_slave_handler));
                mb_param_info_t reg_info; // keeps the Modbus registers access information
                mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure

                mb_communication_info_t comm_info = { 0 };
                comm_info.ip_port = MB_TCP_PORT_NUMBER;
                comm_info.ip_addr_type = MB_IPV4;

                comm_info.ip_mode = MB_MODE_TCP;
                comm_info.ip_addr = NULL;
                comm_info.ip_netif_ptr = (void*)get_example_netif();

                ESP_ERROR_CHECK(mbc_slave_setup((void*)&comm_info));
                vTaskDelay(1000);
                }
            else
                 ESP_LOGI(TAG,"modbus deshabilitado\n");
        }
        else
             ESP_LOGI(TAG,"error leyendo\n");
    vTaskDelay(1000);
    }
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

    connectionSemaphore = xSemaphoreCreateBinary();
    Cola_Msg = xQueueCreate(TAM_COLA,TAM_MSG);
    
    s_wifi_event_group = xEventGroupCreate();

    wifi_init_sta();
    
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    xTaskCreate(&OnConnected, "handel comms", 1024 * 2, NULL, 5, NULL);
    xTaskCreatePinnedToCore(&modbus_test, "modbus", 1024 * 8, NULL, 6, NULL,0);
}