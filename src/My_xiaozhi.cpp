#include "My_xiaozhi.h"
extern Audio audio;

static bool speaking_flag = false;

void My_xiaozhi_init()
{
    xiaozhi_init();
}

void My_xiaozhi_loop()
{
    if (audio.isRunning())//检查音频是否正在播放
    {
        return;
    }
    xiaozhi_loop();

    // 识别结果由库内部缓存，xiaozhi_answer() 会自行取用，这里不需要接返回值
    xiaozhi_listen();

    if (xiaozhi_speak() && !speaking_flag)
    {
        speaking_flag = true; // 锁住，防止循环重复执行

        String answer_url  = xiaozhi_answer(1);   //1 TTS url

        audio.stopSong(); // 停止之前正在播放的音频
        audio.connecttohost(answer_url.c_str());
    }

    // 播放结束，释放标志位
    if(speaking_flag && !audio.isRunning())
    {
        speaking_flag = false;
    }

    delay(10);
}

