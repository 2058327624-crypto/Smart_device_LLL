#include "ui_view.h"
#include "weather_icons.h"

#include <lv_games.h>          // yang_update()（羊了个羊）
#include "ui/ui.h"
#include "My_audio.h"          // music_* / audio_* 查询
#include "screen.h"            // screen_set_brightness
#include "sd_card.h"           // g_sdcard.isMounted()：区分"卡没好"和"卡里没歌"
#include "state/app_state.h"

#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* 小智对话框的行长度上限：字体缺字 + 画不下，超了直接截断 */
#define XIAOZHI_LINE_MAX 300

/* 串口接收框保留的最大字符数，超了从头砍。长时间收发不加限制会把堆吃光。 */
#define SERIAL_RX_MAX 2000

extern bool getLocalTime(struct tm * info, uint32_t ms);
// 主页

static void wifi_state_cb(void* param)
{
    const char* text = (const char*)param;
    if (ui_Label2 != NULL)
    {
        lv_label_set_text(ui_Label2, text);
    }
    free(param);
}

void ui_view_wifi_state(bool connected)
{
    char* txt = (char*)malloc(16);
    if (txt == NULL) return;
    strcpy(txt, connected ? "已连接" : "未连接");
    lv_async_call(wifi_state_cb, txt);
}


static void clock_cb(void* param)
{
    (void)param;
    struct tm tm_now;
    char buf[24];

    if (getLocalTime(&tm_now, 0U))
    {
        strftime(buf, sizeof(buf), "%H:%M:%S", &tm_now);
        if (ui_time != NULL) lv_label_set_text(ui_time, buf);
        strftime(buf, sizeof(buf), "%Y/%m/%d", &tm_now);
        if (ui_date != NULL) lv_label_set_text(ui_date, buf);
    }
    else
    {
        /* 还没对时成功（没连上 WiFi）。显示占位符，别让用户看到 1970 年。 */
        if (ui_time != NULL) lv_label_set_text(ui_time, "--:--:--");
        if (ui_date != NULL) lv_label_set_text(ui_date, "----/--/--");
    }
}

void ui_view_clock(void)
{
    /* 以前 time_task 直接调 refresh_home_clock() 写标签，跨任务碰控件。
     * 现在统一走异步。 */
    lv_async_call(clock_cb, NULL);
}
// 设置页

static void wifi_list_cb(void* param)
{
    std::string* opt = (std::string*)param;
    if (ui_Roller5 != NULL)
    {
        lv_roller_set_options(ui_Roller5, opt->c_str(), LV_ROLLER_MODE_NORMAL);
    }
    delete opt;
}

void ui_view_wifi_list(const char* options)
{
    if (options == NULL) return;
    lv_async_call(wifi_list_cb, new std::string(options));
}
// 音乐页
static bool  s_music_roller_syncing = false;
static char  s_music_list_buf[512];

void ui_view_music_list(void)
{

    if (ui_Roller4 == NULL) return;

    int n = music_scan();
    if (n <= 0)
    {
        /* music_scan() 返回 0 有两种可能，给不同的提示。两种情况都只是
         * 显示一行字，不记录任何状态 —— 下次进来照样重扫。 */
        lv_roller_set_options(ui_Roller4,
                              g_sdcard.isMounted() ? "未找到歌曲" : "SD卡未挂载",
                              LV_ROLLER_MODE_NORMAL);
        return;
    }

    /* 拼成 "歌名\n歌名\n..." 的格式，Roller 按换行分行 */
    s_music_list_buf[0] = '\0';
    for (int i = 0; i < n; i++)
    {
        if (i > 0) strncat(s_music_list_buf, "\n", sizeof(s_music_list_buf) - strlen(s_music_list_buf) - 1);
        strncat(s_music_list_buf, music_name(i), sizeof(s_music_list_buf) - strlen(s_music_list_buf) - 1);
    }
    lv_roller_set_options(ui_Roller4, s_music_list_buf, LV_ROLLER_MODE_NORMAL);

    /* 滚到当前正在播放的那首，没有就停在第一首 */
    int cur = music_current_index();
    lv_roller_set_selected(ui_Roller4, (cur >= 0) ? cur : 0, LV_ANIM_OFF);
}

/* 刚点完开关的宽限期：这段时间内不做状态同步，
 * 否则请求还没被 audio_task 执行，同步逻辑就把开关弹回去了。 */
static uint32_t s_music_switch_grace_until = 0;
#define MUSIC_SWITCH_GRACE_MS 800

void ui_view_music_switch_sync(void)
{
    /* 【同步】由 ui_task 每轮调。
     * 音频的实际状态在 audio_task 手里（audio_is_playing），UI 这边只是跟随，
     * 所以不能反过来把 UI 的状态当成播放状态用。 */
    if (ui_Switch2 == NULL) return;
    if (lv_scr_act() != ui_MusicPage) return;   // 不在音乐页就别白费劲
    if (audio_request_pending()) return;        // 请求还没执行完，别抢答
    if (millis() < s_music_switch_grace_until) return;

    bool running = (audio_is_playing() != 0);
    bool shown   = lv_obj_has_state(ui_Switch2, LV_STATE_CHECKED);
    if (running && !shown)       lv_obj_add_state(ui_Switch2, LV_STATE_CHECKED);
    else if (!running && shown)  lv_obj_clear_state(ui_Switch2, LV_STATE_CHECKED);
}
// 天气页
static bool has(const char* hay, const char* needle)
{
    return strstr(hay, needle) != NULL;
}

