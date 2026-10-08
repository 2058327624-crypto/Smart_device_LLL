#include "My_Wifi.h"

// 真实值在 include/secrets.h（已被 .gitignore 忽略），由 platformio.ini 的
// -include secrets.h 全局注入，这里不需要 #include
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

#define TIME_ZONE     "CST-8"
#define NTP_SERVER    "ntp.aliyun.com"
// 上电WiFi连接
void My_wifi_init()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        timeout++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
}

//阻塞，只允许在wifi_task任务调用，禁止LVGL回调调用！
void wifi_Search()
{
    WiFi.scanNetworks();
}

// 获取扫描到热点总数
int wifi_get_ap_count()
{
    return WiFi.scanComplete();
}

// 获取对应索引SSID
String wifi_get_ap_ssid(int index)
{
    return WiFi.SSID(index);
}

// 动态连接传入的ssid和密码
bool wifi_connect_ap(const char* target_ssid, const char* pwd, uint32_t timeout_ms)
{
    timeout_ms = 10000;
    WiFi.disconnect(true);
    delay(200);

    WiFi.begin(target_ssid, pwd);
    uint32_t start = millis();

    while (millis() - start < timeout_ms)
    {
        if(WiFi.status() == WL_CONNECTED)
        {
            return true;
        }
        delay(100);
    }
    return false;
}

//释放扫描内存
void wifi_scan_clean()
{
    WiFi.scanDelete();
}


void ntp_start_sync(void)
{
    configTzTime(TIME_ZONE, NTP_SERVER, "cn.pool.ntp.org");
}