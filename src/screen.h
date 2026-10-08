#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#if defined(__cplusplus)
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

class ScreenModule
{
public:
    ScreenModule();
    void init();
    TFT_eSPI& getTFT();
    bool readTouch(uint16_t &x, uint16_t &y);

    //亮度控制函数
    void setBrightness(uint8_t brightness);

private:
    TFT_eSPI tft;
    bool initialized;
    uint8_t currentBrightness = 255;
    static const uint8_t BL_PIN = 16;
};

extern ScreenModule g_screen;

#endif

#ifdef __cplusplus
extern "C" {
#endif

void screen_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_H
