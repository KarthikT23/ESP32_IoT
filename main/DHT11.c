/*------------------------------------------------------------------------------
	DHT11 temperature & humidity sensor driver for ESP32
	Code based on your implementation with improvements
------------------------------------------------------------------------------*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "tasks_common.h"
#include "DHT11.h"

// == global defines =============================================
static const char* TAG = "DHT11";
static dht11_t dht_sensor = { .dht11_pin = DHT_GPIO, .temperature = 0.0, .humidity = 0.0 };

int wait_for_state(dht11_t dht11, int state, int timeout)
{
    gpio_set_direction(dht11.dht11_pin, GPIO_MODE_INPUT);
    int count = 0;
    
    while(gpio_get_level(dht11.dht11_pin) != state)
    {
        if(count >= timeout) return -1;
        count += 2;
        ets_delay_us(2);
    }
    return count;
}

void hold_low(dht11_t dht11, int hold_time_us)
{
    gpio_set_direction(dht11.dht11_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht11.dht11_pin, 0);
    ets_delay_us(hold_time_us);
    gpio_set_level(dht11.dht11_pin, 1);
}

int dht11_read(dht11_t *dht11, int connection_timeout)
{
    int waited = 0;
    int one_duration = 0;
    int zero_duration = 0;
    int timeout_counter = 0;
    uint8_t received_data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    
    while(timeout_counter < connection_timeout)
    {
        timeout_counter++;
        gpio_set_direction(dht11->dht11_pin, GPIO_MODE_INPUT);
        hold_low(*dht11, 18000);  // Hold low for 18ms to trigger DHT11
        
        // Phase 1: Wait for DHT11 to pull the line low
        waited = wait_for_state(*dht11, 0, 40);
        if(waited == -1)
        {
            ESP_LOGE(TAG, "Failed at phase 1");
            ets_delay_us(20000);
            continue;
        } 
        
        // Phase 2: Wait for DHT11 to pull the line high
        waited = wait_for_state(*dht11, 1, 90);
        if(waited == -1)
        {
            ESP_LOGE(TAG, "Failed at phase 2");
            ets_delay_us(20000);
            continue;
        } 
        
        // Phase 3: Wait for DHT11 to pull the line low again
        waited = wait_for_state(*dht11, 0, 90);
        if(waited == -1)
        {
            ESP_LOGE(TAG, "Failed at phase 3");
            ets_delay_us(20000);
            continue;
        }
        
        break;
    }
    
    if(timeout_counter == connection_timeout) 
    {
        ESP_LOGE(TAG, "Connection timeout");
        return DHT_TIMEOUT_ERROR;
    }
    
    // Read 40 bits of data (5 bytes)
    for(int i = 0; i < 5; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            zero_duration = wait_for_state(*dht11, 1, 58);
            one_duration = wait_for_state(*dht11, 0, 74);
            
            // Check if any waiting failed
            if (zero_duration == -1 || one_duration == -1) {
                ESP_LOGE(TAG, "Timeout while reading bits");
                return DHT_TIMEOUT_ERROR;
            }
            
            // For DHT11: bit is '1' if high pulse is longer than low pulse
            if (one_duration > zero_duration) {
                received_data[i] |= (1 << (7 - j));
            }
        }
    }
    
    // Debug: print raw data
    ESP_LOGD(TAG, "Raw data: %02x %02x %02x %02x %02x", 
             received_data[0], received_data[1], received_data[2], 
             received_data[3], received_data[4]);
    
    // Verify checksum
    int crc = received_data[0] + received_data[1] + received_data[2] + received_data[3];
    crc = crc & 0xff;
    
    if(crc == received_data[4]) 
    {
        // For DHT11 (different from DHT22):
        // Byte 0: Humidity integral part
        // Byte 1: Humidity decimal part (usually 0 for DHT11)
        // Byte 2: Temperature integral part
        // Byte 3: Temperature decimal part (usually 0 for DHT11)
        
        // Most DHT11 sensors only return whole numbers for humidity and temperature
        dht11->humidity = (float)received_data[0];
        dht11->temperature = (float)received_data[2];
        
        // Check if the temperature or humidity readings are reasonable
        if (dht11->temperature > 60 || dht11->humidity > 100) {
            ESP_LOGW(TAG, "Suspicious readings: Temp=%0.1f, Humidity=%0.1f", 
                    dht11->temperature, dht11->humidity);
        }
        
        return DHT_OK;
    }
    else 
    {
        ESP_LOGE(TAG, "Wrong checksum: calculated %d, received %d", crc, received_data[4]);
        return DHT_CHECKSUM_ERROR;
    }
}

static void DHT11_task(void *pvParameter)
{
    // Initialize GPIO with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << dht_sensor.dht11_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "DHT11 task started on pin %d", dht_sensor.dht11_pin);
    
    for (;;)
    {
        //ESP_LOGI(TAG, "Reading DHT11 sensor...");
        int ret = dht11_read(&dht_sensor, 5);  // Try 5 times before giving up
        
        if (ret == DHT_OK)
        {
            //ESP_LOGI(TAG, "Temperature: %.1f C", dht_sensor.temperature);
            //ESP_LOGI(TAG, "Humidity: %.1f%%", dht_sensor.humidity);
        }
        else if (ret == DHT_CHECKSUM_ERROR)
        {
            //ESP_LOGE(TAG, "Checksum error");
        }
        else if (ret == DHT_TIMEOUT_ERROR)
        {
            //ESP_LOGE(TAG, "Sensor timeout");
        }
        
        // Wait at least 2 seconds before next reading
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

float DHT11_get_temperature(void)
{
    return dht_sensor.temperature;
}

float DHT11_get_humidity(void)
{
    return dht_sensor.humidity;
}

void DHT11_task_start(void)
{
    dht_sensor.dht11_pin = DHT_GPIO;
    xTaskCreatePinnedToCore(&DHT11_task, "DHT11_task", DHT11_TASK_STACK_SIZE, NULL, DHT11_TASK_PRIORITY, NULL, DHT11_TASK_CORE_ID);
}
