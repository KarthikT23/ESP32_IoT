/*
 * http_server.h
 *
 *  Created on: 9 Dec 2024
 *      Author: karthik
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

#include "portmacro.h"
#include "bmp180.h"

// OTA Update Status Constants
#define OTA_UPDATE_PENDING      0
#define OTA_UPDATE_SUCCESSFUL   1
#define OTA_UPDATE_FAILED      -1

// Connection status for Wifi
typedef enum {
    NONE = 0,
    HTTP_WIFI_STATUS_CONNECTING,
    HTTP_WIFI_STATUS_CONNECT_FAILED,
    HTTP_WIFI_STATUS_CONNECT_SUCCESS,
    HTTP_WIFI_STATUS_DISCONNECTED,
} http_server_wifi_connect_status_e;

// Messages for the HTTP Monitor
typedef enum {
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
    HTTP_MSG_WIFI_CONNECT_SUCCESS,
    HTTP_MSG_WIFI_CONNECT_FAIL,
    HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
    HTTP_MSG_OTA_UPDATE_FAILED,
    HTTP_MSG_WIFI_USER_DISCONNECT,
    HTTP_MSG_TIME_SERVICE_INITIALIZED,
} http_server_message_e;

// Structure for Message queue
typedef struct {
    http_server_message_e msgID;
} http_server_queue_message_t;

// Function declarations
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);
void http_server_start(void);
void http_server_stop(void);
void http_server_fw_update_reset_callback(void *arg);

#endif /* MAIN_HTTP_SERVER_H_ */
