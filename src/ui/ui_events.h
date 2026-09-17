#ifndef _UI_EVENTS_H
#define _UI_EVENTS_H

#ifdef __cplusplus
extern "C" {
#endif

void ui_events_init(void);
void refresh_home_clock(void);
void switch_xiaozhi_cb(lv_event_t * e);
void music_page_on_show(void);
void music_sync_play_switch(void);
void on_volume_change(lv_event_t * e);
void on_song_select(lv_event_t * e);
void on_btn_play_pause(lv_event_t * e);
void on_btn_prev(lv_event_t * e);
void on_btn_next(lv_event_t * e);
void silder_brightness_cb(lv_event_t * e);
void Keyboard_Show(lv_event_t * e);
void Keyboard_hide(lv_event_t * e);
void Game_yang(lv_event_t * e);
void calendar_update_real_time(lv_event_t * e);
void Keyboard1_Show(lv_event_t * e);
void Keyboard1_send(lv_event_t * e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
