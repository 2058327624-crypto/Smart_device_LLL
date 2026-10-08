#include "weather.h"
#include <time.h>
#include <WiFi.h>

static const char* const WEATHER_API_KEY  = SENIVERSE_API_KEY;
static const char* const WEATHER_LOCATION = WEATHER_CITY;

static const char* const WEATHER_LANGUAGE = "zh-Hans";
static const char* const WEATHER_UNIT = "c";

#define URL_BASE "http://api.seniverse.com/v3/weather/"

// API key / 城市这些从 secrets.h 注入，别让它们泄漏成全局符号
static const char* const week_names[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};

static String build_url(const char* endpoint, const char* extra)
{
    String u = String(URL_BASE) + endpoint;
    u += "?key=";      u += WEATHER_API_KEY;
    u += "&location="; u += WEATHER_LOCATION;
    u += "&language="; u += WEATHER_LANGUAGE;
    u += "&unit=";     u += WEATHER_UNIT;
    if (extra) u += extra;
    return u;
}

// 获取实时+未来天气并更新UI
bool get_weather(void)
{
    HTTPClient http_now;      // 实时天气HTTP客户端
    HTTPClient http_daily;    // 未来天气HTTP客户端

    String url_now   = build_url("now.json", nullptr);
    String url_daily = build_url("daily.json", "&start=0&days=4");
    
    http_now.begin(url_now);
    int httpCode_now = http_now.GET();
    if (httpCode_now != 200) {

        Serial.printf("[天气] 实时接口 HTTP %d  (WiFi=%d)\n",
                      httpCode_now, (int)WiFi.status());
        if (httpCode_now == -1) {
            Serial.printf("[天气] 请求地址: %s\n", url_now.c_str());
        }
        http_now.end();
        return false;
    }
    String response_now = http_now.getString();
    http_now.end();

    DynamicJsonDocument doc_now(1024);
    DeserializationError error_now = deserializeJson(doc_now, response_now);
    if (error_now) {
        Serial.printf("[天气] 实时JSON解析失败: %s\n", error_now.c_str());
        return false;
    }

    DynamicJsonDocument doc_daily(2048);
    bool daily_ok = false;
    http_daily.begin(url_daily);
    int httpCode_daily = http_daily.GET();
    if (httpCode_daily == 200) {
        String response_daily = http_daily.getString();
        http_daily.end();
        daily_ok = !deserializeJson(doc_daily, response_daily);
        if (!daily_ok) Serial.println("[天气] 预报JSON解析失败，只显示实时天气");
    } else {
        Serial.printf("[天气] 预报接口 HTTP %d，只显示实时天气\n", httpCode_daily);
        http_daily.end();
    }

    strlcpy(g_weather_now.city, 
             doc_now["results"][0]["location"]["name"] | "未知", 
             sizeof(g_weather_now.city));
    
    strlcpy(g_weather_now.temp, 
             doc_now["results"][0]["now"]["temperature"] | "N/A", 
             sizeof(g_weather_now.temp));
    
    strlcpy(g_weather_now.text, 
             doc_now["results"][0]["now"]["text"] | "N/A", 
             sizeof(g_weather_now.text));
    
    strlcpy(g_weather_now.humi, 
             doc_now["results"][0]["now"]["humidity"] | "N/A", 
             sizeof(g_weather_now.humi));
    
    strlcpy(g_weather_now.last_update, 
             doc_now["results"][0]["last_update"] | "N/A", 
             sizeof(g_weather_now.last_update));
    

    g_weather_forecast[0] = '\0';
    

    time_t now_time = time(NULL);
    struct tm timeinfo;
    localtime_r(&now_time, &timeinfo);
    int today_wday = timeinfo.tm_wday;
    
    {
        char line[96];
        snprintf(line, sizeof(line), "%s %s %s度\n",
                 g_weather_now.city, g_weather_now.text, g_weather_now.temp);
        strcat(g_weather_forecast, line);
    }

    JsonArray daily = doc_daily["results"][0]["daily"];
    int n = daily_ok ? (int)daily.size() : 0;

    for (int i = 0; i < n; i++) {
        JsonObject d = daily[i];
        const char* text_day  = d["text_day"] | "";
        const char* low_temp  = d["low"]      | "";
        const char* high_temp = d["high"]     | "";

        int wday = (today_wday + i) % 7;
        const char* day_name = (i == 0) ? "今天" : week_names[wday];

        char line[128];
        snprintf(line, sizeof(line), "%s %s %s~%s度\n",
                 day_name, text_day, high_temp, low_temp);

        strcat(g_weather_forecast, line);
    }
    
    // 去掉末尾多余换行符
    size_t len = strlen(g_weather_forecast);
    if (len > 0 && g_weather_forecast[len-1] == '\n') {
        g_weather_forecast[len-1] = '\0';
    }
    
    return true;
}
