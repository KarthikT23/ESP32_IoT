/*
 * http_server.c
 *
 *  Created on: 9 Dec 2024
 *      Author: karthik
 */

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_interface.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_netif_types.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"
#include "freertos/idf_additions.h"
#include "http_parser.h"
#include "http_server.h"
#include "sntp_time_sync.h"
#include "portmacro.h"
#include "tasks_common.h"
#include "unity_internals.h"
#include "wifi_app.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_ota_ops.h"
#include "sys/param.h"
#include <inttypes.h>

#include "DHT22.h"
#include "esp_wifi.h"

// Tag used for ESP Serial console messages
static const char TAG[] = "http_server";

// Global state variables
static int g_wifi_connect_status = NONE;
static int g_fw_update_status = OTA_UPDATE_PENDING;
static bool g_is_local_time_set = false;
static httpd_handle_t http_server_handle = NULL;
static TaskHandle_t task_http_server_monitor = NULL;
static QueueHandle_t http_server_monitor_queue_handle;

// Timer configuration and handle
const esp_timer_create_args_t fw_update_reset_args = {
    .callback = &http_server_fw_update_reset_callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

// Embedded file declarations
extern const uint8_t jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t app_css_start[] asm("_binary_app_css_start");
extern const uint8_t app_css_end[] asm("_binary_app_css_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

// Helper function for serving static files
static esp_err_t serve_static_file(httpd_req_t *req, const char *content_type, 
                                   const uint8_t *start, const uint8_t *end, const char *filename)
{
    ESP_LOGI(TAG, "%s requested", filename);
    httpd_resp_set_type(req, content_type);
    httpd_resp_send(req, (const char *)start, end - start);
    return ESP_OK;
}

// Helper function for sending JSON responses
static esp_err_t send_json_response(httpd_req_t *req, const char *json_data)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_data, strlen(json_data));
    return ESP_OK;
}

static void http_server_fw_update_reset_timer(void)
{
    if(g_fw_update_status == OTA_UPDATE_SUCCESSFUL) {
        ESP_LOGI(TAG, "FW updated successful, starting reset timer");
        ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
        ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
    } else {
        ESP_LOGI(TAG, "FW update unsuccessful");
    }
}

static void http_server_monitor(void *parameter)
{
    http_server_queue_message_t msg;
    
    // Message handling lookup table
    struct {
        http_server_message_e msg_id;
        int *status_var;
        int status_value;
        const char *log_msg;
        bool trigger_reset_timer;
    } msg_handlers[] = {
        {HTTP_MSG_WIFI_CONNECT_INIT, &g_wifi_connect_status, HTTP_WIFI_STATUS_CONNECTING, "WIFI_CONNECT_INIT", false},
        {HTTP_MSG_WIFI_CONNECT_SUCCESS, &g_wifi_connect_status, HTTP_WIFI_STATUS_CONNECT_SUCCESS, "WIFI_CONNECT_SUCCESS", false},
        {HTTP_MSG_WIFI_CONNECT_FAIL, &g_wifi_connect_status, HTTP_WIFI_STATUS_CONNECT_FAILED, "WIFI_CONNECT_FAIL", false},
        {HTTP_MSG_WIFI_USER_DISCONNECT, &g_wifi_connect_status, HTTP_WIFI_STATUS_DISCONNECTED, "WIFI_USER_DISCONNECT", false},
        {HTTP_MSG_OTA_UPDATE_SUCCESSFUL, &g_fw_update_status, OTA_UPDATE_SUCCESSFUL, "OTA_UPDATE_SUCCESSFUL", true},
        {HTTP_MSG_OTA_UPDATE_FAILED, &g_fw_update_status, OTA_UPDATE_FAILED, "OTA_UPDATE_FAILED", false},
        {HTTP_MSG_TIME_SERVICE_INITIALIZED, NULL, 0, "TIME_SERVICE_INITIALIZED", false}
    };
    
    for(;;) {
        if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY)) {
            // Handle TIME_SERVICE_INITIALIZED separately as it uses a boolean
            if (msg.msgID == HTTP_MSG_TIME_SERVICE_INITIALIZED) {
                ESP_LOGI(TAG, "HTTP_MSG_TIME_SERVICE_INITIALIZED");
                g_is_local_time_set = true;
                continue;
            }
            
            // Handle other messages using lookup table
            for (int i = 0; i < sizeof(msg_handlers)/sizeof(msg_handlers[0]); i++) {
                if (msg_handlers[i].msg_id == msg.msgID && msg_handlers[i].status_var != NULL) {
                    ESP_LOGI(TAG, "HTTP_MSG_%s", msg_handlers[i].log_msg);
                    *(msg_handlers[i].status_var) = msg_handlers[i].status_value;
                    
                    if (msg_handlers[i].trigger_reset_timer) {
                        http_server_fw_update_reset_timer();
                    }
                    break;
                }
            }
        }
    }
}

