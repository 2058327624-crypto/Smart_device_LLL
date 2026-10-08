#include "My_xiaozhi.h"
#include "My_audio.h"
#include "view/ui_view.h"

// 小智语音助手 —— 每轮对话的状态机
static bool speaking_flag = false;

void My_xiaozhi_init()
{
    xiaozhi_init();
}

void My_xiaozhi_loop()
{
    if (audio_is_playing())
    {
        return;
    }
    xiaozhi_loop();

   //xiaozhi_listen() 返回 true 表示刚识别完一整句。
    if (xiaozhi_listen())
    {
        String q = xiaozhi_question();
        String line = String("> ") + q;
        ui_view_xiaozhi_line(line.c_str());
    }

    if (xiaozhi_speak() && !speaking_flag)
    {
        speaking_flag = true;

        //先取回答文本：xiaozhi_answer(0) 读的是答案文字，不清标志；
        //xiaozhi_answer(1) 才取 TTS 地址并清标志。
        String answer = xiaozhi_answer(0);
        String line = String("小智: ") + answer;
        ui_view_xiaozhi_line(line.c_str());

        String answer_url = xiaozhi_answer(1);   // 1 = TTS url
        audio_request_tts(answer_url.c_str());
    }

    // 播放结束，释放标志位
    if(speaking_flag && !audio_is_playing())
    {
        speaking_flag = false;
    }

    delay(10);
}
