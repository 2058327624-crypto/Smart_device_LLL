# 多功能智能终端 (Smart Device LLL)

基于 **ESP32-S3** 的多功能智能终端。一块 320×240 触摸屏 + 语音助手 + SD 卡音乐播放器 + 天气/时钟/日历，全部跑在 FreeRTOS 上。

![平台](https://img.shields.io/badge/Platform-ESP32--S3-blue)
![框架](https://img.shields.io/badge/Framework-Arduino-00979D)
![UI](https://img.shields.io/badge/UI-LVGL%208.3-6C3FC5)
![构建](https://img.shields.io/badge/Build-PlatformIO-orange)

---

## 目录

- [功能特性](#功能特性)
- [硬件清单](#硬件清单)
- [接线说明](#接线说明)
- [软件架构](#软件架构)
- [快速开始](#快速开始)
- [使用说明](#使用说明)
- [项目结构](#项目结构)
- [已知限制](#已知限制)

---

## 功能特性

| 页面 | 功能 |
| --- | --- |
| 🏠 主页 | 实时时钟 + 日期，WiFi 连接状态，八个功能入口 |
| 🤖 小智 | 语音助手：按住开关说话 → 百度语音识别 → 大模型问答 → TTS 播报 |
| 🎵 音乐 | 扫描 SD 卡根目录的 MP3，滚轮选曲、播放/暂停、上一首/下一首、音量调节 |
| ⚙️ 设置 | WiFi 扫描配网（滚轮选热点 + 软键盘输密码）、屏幕亮度调节 |
| 🎮 游戏 | 「羊了个羊」消除小游戏（来自 [lvgl-games](https://github.com/CaddonThaw/lvgl-games)） |
| 📅 日历 | LVGL 日历控件，自动定位到当天，可翻月 |
| 🌤 天气 | 心知天气 API，实时天气 + 未来 4 天预报，5 分钟自动刷新 |
| 📄 声明 | 项目说明页 |
| 🔌 串口 | 与 PC 双向串口通信，屏幕软键盘发送消息，接收区显示回传内容 |

**技术要点**

- **8 个 FreeRTOS 任务**并行调度，LVGL 与网络/音频/SD 完全解耦
- **跨任务 UI 更新**统一走 `lv_async_call()`，避免 LVGL 线程安全问题
- **SD 总线互斥锁**（递归互斥量），三处并发访问 SD 互不打架
- **SPI 总线分离**：屏幕走 SPI2，SD 卡走 SPI3（HSPI），两者不抢总线
- **双 I2S 通道**：麦克风输入与功放输出分属 I2S0 / I2S1

---

## 硬件清单

| 元件 | 型号 | 数量 | 说明 |
| --- | --- | --- | --- |
| 主控 | **ESP32-S3-DevKitC-1** (N16R8) | 1 | 16MB Flash + 8MB **Octal PSRAM** |
| 屏幕 | **ILI9341**  SPI TFT，320×240 | 1 | 带 **XPT2046** 电阻触摸 |
| 麦克风 | **INMP441** | 1 | I2S 数字麦克风 |
| 功放 | **NS4168** | 1 | I2S 数字功放，接 4Ω/8Ω 喇叭 |
| 喇叭 | 4Ω 3W 或 8Ω 1W | 1 | — |
| SD 卡模块 | MicroSD SPI 转接板 | 1 | 独立 SPI 总线 |
| SD 卡 | MicroSD 卡（FAT32） | 1 | 存放 MP3 |

> ⚠️ **务必选 Octal PSRAM（R8）版本**。`platformio.ini` 里配置了 `memory_type = qio_opi`，换成 Quad PSRAM（R2）的板子会启动失败。

---

## 接线说明

### 整体连线框图

```
                        ┌──────────────────────────────┐
                        │      ESP32-S3-DevKitC-1      │
                        │         (N16R8)              │
                        └──────────────────────────────┘
                                       │
        ┌──────────────────┬───────────┼───────────┬──────────────────┐
        │                  │           │           │                  │
   ┌────┴─────┐      ┌─────┴────┐ ┌────┴────┐ ┌────┴─────┐      ┌─────┴──────┐
   │ ILI9341  │      │ XPT2046  │ │ INMP441 │ │NS4168 │      │  MicroSD   │
   │  屏幕    │      │  触摸    │ │  麦克风 │ │  功放    │      │   模块     │
   └──────────┘      └──────────┘ └─────────┘ └────┬─────┘      └────────────┘
    SPI2 (共用)        SPI2 独立      I2S0 输入     I2S1 输出        SPI3 (HSPI)
                                                      │
                                                   ┌──┴──┐
                                                   │喇叭 │
                                                   └─────┘
```

### 1. ILI9341 显示屏 + XPT2046 触摸（SPI2，与屏幕共用）

| ILI9341 / XPT2046 引脚 | ESP32-S3 GPIO | 说明 |
| :--- | :---: | :--- |
| VCC | **3V3** | 逻辑供电 |
| GND | **GND** | — |
| CS | **4** | 屏幕片选 |
| RESET | **5** | 屏幕复位 |
| DC / RS | **6** | 数据/命令选择 |
| MOSI / SDI | **7** | SPI 数据 |
| SCK / CLK | **15** | SPI 时钟 |
| MISO / SDO | **17** | SPI 数据回读 |
| LED / BL | **16** | 背光，接 **LEDC PWM** 调亮度 |
| T_CLK | **15** | 触摸时钟（与屏幕 SCK 共用） |
| T_CS | **8** | 触摸片选 |
| T_DIN | **7** | 触摸数据（与屏幕 MOSI 共用） |
| T_DO | **17** | 触摸数据（与屏幕 MISO 共用） |
| T_IRQ | 不接 | 代码用轮询，不用中断 |

> 屏幕和触摸共用 SCK/MOSI/MISO 三条线，靠 **CS 片选**区分（屏幕 CS=4，触摸 CS=8）。

### 2. INMP441 麦克风（I2S0，输入）

| INMP441 | ESP32-S3 GPIO | 说明 |
| :--- | :---: | :--- |
| VDD | **3V3** | ⚠️ 只能接 3.3V |
| GND | **GND** | — |
| SCK / BCLK | **11** | 位时钟 `I2S_SCK` |
| WS / LRCL | **13** | 声道时钟 `I2S_WS` |
| SD / DOUT | **12** | 数据输出 `I2S_SD` |
| L/R | **GND** | 接地选**左声道** |
| CHIPEN | 接 3V3 或不接 | 默认使能 |

> `L/R` 悬空会随机选声道，务必接地。录音质量差时优先检查这里和电源退耦。

### 3. NS4168 功放（I2S1，输出）

| MAX98357A | ESP32-S3 GPIO | 说明 |
| :--- | :---: | :--- |
| VIN | **5V** | ⚠️ 强烈建议接 5V，见下方提示 |
| GND | **GND** | — |
| BCLK | **40** | 位时钟 `I2S_BCLK` |
| LRC / LRCLK | **41** | 声道时钟 `I2S_LRC` |
| DIN | **39** | 数据输入 `I2S_DOUT` |
| SD / SHDN | 不接 | 内部上拉，默认使能 |
| GAIN | 不接 | 默认 9dB；接 GND=15dB，接 VDD=3dB |
| OUT+ / OUT− | 接喇叭 | 差分输出，**不要**接地 |

> **供电提示**：本项目把音量上限锁在 `AUDIO_VOLUME_MAX = 10`（库范围 0~21），原因就是 3.3V 供电余量不足 —— 音量开大时功放会把 3.3V 轨拉垮，连带 SD 卡掉线。把功放 VIN 接到 **5V** 可以显著改善。

### 4. MicroSD 卡模块（SPI3 / HSPI，独立）

| SD 模块 | ESP32-S3 GPIO | 说明 |
| :--- | :---: | :--- |
| VCC | **3V3** | 部分模块支持 5V，优先 3.3V |
| GND | **GND** | — |
| CS | **10** | 片选 `SD_CS` |
| MOSI | **47** | `SD_MOSI` |
| MISO | **18** | `SD_MISO` |
| SCK | **21** | `SD_SCLK` |

> SD 卡**单独挂在 SPI3**，和屏幕的 SPI2 物理隔离。这样音频解码读卡时不会和 LVGL 刷屏抢总线，是画面不卡顿的关键。

### 引脚占用总览

```
GPIO  4  ── TFT_CS          GPIO 18 ── SD_MISO
GPIO  5  ── TFT_RST         GPIO 21 ── SD_SCLK
GPIO  6  ── TFT_DC          GPIO 39 ── I2S_DOUT   (功放 DIN)
GPIO  7  ── TFT_MOSI/T_DIN  GPIO 40 ── I2S_BCLK   (功放 BCLK)
GPIO  8  ── TOUCH_CS        GPIO 41 ── I2S_LRC    (功放 LRC)
GPIO 10  ── SD_CS           GPIO 43 ── UART TX    (串口页，勿占用)
GPIO 11  ── I2S_SCK         GPIO 44 ── UART RX    (串口页，勿占用)
GPIO 12  ── I2S_SD          GPIO 33~37 ── Octal PSRAM 占用，勿用
GPIO 13  ── I2S_WS
GPIO 15  ── TFT_SCLK/T_CLK
GPIO 16  ── TFT_BL (背光 PWM)
GPIO 17  ── TFT_MISO/T_DO
GPIO 47  ── SD_MOSI
```

---

## 软件架构

### FreeRTOS 任务

| 任务 | 优先级 | 栈 (字) | 周期 | 职责 |
| :--- | :---: | :---: | :--- | :--- |
| `audio_task` | 5 | 8192 | 1 ms | 音频解码播放，执行播放/暂停/音量请求 |
| `weather_task` | 5 | 4800 | 5 min | 拉取天气 API，刷新天气页 |
| `sd_task` | 4 | 4096 | 5 s | 监测 SD 卡是否掉线 |
| `xiaozhi_task` | 4 | 4500 | 1 ms | 语音助手对话（开关打开时才跑） |
| `ui_task` | 3 | 4500 | 1 ms | `lv_timer_handler()` + 时钟/播放开关同步 |
| `wifi_task` | 3 | 4500 | 事件驱动 | 扫描热点、连接 WiFi、更新状态显示 |
| `time_task` | 3 | 4500 | 1 s | 刷新主页时钟日期 |
| `uart_task` | 3 | 4500 | 1 s | 串口收发 |

### 跨任务通信

整个项目**没有任何任务直接操作另一个任务的资源**，全部靠两种机制解耦：

**1. `lv_async_call()` —— 非 UI 任务更新界面**

LVGL 不是线程安全的，只有 `ui_task` 能碰控件。`wifi_task`、`weather_task`、`time` 等需要改界面时，把回调和数据指针投递给 LVGL，由 `ui_task` 在 `lv_timer_handler()` 里统一执行：

```cpp
// wifi_task 里更新 WiFi 状态标签
char* txt = (char*)malloc(128);
sprintf(txt, "WiFi:已连接\nIP:%s", ip.toString().c_str());
lv_async_call(update_wifi_label_cb, txt);   // 交给 ui_task 执行
```

**2. 标志位 —— 事件跨任务通知**

`g_wifi_scan_req`、`g_wifi_connect_req`、`g_audio_cmd` 等 `volatile` 标志位，生产者只置位，消费者只清位：

```cpp
// UI 回调（运行在 ui_task）
audio_request_play(idx, 0);      // 只是置个标志，立刻返回

// audio_task 每轮检查并执行
void audio_process_requests(void) { /* 取走标志 → connecttoSD() */ }
```

### SD 卡并发保护

音频播放、音乐列表扫描、音乐加载三处会同时访问 SD 卡，用**递归互斥锁**保护（`src/sd_card.h`）：

```cpp
SD_LOCK();                          // xSemaphoreTakeRecursive
bool ok = audio.connecttoSD(path);  // 整段持有锁
SD_UNLOCK();
```

---

## 快速开始

### 1. 准备环境

- [PlatformIO](https://platformio.org/install)（VS Code 插件或 CLI 均可）
- 一块 **ESP32-S3-DevKitC-1 (N16R8，带 Octal PSRAM)**

### 2. 克隆并配置凭据

```bash
git clone https://github.com/2058327624-crypto/Smart_device_LLL
cd Smart_device_LLL

# 从模板生成私有凭据文件
cp include/secrets.example.h include/secrets.h
```

然后编辑 `include/secrets.h`，填入你自己的值：

| 宏 | 用途 | 获取方式 |
| :--- | :--- | :--- |
| `WIFI_SSID` / `WIFI_PASSWORD` | WiFi 连接 | 注意必须在 **2.4GHz** 频段 |
| `BAIDU_APP_ID` / `BAIDU_APP_KEY` / `BAIDU_SECRET_KEY` | 百度语音识别 + 合成 | [百度智能云](https://console.bce.baidu.com/ai/#/ai/speech/app/list) 创建「语音技术」应用 |
| `SENIVERSE_API_KEY` | 心知天气 | [心知天气控制台](https://console.seniverse.com/) |
| `WEATHER_CITY` | 天气城市 | 拼音，如 `xian`、`beijing` |
| `MINIMAX_API_KEY` | 小智的回答生成 | [MiniMax 开放平台](https://platform.minimaxi.com/) → 账户管理 → 接口密钥 |

> `include/secrets.h` 已在 `.gitignore` 中，**不会**被提交。`platformio.ini` 通过 `-include secrets.h` 全局注入这些宏，所以不用改任何 `.cpp`。

**关于 `MINIMAX_API_KEY`**：`baidu-xiaozhi` 库把 MiniMax key 写死在它自己的头文件里，而且没有 `#ifndef` 保护，`-D` 覆盖不了、只能改文件——可库文件在 `.pio/libdeps/` 下，删 `.pio` 或 `pio pkg update` 就会丢失，丢了以后**语音助手会静默失效**（界面正常、开关能开，但永远返回 `<error>`，只在串口报错）。

所以本项目用 `scripts/patch_minimax.py` 在编译前自动把这个 key 注入进去，你只需要填 `secrets.h`，不用手改库文件。脚本是幂等的，key 没变时不会重复写盘。

### 语音助手的数据流

```
麦克风(INMP441) ──I2S0──▶ 百度语音识别 ──▶ MiniMax 大模型 ──▶ 百度语音合成 ──▶ 功放(MAX98357A)
                            BAIDU_*_KEY      MINIMAX_API_KEY      BAIDU_*_KEY
```

四段链路任意一个 key 没配好，都会表现为「开关能开但没反应」，排查时先看串口日志。

### 3. 编译烧录

```bash
pio run              # 编译
pio run -t upload    # 烧录（首次建议先按住 BOOT 再点 RST 进下载模式）
pio device monitor   # 查看串口日志（115200 波特率）
```

### 4. 准备 SD 卡

把 MP3 文件**放在 SD 卡根目录**（不递归子目录），格式化为 **FAT32**：

```
SD卡根目录/
├── 晴天.mp3
├── 稻香.mp3
└── 起风了.mp3
```

最多识别 **32 首**，文件名最长 63 字符。

---

## 使用说明

| 操作 | 说明 |
| :--- | :--- |
| **切换页面** | 主页八个图标点击进入，各页右上角返回图标回主页 |
| **配网** | 设置页 → 点 SSID 输入框 → 滚轮选热点 → 弹键盘输密码 → 点 ✓ |
| **放音乐** | 音乐页 → 滚轮选曲（选中即加载）→ 打开播放开关出声音 |
| **语音助手** | 小智页 → 打开开关 → 对着麦克风说话 → 自动识别并语音回答 |
| **看天气** | WiFi 连上后自动拉取，5 分钟刷新一次 |
| **串口通信** | 串口页 → 软键盘输入 → 发送到 PC；PC 发来的内容显示在接收区 |

> 串口页和 `pio device monitor` 共用同一个 UART0，调试时注意别互相干扰。

---

## 项目结构

```
Smart_device_LLL/
├── platformio.ini              # 构建配置：引脚宏、库依赖
├── .gitignore                  # 已忽略 secrets.h / .pio / .claude
├── include/
│   ├── lv_conf.h               # LVGL 配置
│   ├── secrets.example.h       # 凭据模板
│   └── secrets.h               # 真实凭据
├── scripts/
│   └── patch_minimax.py        # 编译前自动注入 MiniMax key 到 baidu-xiaozhi 库
└── src/
    ├── main.cpp                # setup()：初始化各模块 + 创建 8 个任务
    ├── screen.cpp/.h           # TFT 驱动 + LVGL 显示/触摸移植层
    ├── My_Wifi.cpp/.h          # WiFi 连接、扫描、NTP 对时
    ├── weather.cpp/.h          # 心知天气 API + 天气页更新
    ├── My_audio.cpp/.h         # 音频播放、MP3 扫描、跨任务请求队列
    ├── My_xiaozhi.cpp/.h       # 语音助手状态机
    ├── sd_card.cpp/.h          # SD 卡挂载 + 递归互斥锁
    ├── rtos/                   # 各 FreeRTOS 任务
    │   ├── ui_task.cpp
    │   ├── audio_task.cpp
    │   ├── wifi_task.cpp
    │   ├── xiaozhi_task.cpp
    │   ├── weather_task.cpp
    │   ├── time_task.cpp
    │   ├── uart_task.cpp
    │   └── sd_task.cpp
    └── ui/                     # SquareLine Studio 生成的 LVGL 界面
        ├── ui.c/.h             # 界面总入口 + 控件句柄
        ├── ui_events.c/.h      # 事件回调（本项目手写，非生成）
        ├── screens/            # 9 个页面
        ├── components/ fonts/ images/
        ├── filelist.txt        # SquareLine 源文件清单
        └── CMakeLists.txt
```

### 依赖库

由 PlatformIO 自动安装（见 `platformio.ini`）：

| 库 | 版本 | 用途 |
| :--- | :--- | :--- |
| `lvgl/lvgl` | 8.3.11 | 图形界面 |
| `bodmer/TFT_eSPI` | 2.5.0 | ILI9341 驱动 |
| `bblanchon/ArduinoJson` | ^6.21.4 | 天气 JSON 解析 |
| `esphome/ESP32-audioI2S` | 2.0.6 | MP3 解码播放 |
| `gilmaimon/ArduinoWebsockets` | ^0.5.3 | 语音助手通信 |
| `plageoj/UrlEncode` | ^1.0.1 | 天气 URL 编码 |
| `CaddonThaw/baidu-xiaozhi` | git | 百度语音识别 + 合成封装 |
| `CaddonThaw/lvgl-games` | git | 羊了个羊小游戏 |

---

## 已知限制

- **MP3 只扫根目录**，不支持子文件夹（见 `music_scan()` 注释）
- **最多 32 首**曲目，超出部分不显示
- **音量上限锁在 10/21**，是供电限制而非软件限制，改 `AUDIO_VOLUME_MAX` 前请先改善供电
- **SD 卡掉线**：如果拔卡或供电不足，`sd_task` 会打印告警，但**不会自动重新挂载**
- **天气 API 免费版**仅支持部分城市，且每日调用次数有限
- **语音助手**依赖百度 + MiniMax 在线 API，断网不可用；且与音乐播放互斥（播放音乐时语音助手暂停）
- **首次上电** WiFi 连接是阻塞的（最多 10 秒），期间界面不响应

---

## 致谢

- UI 由 [SquareLine Studio](https://squareline.io/) 生成
- [lvgl-games](https://github.com/CaddonThaw/lvgl-games) 提供小游戏
- [baidu-xiaozhi](https://github.com/CaddonThaw/baidu-xiaozhi) 提供语音能力封装

## 许可

本项目仅供学习交流使用。第三方库版权归各自作者所有。
