#include "My_audio.h"
#include "sd_card.h"

Audio audio(false,3,I2S_NUM_1);

// 音量上限。库的可调范围是 0~21，但本板 3.3V 供电余量不足：
#define AUDIO_VOLUME_MAX 10
// 曲目表容量
#define MUSIC_MAX_TRACKS 32
// 单条曲目路径长度（"/" + 文件名 + ".mp3" + '\0'）
#define MUSIC_PATH_MAX   64
// 曲目显示名长度（去掉路径和 .mp3 后缀）
#define MUSIC_NAME_MAX   40

static char s_track_path[MUSIC_MAX_TRACKS][MUSIC_PATH_MAX];
static char s_track_name[MUSIC_MAX_TRACKS][MUSIC_NAME_MAX];
static int  s_track_count = 0;
static int  s_current     = -1;   // 当前曲目序号，-1 = 未播放


void My_audio_init() {
    // 初始化音频播放相关的设置
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(AUDIO_VOLUME_MAX);
    // 状态层里给的是 0，音量请求的初值在这里补上，免得第一次静音
    g_audio_req_vol = AUDIO_VOLUME_MAX;
}

void audio_loop() {
    // 处理音频播放相关的逻辑
    audio.loop();
}

// 判断文件名是否以 .mp3 结尾
static bool has_mp3_ext(const char* name)
{
    size_t len = strlen(name);
    if(len < 4) return false;
    const char* ext = name + len - 4;
    return (ext[0] == '.' &&
            (ext[1] == 'm' || ext[1] == 'M') &&
            (ext[2] == 'p' || ext[2] == 'P') &&
            (ext[3] == '3'));
}

int music_scan(void)
{
    s_track_count = 0;
    // 检查 SD 卡是否挂载
    if(!g_sdcard.isMounted()) return 0;

    // 只扫根目录。放子目录里的话这里扫不到，需要的话再改成递归。
    SD_LOCK();
    File root = SD.open("/");
    if(!root || !root.isDirectory()) {
        if(root) root.close();
        SD_UNLOCK();
        return 0;
    }

    File entry = root.openNextFile();
    while(entry && s_track_count < MUSIC_MAX_TRACKS) {
        if(!entry.isDirectory()) {
            const char* nm = entry.name();
            if(has_mp3_ext(nm)) {
                // entry.name() 在根目录下可能带前导 '/'，统一去掉
                const char* base = (nm[0] == '/') ? nm + 1 : nm;

                snprintf(s_track_path[s_track_count], MUSIC_PATH_MAX,
                         "/%s", base);

                // 显示名：去掉 .mp3 后缀
                size_t nlen = strlen(base);
                size_t clen = (nlen > 4) ? (nlen - 4) : nlen;
                if(clen >= MUSIC_NAME_MAX) clen = MUSIC_NAME_MAX - 1;
                memcpy(s_track_name[s_track_count], base, clen);
                s_track_name[s_track_count][clen] = '\0';

                s_track_count++;
            }
        }
        entry = root.openNextFile();
    }

    if(entry) entry.close();
    root.close();
    SD_UNLOCK();

    /* 换了卡或删了歌之后曲目表可能变短，把越界的播放位置清掉，
     * 免得后面按索引取曲目/歌名时越界。 */
    if(s_current >= s_track_count) s_current = -1;

    return s_track_count;
}

int music_count(void)
{
    return s_track_count;
}

const char* music_name(int index)
{
    if(index < 0 || index >= s_track_count) return "";
    return s_track_name[index];
}

int music_current_index(void)
{
    return s_current;
}


// autoPlay = 0：只把这首装载好、停在暂停态（由播放开关决定何时出声）
// autoPlay = 1：装载后立即播放
void audio_request_play(int index, int autoPlay)
{
    if(index < 0 || index >= s_track_count) return;
    g_audio_req_index = index;
    g_audio_req_play  = autoPlay ? 1 : 0;
    g_audio_cmd = AUDIO_CMD_PLAY_INDEX;
}