// Static file handlers using the helper function
static esp_err_t http_server_jquery_handler(httpd_req_t *req) {
    return serve_static_file(req, "application/javascript", jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end, "JQuery");
}

static esp_err_t http_server_index_html_handler(httpd_req_t *req) {
    return serve_static_file(req, "text/html", index_html_start, index_html_end, "index.html");
}

static esp_err_t http_server_app_css_handler(httpd_req_t *req) {
    return serve_static_file(req, "text/css", app_css_start, app_css_end, "app.css");
}

static esp_err_t http_server_app_js_handler(httpd_req_t *req) {
    return serve_static_file(req, "application/javascript", app_js_start, app_js_end, "app.js");
}

static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req) {
    return serve_static_file(req, "image/x-icon", favicon_ico_start, favicon_ico_end, "favicon.ico");
}

esp_err_t http_server_OTA_update_handler(httpd_req_t *req)
{
    esp_ota_handle_t ota_handle;
    char ota_buff[1024];
    int content_length = req->content_len;
    int content_received = 0;
    int recv_len;
    bool is_req_body_started = false;
    bool flash_successful = false;
    
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    
    do {
        if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0) {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
                ESP_LOGI(TAG, "Socket Timeout");
                continue;
            }
            ESP_LOGI(TAG, "OTA other error %d", recv_len);
            return ESP_FAIL;
        }
        printf("OTA RX: %d of %d\r", content_received, content_length);
        
        if(!is_req_body_started) {
            is_req_body_started = true;
            char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
            int body_part_len = recv_len - (body_start_p - ota_buff);
            printf("OTA file size: %d\r\n", content_length);
            
            esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
            if (err != ESP_OK) {
                printf("Error with OTA begin, cancelling OTA\r\n");
                return ESP_FAIL;
            } else {
                printf("Writing to partition subtype %d at offset 0x%"PRIx32"\r\n", 
                       update_partition->subtype, update_partition->address);
            }
            esp_ota_write(ota_handle, body_start_p, body_part_len);
            content_received += body_part_len;
        } else {
            esp_ota_write(ota_handle, ota_buff, recv_len);
            content_received += recv_len;
        }
    } while (recv_len > 0 && content_received < content_length);
    
    if (esp_ota_end(ota_handle) == ESP_OK) {
        if (esp_ota_set_boot_partition(update_partition) == ESP_OK) {
            const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "Next boot partition subtype %d at offset 0x%"PRIx32, 
                     boot_partition->subtype, boot_partition->address);
            flash_successful = true;
        } else {
            ESP_LOGI(TAG, "FLASHED ERROR!!");
        }
    } else {
        ESP_LOGI(TAG, "esp_ota_end ERROR!!");
    }
    
    http_server_monitor_send_message(flash_successful ? HTTP_MSG_OTA_UPDATE_SUCCESSFUL : HTTP_MSG_OTA_UPDATE_FAILED);
    return ESP_OK;
}

esp_err_t http_server_OTA_status_handler(httpd_req_t *req)
{
    char otaJSON[100];
    ESP_LOGI(TAG, "OTAstatus requested, current status: %d", g_fw_update_status);
    
    sprintf(otaJSON, "{\"ota_update_status\":%d, \"compile_time\":\"%s\",\"compile_date\":\"%s\"}", 
            g_fw_update_status, __TIME__, __DATE__);
    ESP_LOGI(TAG, "Sending OTA JSON: %s", otaJSON);
    
    return send_json_response(req, otaJSON);
}

