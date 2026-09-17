#ifndef My_WIFI_H
#define My_WIFI_H
#include <Arduino.h>
#include <WiFi.h>


void My_wifi_init();
void wifi_Search();
int wifi_get_ap_count();
String wifi_get_ap_ssid(int index);
bool wifi_connect_ap(const char* ssid, const char* pwd, uint32_t timeout_ms = 15000); 
void wifi_scan_clean();
void ntp_start_sync(void);

#endif // My_WIFI_H
