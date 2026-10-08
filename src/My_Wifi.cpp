#include "My_Wifi.h"
#include "state/app_state.h" 

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
        Serial.printf("[WiFi] 上电连接 %s 失败, status=%d (10 秒超时)\n",
                      ssid, (int)WiFi.status());
    }

    g_wifi_init_done = true;
}

//阻塞，只允许在wifi_task任务调用，禁止LVGL回调调用！
int wifi_Search()
{
    int n = WiFi.scanNetworks();
    if (n < 0) {
        // -1 = WIFI_SCAN_RUNNING(还在扫), -2 = WIFI_SCAN_FAILED
        Serial.printf("[WiFi] 扫描失败, err=%d (STA 是否还在 connecting?)\n", n);
    }
    return n;
}

// 获取对应索引SSID
String wifi_get_ap_ssid(int index)
{
    return WiFi.SSID(index);
}

// 动态连接传入的ssid和密码
bool wifi_connect_ap(const char* target_ssid, const char* pwd, uint32_t timeout_ms)
{
    if(target_ssid == NULL || target_ssid[0] == '\0')
    {
        Serial.println("[WiFi] 连接失败: SSID 为空");
        return false;
    }
    if(timeout_ms == 0) timeout_ms = 15000;
    WiFi.disconnect(false);
    delay(100);

    wl_status_t st = WiFi.begin(target_ssid, pwd);
    if(st == WL_CONNECT_FAILED)
    {
        Serial.printf("[WiFi] begin(\"%s\") 直接失败, status=%d\n", target_ssid, (int)st);
        return false;
    }

    uint32_t start = millis();
    while(millis() - start < timeout_ms)
    {
        if(WiFi.status() == WL_CONNECTED)
        {
            return true;
        }
        delay(100);
    }

    /*SSID 没找到 / 密码错 / 超时，状态码不同 */
    Serial.printf("[WiFi] 连接 \"%s\" 超时(%ums), 最后 status=%d\n",
                  target_ssid, (unsigned)timeout_ms, (int)WiFi.status());
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