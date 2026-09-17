#ifndef _WEATHER_H
#define _WEATHER_H


#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <lvgl.h>


typedef struct {
    char city[32];           // 城市
    char temp[8];            // 当前温度
    char text[32];           // 天气描述（晴/多云等）
    char humi[8];            // 湿度
    char last_update[32];    // 更新时间
} WeatherNow_t;

bool get_weather(void);      // 获取天气（实时+未来），自动更新UI

extern WeatherNow_t g_weather_now;     // 实时天气数据
extern char g_weather_forecast[512];   // 未来天气预报文本（用于Roller）
void update_weather_ui(void);


#endif