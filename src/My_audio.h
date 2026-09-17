#ifndef MY_AUDIO_H
#define MY_AUDIO_H

#ifdef __cplusplus
#include <Arduino.h>
#include <Audio.h>
extern Audio audio;
void My_audio_init();
void audio_loop();
#endif

#ifdef __cplusplus
extern "C" {
#endif


// 请求类型
enum {
    AUDIO_CMD_NONE = 0,
    AUDIO_CMD_PLAY_INDEX,   // 播放曲目 g_audio_req_index
    AUDIO_CMD_PAUSE,        // 暂停
    AUDIO_CMD_RESUME,       // 恢复播放
    AUDIO_CMD_VOLUME        // 设置音量 g_audio_req_vol
};

extern volatile int g_audio_cmd;
extern volatile int g_audio_req_index;
extern volatile int g_audio_req_vol;
extern volatile int g_audio_req_play;   // 1=选中后立即播放，0=只选中不出声

// 扫描 SD 卡根目录的 mp3
int music_scan(void);
// 曲目总数
int music_count(void);
// 取第 index 首的显示名（已去掉路径和 .mp3 后缀），越界返回 ""
const char* music_name(int index);
// 当前曲目序号，-1 表示还没有播放任何曲目
int music_current_index(void);

// autoPlay=0：只装载这首并停在暂停态，等播放开关打开再出声
// autoPlay=1：装载后立即播放
void audio_request_play(int index, int autoPlay);
void audio_request_pause(void);
void audio_request_resume(void);
void audio_request_volume(int vol);

// 查询（只读，任何任务都可调）
int audio_is_playing(void);
/* 是否还有没被 audio_task 执行的请求挂着（1=有）。
 * UI 同步开关状态前要先看这个，否则会在请求生效之前就把它否掉。 */
int audio_request_pending(void);
/* 音量安全上限：本板供电受限，超过这个值功放会把 3.3V 拉垮导致 SD 卡掉线 */
int audio_volume_max(void);

// 由 audio_task 每轮调用，执行挂起的请求
void audio_process_requests(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // MY_AUDIO_H
