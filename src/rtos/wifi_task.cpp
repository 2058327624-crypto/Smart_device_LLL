#include "wifi_task.h"

volatile uint8_t g_wifi_scan_req = 0;
volatile uint8_t g_wifi_connect_req = 0;
char g_wifi_ssid_buf[32] = {0};
char g_wifi_pwd_buf[64] = {0};
TaskHandle_t wifi_task_handle = NULL;
static int last_wifi_status = -99;        // -99 = 首次运行标记
static uint32_t g_wifi_refresh_tick = 0;  // 定时刷新计时

// 修剪换行、空格、回车工具函数
static void trim_wifi_str(char *buf)
{
    if(buf == nullptr) return;
    int len = strlen(buf);
    while(len > 0)
    {
        char ch = buf[len - 1];
        if(ch == '\n' || ch == '\r' || ch == ' ')
        {
            buf[len - 1] = '\0';
            len--;
        }
        else
        {
            break;
        }
    }
}

static void update_wifi_label_cb(void *param)
{
    char *text_buf = (char*)param;
    if (ui_TextArea4 != NULL)
    {
        lv_textarea_set_text(ui_TextArea4, text_buf);
    }
    free(text_buf);
}

void wifi_task(void *arg)
{
    //上电等待LVGL控件完全创建完成
    vTaskDelay(pdMS_TO_TICKS(800));
    g_wifi_scan_req = 1; //上电自动扫描
    g_wifi_refresh_tick = xTaskGetTickCount();
    
    for(;;)
    {
        int cur_status = WiFi.status();
        uint32_t now = xTaskGetTickCount();

        // 首次运行 或者 状态改变 或者 每2秒强制刷新一次UI
        if(last_wifi_status == -99 || cur_status != last_wifi_status || (now - g_wifi_refresh_tick > pdMS_TO_TICKS(2000)))
        {
            g_wifi_refresh_tick = now;
            last_wifi_status = cur_status;

            char* txt = (char*)malloc(128);
            if(txt != nullptr)
            {
                if(cur_status == WL_CONNECTED)
                {
                    IPAddress ip = WiFi.localIP();
                    sprintf(txt, "WiFi:已连接\nIP:%s", ip.toString().c_str());
                }
                else
                {
                    sprintf(txt, "WiFi:未连接");
                }
                lv_async_call(update_wifi_label_cb, txt);
            }
        }

        //扫描请求处理
        if(g_wifi_scan_req == 1)
        {
            g_wifi_scan_req = 0;
            wifi_Search();
            int ap_cnt = wifi_get_ap_count();
            std::string roller_opt;
            if(ap_cnt <= 0)
            {
                roller_opt = "未找到WiFi";
            }
            else
            {
                for(int i = 0; i < ap_cnt; i++)
                {
                    roller_opt += wifi_get_ap_ssid(i).c_str();
                    roller_opt += "\n";
                }
            }
            //跨任务安全更新Roller5，不能直接操作LVGL控件
            lv_async_call([](void* param){
                std::string* opt_str = (std::string*)param;
                lv_roller_set_options(ui_Roller5, opt_str->c_str(), LV_ROLLER_MODE_NORMAL);
                delete opt_str;
            }, new std::string(roller_opt));
            wifi_scan_clean(); //释放扫描内存
        }

        //连接WiFi请求处理
        if(g_wifi_connect_req == 1)
        {
            g_wifi_connect_req = 0;
            trim_wifi_str(g_wifi_ssid_buf);
            trim_wifi_str(g_wifi_pwd_buf);
            wifi_connect_ap(g_wifi_ssid_buf, g_wifi_pwd_buf,15000);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        
    }

}

void wifi_task_create(void)
{
    xTaskCreate(wifi_task, "wifi_task", 4500, NULL, 3, &wifi_task_handle);
}
