#ifndef UI_VIEW_H
#define UI_VIEW_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_view_wifi_state(bool connected); // 主页 WiFi 状态标签
void ui_view_clock(void);                // 主页时钟标签
void ui_view_wifi_list(const char* options);// 设置页 WiFi 热点列表
void ui_view_music_list(void);// 音乐页 Roller 歌曲列表
void ui_view_music_switch_sync(void);// 音乐页开关状态同步（由 ui_task 调）
void ui_view_weather(void);// 天气页 Roller 歌曲列表
void ui_view_xiaozhi_line(const char* line);// 小智页对话框添加一行
void ui_view_serial_append(const char* text);// 串口页接收框添加一行
void ui_view_game_yang(void);// 游戏页

#ifdef __cplusplus
}
#endif

#endif // UI_VIEW_H
