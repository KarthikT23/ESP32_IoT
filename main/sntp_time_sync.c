/*
 * sntp_time_sync.c
 *
 *  Created on: May 21, 2025
 *      Author: Karthik
 */
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"
#include "portmacro.h"
#include "tasks_common.h"
#include "http_server.h"
#include "sntp_time_sync.h"
#include "wifi_app.h"
#include <stdlib.h>
#include <time.h>

static const char TAG[] = "sntp_time_sync";

// SNTP operating mode set status
static bool sntp_op_mode_set = false;
// Track if we've already sent the initialization message
static bool time_service_init_sent = false;

/*
* Initialize SNTP service using SNTP_OPMODE_POLL mode
*/
static void sntp_time_sync_init_sntp(void)
{
	ESP_LOGI(TAG, "Initializing the SNTP service");
	if (!sntp_op_mode_set)
	{
		// Set the operating mode
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		sntp_op_mode_set = true;
	}
	sntp_setservername(0, "pool.ntp.org");
	
	// Initialize the servers
	sntp_init();
	
	// Let the http_server know that service is initialized
	// Only send this message once
	if (!time_service_init_sent)
	{
		http_server_monitor_send_message(HTTP_MSG_TIME_SERVICE_INITIALIZED);
		time_service_init_sent = true;
		ESP_LOGI(TAG, "Sent HTTP_MSG_TIME_SERVICE_INITIALIZED message");
	}
}

/*
* Gets the current time and if the current time is not upto date, the sntp_time_sync_init_sntp function is called
*/
static void sntp_time_sync_obtain_time(void)
{
	time_t now = 0;
	struct tm time_info = {0};
	
	time(&now);
	localtime_r(&now, &time_info);
	
	ESP_LOGI(TAG, "Current time check - Year: %d (comparing with 2016)", time_info.tm_year + 1900);
	
	// Check the time, in case we need to initialize/reinitialize
	if (time_info.tm_year < (2016 - 1900))
	{
		ESP_LOGI(TAG, "Time not set, initializing SNTP");
		sntp_time_sync_init_sntp();
		// Set the local time zone
		setenv("TZ", "IST-5:30", 1);
		tzset();
	}
	else
	{
		ESP_LOGI(TAG, "Time appears to be set correctly");
		// Send the initialization message if we haven't already
		if (!time_service_init_sent)
		{
			http_server_monitor_send_message(HTTP_MSG_TIME_SERVICE_INITIALIZED);
			time_service_init_sent = true;
			ESP_LOGI(TAG, "Sent HTTP_MSG_TIME_SERVICE_INITIALIZED message (time already set)");
		}
	}
}

/*
* The SNTP time synchronization task
@param arg pvParam
*/
static void sntp_time_sync(void *pvParam)
{
	ESP_LOGI(TAG, "SNTP time sync task started");
	while(1)
	{
		sntp_time_sync_obtain_time();
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
	
	vTaskDelete(NULL);
}

char* sntp_time_sync_get_time(void)
{
	static char time_buffer[100] = {0};
	
	time_t now = 0;
	struct tm time_info = {0};
	
	time(&now);
	localtime_r(&now, &time_info);
	
	ESP_LOGI(TAG, "sntp_time_sync_get_time called - Year: %d", time_info.tm_year + 1900);
	
	if (time_info.tm_year < (2016 - 1900))
	{
		ESP_LOGI(TAG, "Time is not set yet");
		strcpy(time_buffer, "Time not synchronized");
	}
	else 
	{
		strftime(time_buffer, sizeof(time_buffer), "%d/%m/%Y , %H:%M:%S", &time_info);
		ESP_LOGI(TAG, "Current time info: %s", time_buffer);
	}
	return time_buffer;
}

void sntp_time_sync_task_start(void)
{
	ESP_LOGI(TAG, "Starting SNTP time sync task");
	xTaskCreatePinnedToCore(&sntp_time_sync, "sntp_time_sync", SNTP_TIME_SYNC_TASK_STACK_SIZE, NULL, SNTP_TIME_SYNC_TASK_PRIORITY, NULL, SNTP_TIME_SYNC_TASK_CORE_ID);	
}