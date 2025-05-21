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
#include "portmacro.h"
#include "DHT22.h"
#include "rgb_led.h"
#include "wifi_app.h"
#include "wifi_reset_button.h"
// Application entry point

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
	
	// Start WiFi
	wifi_app_start();
	
	// Configure Wifi reset button	
	wifi_reset_button_config();
	
	// Start DHT11 sensor task
	DHT11_task_start();
}

