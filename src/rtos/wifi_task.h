#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <stdint.h>

/* 以下头文件只给 C++ 用；C 文件（ui_events.c）不能看到它们 */
#if defined(__cplusplus)
#include <Arduino.h>
#include <WiFi.h>
#include "My_Wifi.h"
#include <string>
#include <cstring>
#include "ui/ui.h"
extern "C" {
#endif

extern volatile uint8_t g_wifi_scan_req;
extern volatile uint8_t g_wifi_connect_req;
extern char g_wifi_ssid_buf[32];
extern char g_wifi_pwd_buf[64];
void wifi_task(void *arg);
void wifi_task_create(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // WIFI_TASK_H