/* 
	DHT22 temperature sensor driver
*/

#ifndef DHT22_H_  
#define DHT22_H_

#include <driver/gpio.h>
#include <stdio.h>
#include <string.h>
#include <rom/ets_sys.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DHT_OK 0
#define DHT_CHECKSUM_ERROR -1
#define DHT_TIMEOUT_ERROR -2

#define DHT_GPIO 13

/**
 * Structure containing readings and info about the dht22
 * @var dht22_pin the pin associated with the dht22
 * @var temperature last temperature reading
 * @var humidity last humidity reading 
*/
typedef struct
{
    int dht22_pin;
    float temperature;
    float humidity;
} dht22_t;

/**
 * @brief Wait on pin until it reaches the specified state
 * @return returns either the time waited or -1 in the case of a timeout
 * @param state state to wait for
 * @param timeout if counter reaches timeout the function returns -1
*/
int wait_for_state(dht22_t dht22, int state, int timeout);

/**
 * @brief Holds the pin low fo the specified duration
 * @param hold_time_us time to hold the pin low for in microseconds
*/
void hold_low(dht22_t dht22, int hold_time_us);

/**
 * @brief The function for reading temperature and humidity values from the dht22
 * @note  This function is blocking, ie: it forces the cpu to busy wait for the duration necessary to finish comms with the sensor.
 * @note  Wait for atleast 2 seconds between reads 
 * @param connection_timeout the number of connection attempts before declaring a timeout
*/
int dht22_read(dht22_t *dht22, int connection_timeout);

/**
 * @brief Start the DHT22 sensor task
 */
void DHT22_task_start(void);

/**
 * @brief Get the current temperature reading
 * @return Current temperature in Celsius
 */
float DHT22_get_temperature(void);

/**
 * @brief Get the current humidity reading
 * @return Current humidity percentage
 */
float DHT22_get_humidity(void);

#endif