static const lv_img_dsc_t* weather_icon_for(const char* text)
{
    if (text == NULL) return &ui_img_weather_cloudy;

    if (has(text, "雷") || has(text, "冰雹") || has(text, "暴雨") ||
        has(text, "风暴") || has(text, "龙卷") || has(text, "飓"))
        return &ui_img_weather_storm;

    if (has(text, "雪")) return &ui_img_weather_snow;
    if (has(text, "雨")) return &ui_img_weather_rain;
    if (has(text, "风") || has(text, "沙") || has(text, "尘")) return &ui_img_weather_wind;
    if (has(text, "云")) return &ui_img_weather_cloudy;
    if (has(text, "晴")) return &ui_img_weather_sunny;
    if (has(text, "阴") || has(text, "雾") || has(text, "霾")) return &ui_img_weather_overcast;

    /* 认不出来的（含「未知」）统一给多云，和界面初始状态一致 */
    return &ui_img_weather_cloudy;
}

static void weather_cb(void* param)
{
    (void)param;

    /* 上半屏：当前天气图标 + 文字/温度。
     * ui_Label1 用的是 ui_font_Font4，四个字体里只有它含「℃」字形，
     * 所以温度单位别换字体。 */
    if (ui_Image10 != NULL)
    {
        lv_img_set_src(ui_Image10, weather_icon_for(g_weather_now.text));
    }
    if (ui_Label1 != NULL)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s\n%s℃", g_weather_now.text, g_weather_now.temp);
        lv_label_set_text(ui_Label1, buf);
    }

    /* 下半屏：未来几天预报 */
    if (ui_Roller6 != NULL)
    {
        lv_obj_set_style_text_font(ui_Roller6, &ui_font_Font1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_roller_set_options(ui_Roller6, g_weather_forecast, LV_ROLLER_MODE_NORMAL);
    }
}

void ui_view_weather(void)
{
    lv_async_call(weather_cb, NULL);
}
// 小智页
static void xiaozhi_line_cb(void* param)
{
    char* line = (char*)param;
    if (ui_TextArea5 != NULL)
    {
        lv_textarea_add_text(ui_TextArea5, line);
        lv_obj_scroll_to_y(ui_TextArea5, LV_COORD_MAX, LV_ANIM_OFF);
    }
    free(line);
}

void ui_view_xiaozhi_line(const char* line)
{
    if (line == NULL || line[0] == '\0') return;

    size_t len = strlen(line);
    if (len > XIAOZHI_LINE_MAX) len = XIAOZHI_LINE_MAX;

    /* +2 = 换行 + 结束符 */
    char* buf = (char*)malloc(len + 2);
    if (buf == NULL) return;
    memcpy(buf, line, len);
    buf[len]     = '\n';
    buf[len + 1] = '\0';

    lv_async_call(xiaozhi_line_cb, buf);
}
// 串口页
static void serial_append_cb(void* param)
{
    char* text = (char*)param;
    if (ui_TextArea3 != NULL)
    {
        lv_textarea_add_text(ui_TextArea3, text);

        /* 超过上限就把前面砍掉。串口可能一直有数据进来，
         * 不限制的话这个文本框会一直长，最后把堆吃光。 */
        const char* full = lv_textarea_get_text(ui_TextArea3);
        size_t n = full ? strlen(full) : 0;
        if (n > SERIAL_RX_MAX)
        {
            /* 从开头删掉超出的部分，留出余量免得每来一点就删一次 */
            size_t drop = n - SERIAL_RX_MAX + 256;
            char* keep = strdup(full + drop);
            if (keep != NULL)
            {
                lv_textarea_set_text(ui_TextArea3, keep);
                free(keep);
            }
        }
        lv_obj_scroll_to_y(ui_TextArea3, LV_COORD_MAX, LV_ANIM_OFF);
    }
    free(text);
}

void ui_view_serial_append(const char* text)
{
    if (text == NULL || text[0] == '\0') return;

    char* buf = strdup(text);
    if (buf == NULL) return;
    lv_async_call(serial_append_cb, buf);
}
// 游戏页
static void* s_game_root = NULL;

static void yang_exit_async_cb(void* user_data)
{
    (void)user_data;
    if (s_game_root == NULL) return;
    lv_anim_del_all();
    lv_obj_del((lv_obj_t*)s_game_root);
    s_game_root = NULL;
}

static void yang_exit_cb(lv_event_t* e)
{
    (void)e;
    lv_async_call(yang_exit_async_cb, NULL);
}

void ui_view_game_yang(void)
{
    if (s_game_root != NULL) return;   // 已经在游戏里了，忽略重复进入

    lv_obj_t* root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(root);      // 去掉默认白底/边框/圆角
    lv_obj_set_size(root, 320, 240);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    s_game_root = root;

    srand(millis());    // yang.c 自己不种随机种子，不种的话每次上电牌序完全一样
    yang_update(root);
    // 退出按钮
    lv_obj_t* btn = lv_btn_create(root);
    lv_obj_set_size(btn, 56, 26);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 4, -4);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xb03030), 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "退出");
    lv_obj_set_style_text_font(label, &ui_font_Font1, 0);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, yang_exit_cb, LV_EVENT_CLICKED, NULL);
}
