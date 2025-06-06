/*
 * tasks_common.h
 *
 *  Created on: 8 Dec 2024
 *      Author: karthik
 */

#ifndef MAIN_TASKS_COMMON_H_
#define MAIN_TASKS_COMMON_H_

// WiFi Application tasks
#define WIFI_APP_TASK_STACK_SIZE 				4096
#define WIFI_APP_TASK_PRIORITY					5
#define WIFI_APP_TASK_CORE_ID					0	

// HTTP Server Task
#define HTTP_SERVER_TASK_STACK_SIZE				8192
#define HTTP_SERVER_TASK_PRIORITY				4
#define HTTP_SERVER_TASK_CORE_ID 				0

// HTTP Server Monitor Task
#define HTTP_SERVER_MONITOR_STACK_SIZE			4096
#define HTTP_SERVER_MONITOR_PRIORITY			3
#define HTTP_SERVER_MONITOR_CORE_ID				0

// Wifi Reset Button task
#define WIFI_RESET_BUTTON_TASK_STACK_SIZE		2048
#define WIFI_RESET_BUTTON_TASK_PRIORITY			6
#define WIFI_RESET_BUTTON_TASK_CORE_ID			0

// DHT22 Sensor task
#define DHT22_TASK_STACK_SIZE					4096
#define DHT22_TASK_PRIORITY						5
#define DHT22_TASK_CORE_ID						1

// BMP180 Sensor task
#define BMP180_TASK_STACK_SIZE					4096
#define BMP180_TASK_PRIORITY					5
#define BMP180_TASK_CORE_ID						1

// SNTP Time sync task
#define SNTP_TIME_SYNC_TASK_STACK_SIZE			4096
#define SNTP_TIME_SYNC_TASK_PRIORITY			4
#define SNTP_TIME_SYNC_TASK_CORE_ID				1


#endif /* MAIN_TASKS_COMMON_H_ */