static esp_err_t http_server_get_dht_sensor_readings_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/dhtSensor.json requested");
    char dhtSensorJSON[100];
    
    sprintf(dhtSensorJSON, "{\"temperature\":\"%.1f\",\"humidity\":\"%.1f\"}", 
            DHT22_get_temperature(), DHT22_get_humidity());
    
    return send_json_response(req, dhtSensorJSON);
}

// Helper function to get header value
static char* get_header_value(httpd_req_t *req, const char *header_name)
{
    size_t len = httpd_req_get_hdr_value_len(req, header_name) + 1;
    if (len <= 1) return NULL;
    
    char *value = malloc(len);
    if (httpd_req_get_hdr_value_str(req, header_name, value, len) == ESP_OK) {
        ESP_LOGI(TAG, "Found header => %s: %s", header_name, value);
        return value;
    }
    
    free(value);
    return NULL;
}

static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/wifiConnect.json requested");
    
    char *ssid_str = get_header_value(req, "my-connect-ssid");
    char *pass_str = get_header_value(req, "my-connect-pwd");
    
    if (ssid_str && pass_str) {
        wifi_config_t* wifi_config = wifi_app_get_wifi_config();
        memset(wifi_config, 0x00, sizeof(wifi_config_t));
        memcpy(wifi_config->sta.ssid, ssid_str, strlen(ssid_str));
        memcpy(wifi_config->sta.password, pass_str, strlen(pass_str));
        wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);
    }
    
    free(ssid_str);
    free(pass_str);
    return ESP_OK;
}

static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/wifiConnectStatus requested");
    char statusJSON[100];
    
    sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);
    return send_json_response(req, statusJSON);
}

static esp_err_t http_server_get_wifi_connect_info_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/wifiConnectInfo.json requested");
    char ipInfoJSON[200] = {0};
    
    if (g_wifi_connect_status == HTTP_WIFI_STATUS_CONNECT_SUCCESS) {
        wifi_ap_record_t wifi_data;
        esp_netif_ip_info_t ip_info;
        
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_data));
        ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_sta, &ip_info));
        
        char ip[IP4ADDR_STRLEN_MAX], netmask[IP4ADDR_STRLEN_MAX], gw[IP4ADDR_STRLEN_MAX];
        esp_ip4addr_ntoa(&ip_info.ip, ip, IP4ADDR_STRLEN_MAX);
        esp_ip4addr_ntoa(&ip_info.netmask, netmask, IP4ADDR_STRLEN_MAX);
        esp_ip4addr_ntoa(&ip_info.gw, gw, IP4ADDR_STRLEN_MAX);
        
        sprintf(ipInfoJSON, "{\"ip\":\"%s\",\"netmask\":\"%s\",\"gw\":\"%s\",\"ap\":\"%s\"}", 
                ip, netmask, gw, (char*)wifi_data.ssid);
        ESP_LOGI(TAG, "Sending connection info JSON: %s", ipInfoJSON);
    }
    
    return send_json_response(req, ipInfoJSON);
}

static esp_err_t http_server_wifi_disconnect_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "wifiDisconnect.json requested");
    wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);
    return send_json_response(req, "{\"status\":\"disconnecting\"}");
}

static esp_err_t http_server_get_local_time_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/localTime.json requested");
    char localTimeJSON[150] = {0};
    
    ESP_LOGI(TAG, "g_is_local_time_set: %s", g_is_local_time_set ? "true" : "false");
    
    if (g_is_local_time_set) {
        char* time_str = sntp_time_sync_get_time();
        ESP_LOGI(TAG, "Time string from SNTP: %s", time_str);
        
        if (time_str != NULL && strlen(time_str) > 0) {
            sprintf(localTimeJSON, "{\"time\":\"%s\",\"status\":\"synchronized\"}", time_str);
        } else {
            sprintf(localTimeJSON, "{\"time\":\"Synchronizing...\",\"status\":\"pending\"}");
        }
    } else {
        sprintf(localTimeJSON, "{\"time\":\"Time service not initialized\",\"status\":\"not_initialized\"}");
    }
    
    ESP_LOGI(TAG, "Sending JSON: %s", localTimeJSON);
    return send_json_response(req, localTimeJSON);
}

