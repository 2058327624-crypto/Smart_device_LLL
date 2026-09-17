/**
 * 私有凭据模板
 *
 * 用法：复制本文件为同目录下的 secrets.h，再把下面的占位符换成你自己的值。
 *       secrets.h 已被 .gitignore 忽略，不会进仓库。
 *
 *   cp include/secrets.example.h include/secrets.h
 *
 * platformio.ini 里的 -include secrets.h 会让所有源文件自动看到这些宏，
 * 所以这里改完不需要动任何 .cpp。
 */

#ifndef SECRETS_H
#define SECRETS_H

/* ---- WiFi（2.4GHz，ESP32-S3 不支持 5GHz） ---- */
#define WIFI_SSID           "你的WiFi名称"
#define WIFI_PASSWORD       "你的WiFi密码"

/* ---- 百度智能云语音技术（语音识别 + 语音合成）----
 * 申请地址：https://console.bce.baidu.com/ai/#/ai/speech/app/list
 * 创建「语音技术」应用后即可拿到这三个值 */
#define BAIDU_APP_ID        "你的APPID"
#define BAIDU_APP_KEY       "你的APIKEY"
#define BAIDU_SECRET_KEY    "你的SECRETKEY"

/* ---- 心知天气（实时天气 + 4 天预报）----
 * 申请地址：https://console.seniverse.com/
 * 免费版选「免费版」即可，注意免费版只支持部分城市 */
#define SENIVERSE_API_KEY   "你的心知天气API_KEY"
#define WEATHER_CITY        "xian"

/* ---- MiniMax 大模型（小智的回答生成）----
 * 申请地址：https://platform.minimaxi.com/  → 账户管理 → 接口密钥
 * 由 scripts/patch_minimax.py 在编译前自动写进 baidu-xiaozhi 库，无需手动改库文件 */
#define MINIMAX_API_KEY     "你的MiniMax_API_KEY"

#endif // SECRETS_H
