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

// DHT11 Sensor task
#define DHT11_TASK_STACK_SIZE					4096
#define DHT11_TASK_PRIORITY						5
#define DHT11_TASK_CORE_ID						1


#endif /* MAIN_TASKS_COMMON_H_ */
