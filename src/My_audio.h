#ifndef MY_AUDIO_H
#define MY_AUDIO_H

#include "state/app_state.h"

#ifdef __cplusplus
#include <Arduino.h>
#include <Audio.h>
/* audio 这个对象只给本模块内部用，不要 extern 出去 —— 它不是线程安全的，
 * 外面拿到就会绕过 audio_process_requests() 的串行化。 */
void My_audio_init();
void audio_loop();
#endif

#ifdef __cplusplus
extern "C" {
#endif


int music_scan(void);
int music_count(void);
const char* music_name(int index);
int music_current_index(void);

void audio_request_play(int index, int autoPlay);
void audio_request_pause(void);
void audio_request_resume(void);
void audio_request_volume(int vol);
void audio_request_tts(const char* url);
int audio_is_playing(void);
int audio_request_pending(void);
int audio_volume_max(void);
void audio_process_requests(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // MY_AUDIO_H