static esp_err_t http_server_get_ap_ssid_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/apSSID.json requested");
    char ssidJSON[50];
    
    wifi_config_t *wifi_config = wifi_app_get_wifi_config();
    esp_wifi_get_config(ESP_IF_WIFI_AP, wifi_config);
    
    sprintf(ssidJSON, "{\"ssid\":\"%s\"}", (char*)wifi_config->ap.ssid);
    return send_json_response(req, ssidJSON);
}

// URI handler registration helper
static void register_uri_handler(httpd_handle_t server, const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r))
{
    httpd_uri_t uri_handler = {
        .uri = uri,
        .method = method,
        .handler = handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_handler);
}

static httpd_handle_t http_server_configure(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Create HTTP Server monitor task
    xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_TASK_STACK_SIZE, 
                           NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);
    
    // Create message queue
    http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));
    
    // Configure server settings
    config.core_id = HTTP_SERVER_TASK_CORE_ID;
    config.task_priority = HTTP_SERVER_TASK_PRIORITY;
    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
    config.max_uri_handlers = 20;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    
    ESP_LOGI(TAG, "Starting server on port: '%d' with task priority: '%d'", 
             config.server_port, config.task_priority);
    
    if(httpd_start(&http_server_handle, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        
        // Register all handlers using helper function
        register_uri_handler(http_server_handle, "/jquery-3.3.1.min.js", HTTP_GET, http_server_jquery_handler);
        register_uri_handler(http_server_handle, "/", HTTP_GET, http_server_index_html_handler);
        register_uri_handler(http_server_handle, "/app.css", HTTP_GET, http_server_app_css_handler);
        register_uri_handler(http_server_handle, "/app.js", HTTP_GET, http_server_app_js_handler);
        register_uri_handler(http_server_handle, "/favicon.ico", HTTP_GET, http_server_favicon_ico_handler);
        register_uri_handler(http_server_handle, "/OTAupdate", HTTP_POST, http_server_OTA_update_handler);
        register_uri_handler(http_server_handle, "/OTAstatus", HTTP_POST, http_server_OTA_status_handler);
        register_uri_handler(http_server_handle, "/dhtSensor.json", HTTP_GET, http_server_get_dht_sensor_readings_json_handler);
        register_uri_handler(http_server_handle, "/wifiConnect.json", HTTP_POST, http_server_wifi_connect_json_handler);
        register_uri_handler(http_server_handle, "/wifiConnectStatus.json", HTTP_POST, http_server_wifi_connect_status_json_handler);
        register_uri_handler(http_server_handle, "/wifiConnectInfo.json", HTTP_GET, http_server_get_wifi_connect_info_json_handler);
        register_uri_handler(http_server_handle, "/wifiDisconnect.json", HTTP_DELETE, http_server_wifi_disconnect_json_handler);
        register_uri_handler(http_server_handle, "/localTime.json", HTTP_GET, http_server_get_local_time_json_handler);
        register_uri_handler(http_server_handle, "/apSSID.json", HTTP_GET, http_server_get_ap_ssid_json_handler);
        
        return http_server_handle;
    }
    
    return NULL;
}

void http_server_start(void)
{
    if(http_server_handle == NULL) {
        http_server_handle = http_server_configure();
    }
}

void http_server_stop(void)
{
    if(http_server_handle) {
        httpd_stop(http_server_handle);
        ESP_LOGI(TAG, "Stopping HTTP Server");
        http_server_handle = NULL;
    }
    if (task_http_server_monitor) {
        vTaskDelete(task_http_server_monitor);
        ESP_LOGI(TAG, "Stopping HTTP Server monitor");
        task_http_server_monitor = NULL;
    }
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
    http_server_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void *arg)
{
    ESP_LOGI(TAG, "Timer timed-out, restarting device");
    esp_restart();
}