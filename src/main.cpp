#include <Arduino.h>
#include "screen.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/ui_events.h"
#include "state/app_state.h"
#include "My_Wifi.h"
#include "My_xiaozhi.h"
#include "sd_card.h"
#include "My_audio.h"
#include "tasks/tasks.h"

void setup()
{
  Serial.begin(115200);
  delay(100);

  app_state_init();   //创建互斥锁

  g_screen.init();   // 初始化显示驱动
  ui_init();         // 初始化用户界面
  ui_events_init();  // 初始化事件  

  My_audio_init();   // 初始化音频
  My_xiaozhi_init(); // 初始化小智

  wifi_task_create();
  xiaozhi_task_create();
  ui_task_create();
  audio_task_create();
  uart_task_create();
  time_task_create();
  weather_task_create();
  sd_task_create();

  My_wifi_init();    // 初始化WiFi
  ntp_start_sync();  // 同步时间
}

void loop()
{

}
