#ifndef __WIFI_H__
#define __WIFI_H__
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "init_common.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define WIFI_SSID      "Xiaomi_B88D"
#define WIFI_PASS      "12345678"
#define MAXIMUM_RETRY  5
#define TCP_PORT 8080

void __wifi_link__(void);
#endif