#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "DHT22.h"
#include "portmacro.h"
#include "rgb_led.h"
#include "wifi_app.h"
#include "esp_log.h"
#include "sntp_time_sync.h"
#include "bmp180.h"

static const char TAG[] = "main";

void wifi_application_connected_events(void)
{
	ESP_LOGI(TAG, "Wifi Application Connected!");
	sntp_time_sync_task_start();
}

void app_main(void)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	
	// Set connected event callback
	wifi_app_set_callback(&wifi_application_connected_events);
	
	// Start WiFi
	wifi_app_start();
	
	// Start DHT22 sensor task
	DHT22_task_start();
	
	// Start BMP180 sensor task (SDA=21, SCL=22, altitude=920m)
	BMP180_task_start(21, 22, 920.0f);
}
