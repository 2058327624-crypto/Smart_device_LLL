#include <Arduino.h>
#include "screen.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "My_Wifi.h"
#include "My_xiaozhi.h"
#include "rtos/xiaozhi_task.h"
#include "rtos/ui_task.h"
#include "rtos/audio_task.h"
#include "rtos/wifi_task.h"
#include "rtos/uart_task.h"
#include "rtos/weather_task.h"
#include "rtos/time_task.h"
#include "rtos/sd_task.h"
#include "sd_card.h"
#include "My_audio.h"

void setup()
{
  Serial.begin(115200);
  delay(100);

  g_screen.init();
  ui_init();
  ui_events_init();
  My_audio_init();
  My_xiaozhi_init();
  g_sdcard.init();
  wifi_task_create();
  xiaozhi_task_create();
  ui_task_create();
  audio_task_create();
  uart_task_create();
  time_task_create();
  weather_task_create();
  sd_task_create();
  
  My_wifi_init();
  ntp_start_sync();
}

void loop()
{

}
