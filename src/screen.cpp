#include "screen.h"

ScreenModule g_screen;
ScreenModule::ScreenModule() : initialized(false), currentBrightness(255) {}
extern "C" void screen_set_brightness(uint8_t brightness)
{
    g_screen.setBrightness(brightness);
}

void ScreenModule::init()
{
    if (!initialized)
    {
        tft.begin();
        tft.setRotation(1);

        //初始化背光PWM
        ledcSetup(0, 5000, 8);        // 通道0, 5kHz频率, 8位分辨率(0-255)
        ledcAttachPin(BL_PIN, 0);     // 将GPIO16绑定到通道0
        ledcWrite(0, currentBrightness);  // 设置初始亮度(全亮)

        initialized = true;
        lv_init();

        const uint16_t screen_w = 320;
        const uint16_t screen_h = 240;

        // 半屏缓冲区
        static lv_color_t * buf;
        buf = (lv_color_t*)malloc(sizeof(lv_color_t) * screen_w * (screen_h / 2));

        static lv_disp_draw_buf_t draw_buf;
        lv_disp_draw_buf_init(&draw_buf, buf, nullptr, screen_w * (screen_h / 2));

        static lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);

        disp_drv.hor_res = screen_w;
        disp_drv.ver_res = screen_h;
        disp_drv.draw_buf = &draw_buf;

        // 刷新回调
        disp_drv.flush_cb = [](lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
        {
            ScreenModule* sc = (ScreenModule*)disp_drv->user_data;
            TFT_eSPI& tft = sc->getTFT();

            uint32_t w = area->x2 - area->x1 + 1;
            uint32_t h = area->y2 - area->y1 + 1;

            tft.startWrite();
            tft.setAddrWindow(area->x1, area->y1, w, h);
            tft.pushColors((uint16_t *)color_p, w * h, true);
            tft.endWrite();

            lv_disp_flush_ready(disp_drv);
        };

        disp_drv.user_data = this;
        lv_disp_drv_register(&disp_drv);

        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;

        indev_drv.read_cb = [](lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
        {
            ScreenModule* sc = (ScreenModule*)indev_drv->user_data;
            uint16_t x, y;

            if(sc->readTouch(x,y))
            {
                data->point.x = x;
                data->point.y = y;
                data->state = LV_INDEV_STATE_PR;
            }
            else
            {
                data->state = LV_INDEV_STATE_REL;
            }
        };
        indev_drv.user_data = this;
        lv_indev_drv_register(&indev_drv);
    }
}

/**
 * @brief 设置屏幕亮度
 * @param brightness 亮度值 (0=关闭, 255=最亮)
 */
void ScreenModule::setBrightness(uint8_t brightness)
{
    currentBrightness = brightness;
    ledcWrite(0, currentBrightness);  // 通过PWM设置亮度
}

TFT_eSPI& ScreenModule::getTFT()
{
    return tft;
}

bool ScreenModule::readTouch(uint16_t &x, uint16_t &y)
{
    if (tft.getTouch(&x, &y))
    {
        y = 240-1 - y;
        return true;
    }
    return false;
}