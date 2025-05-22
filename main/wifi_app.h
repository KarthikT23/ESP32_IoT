/*
 * wifi_app.h
 *
 *  Created on: 8 Dec 2024
 *      Author: karthik
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h"
#include "esp_wifi_types.h"
#include "portmacro.h"
#include "http_server.h"

// Callback typedef
typedef void (*wifi_connected_event_callback_t)(void);

// WiFi application settings
#define WIFI_AP_SSID 					"ESP32_AP" // AP Name
#define WIFI_AP_PASSWORD 				"password" // Password
#define WIFI_AP_CHANNEL 	   			1		  // AP Channel
#define WIFI_AP_SSID_HIDDEN 			0	      // AP Visibility
#define WIFI_AP_MAX_CONNECTIONS 		5		  // AP Max Clients
#define WIFI_AP_BEACON_INTERVAL			100		  // AP Beacon: 100 milli seconds (recommended or default)
#define WIFI_AP_IP 						"192.168.0.3" // AP Default IP
#define WIFI_AP_GATEWAY 				"192.168.0.3" // AP Default Gateway (should be same as IP)
#define WIFI_AP_NETMASK 				"255.255.255.0" //AP Netmask
#define WIFI_AP_BANDWIDTH 				WIFI_BW_HT20 // AP Bandwidth is 20MHz (40MHz is another option)
#define WIFI_STA_POWER_SAVE 			WIFI_PS_NONE // Power save is not used
#define MAX_SSID_LENGTH 				32 			// IEEE Standard maximum
#define MAX_PASSWORD_LENGTH 			64  		// IEEE Standard maximum
#define MAX_CONNECTION_RETRIES 			5 			// Retry number on disconnect

// netif object for the Station and Access Point
extern esp_netif_t* esp_netif_sta;
extern esp_netif_t* esp_netif_ap;

// Message IDs for the WiFi application task
typedef enum wifi_app_message
{
	WIFI_APP_MSG_START_HTTP_SERVER = 0,
	WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
	WIFI_APP_MSG_STA_CONNECTED_GOT_IP,	
	WIFI_APP_MSG_STA_DISCONNECTED,
	WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT,
	WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS,
} wifi_app_message_e;

// Structure for the message queue
typedef struct wifi_app_queue_message 
{
	wifi_app_message_e msgID;	
} wifi_app_queue_message_t;

// Sends a message to the queue
// @param msgID message ID from the wifi_app_message_e enum.
// @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

// Starts the WiFi RTOS Task
void wifi_app_start(void);

/**
* Gets the wifi configuration 
*/
wifi_config_t* wifi_app_get_wifi_config(void);

/*
* Sets the callback function
*/
void wifi_app_set_callback(wifi_connected_event_callback_t cb);

/*
* Calls the callback function
*/
void wifi_app_call_callback(void);

#endif /* MAIN_WIFI_APP_H_ */