void audio_request_pause(void)
{
    g_audio_cmd = AUDIO_CMD_PAUSE;
}

void audio_request_resume(void)
{
    g_audio_cmd = AUDIO_CMD_RESUME;
}

void audio_request_volume(int vol)
{
    if(vol < 0) vol = 0;
    if(vol > AUDIO_VOLUME_MAX) vol = AUDIO_VOLUME_MAX;
    g_audio_req_vol = vol;
    g_audio_cmd = AUDIO_CMD_VOLUME;
}

/* 播放一段网络音频*/
void audio_request_tts(const char* url)
{
    if(url == nullptr || url[0] == '\0') return;
    size_t len = strlen(url);
    if(len >= sizeof(g_audio_req_tts_url))
    {
        Serial.printf("[音频] !! TTS URL 过长(%u 字节, 上限 %u), 已截断 —— "
                      "语音会不完整或没有声音\n",
                      (unsigned)len, (unsigned)sizeof(g_audio_req_tts_url) - 1);
    }

    strncpy(g_audio_req_tts_url, url, sizeof(g_audio_req_tts_url) - 1);
    g_audio_req_tts_url[sizeof(g_audio_req_tts_url) - 1] = '\0';
    g_audio_cmd = AUDIO_CMD_TTS_URL;
}

// 是否正在播放
int audio_is_playing(void)
{
    return audio.isRunning() ? 1 : 0;
}

int audio_volume_max(void)
{
    return AUDIO_VOLUME_MAX;
}

//判断是否有还有未被处理的请求挂起
int audio_request_pending(void)
{
    return (g_audio_cmd != AUDIO_CMD_NONE) ? 1 : 0;
}

// 处理所有挂起的请求
void audio_process_requests(void)
{
    int cmd = g_audio_cmd;
    if(cmd == AUDIO_CMD_NONE) return;

    // 先清标志，避免执行过程中被新请求覆盖
    g_audio_cmd = AUDIO_CMD_NONE;

    switch(cmd) {
    // 播放指定索引的曲目。
    case AUDIO_CMD_PLAY_INDEX: {
        int idx = g_audio_req_index;
        if(idx < 0 || idx >= s_track_count) break;

        bool wantPlay = (g_audio_req_play != 0);

        SD_LOCK();
        bool ok = audio.connecttoSD(s_track_path[idx]);
        SD_UNLOCK();

        if(ok) {
            s_current = idx;
            if(!wantPlay && audio.isRunning()) audio.pauseResume();
        } else {
            Serial.printf("[音频] 装载失败: %s\n", s_track_path[idx]);
        }
        break;
    }

    // 注意：pauseResume() 是「切换」不是「设置」，
    // 所以必须先用 isRunning() 判断当前状态，否则暂停请求会变成恢复播放
    case AUDIO_CMD_PAUSE:
        if(audio.isRunning()) audio.pauseResume();
        break;

    case AUDIO_CMD_RESUME: {
        if(audio.isRunning()) break;

        // 没选过歌就用第一首；一首都没有就什么都不做
        int idx = (s_current >= 0) ? s_current : ((s_track_count > 0) ? 0 : -1);
        if(idx < 0) break;

        SD_LOCK();
        bool ok = audio.connecttoSD(s_track_path[idx]);
        SD_UNLOCK();

        if(ok) {
            s_current = idx;
        }
        break;
    }

    case AUDIO_CMD_VOLUME:
        audio.setVolume((uint8_t)g_audio_req_vol);
        break;

    // 小智的语音回答。先停掉当前正在放的（可能是歌），再连网播。
    // 这两步必须是同一个任务里连着做，否则会有两个任务同时碰解码器。
    case AUDIO_CMD_TTS_URL:
        audio.stopSong();
        audio.connecttohost(g_audio_req_tts_url);
        break;

    default:
        break;
    }
}
