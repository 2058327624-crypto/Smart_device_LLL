#!/usr/bin/env python3
"""把 SquareLine assets 里的天气图标 PNG 转成 LVGL 8 的 C 数组。

为什么不直接在 SquareLine 里导入？
    因为界面上的图标是运行时按天气现象切换的（晴/阴/雨/雪/风/暴风雨），
    不是设计期摆好的固定图片。放进 SquareLine 的话，每次重新导出都有
    被覆盖 / 漏导入的风险（本项目已经被这个坑过两次）。

输出格式与 SquareLine 生成的完全一致，已用 assets/多云.png 逐字节比对验证：
    LV_IMG_CF_TRUE_COLOR_ALPHA, bpp=32
    每像素 3 字节：RGB565 小端 + 1 字节 alpha
    LV_COLOR_DEPTH = 16（见 include/lv_conf.h）

用法：
    python scripts/gen_weather_icons.py

依赖：Pillow（pip install Pillow）
"""

import os
import sys

from PIL import Image

# 图标源目录（SquareLine 工程里的 assets）
ASSETS_DIR = r"D:/CobeMXproject/LLL/assets"

# (源文件名, 生成的 C 符号名)
ICONS = [
    ("晴.png",     "ui_img_weather_sunny"),
    ("多云.png",   "ui_img_weather_cloudy"),
    ("阴.png",     "ui_img_weather_overcast"),
    ("雨.png",     "ui_img_weather_rain"),
    ("雪.png",     "ui_img_weather_snow"),
    ("风.png",     "ui_img_weather_wind"),
    ("暴风雨.png", "ui_img_weather_storm"),
]

# 放到 view/ 而不是 ui/ —— ui/ 是 SquareLine 的导出目录
OUT_C = "src/view/weather_icons.c"
OUT_H = "src/view/weather_icons.h"

PER_LINE = 12  # 每行输出的字节数


def rgb565_le_bytes(r, g, b):
    """RGB565，小端：低字节在前"""
    v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    return (v & 0xFF, v >> 8)


def convert(path):
    im = Image.open(path).convert("RGBA")
    w, h = im.size
    data = bytearray()
    for r, g, b, a in im.get_flattened_data():
        lo, hi = rgb565_le_bytes(r, g, b)
        data += bytes((lo, hi, a))
    return w, h, data


def fmt_bytes(data):
    lines = []
    for i in range(0, len(data), PER_LINE):
        chunk = data[i:i + PER_LINE]
        lines.append("    " + ",".join("0x%02x" % b for b in chunk) + ",")
    return "\n".join(lines)


def main():
    if not os.path.isdir(ASSETS_DIR):
        sys.exit("找不到资源目录: %s" % ASSETS_DIR)

    c_parts = []
    h_decls = []

    for fname, sym in ICONS:
        src = os.path.join(ASSETS_DIR, fname)
        if not os.path.isfile(src):
            sys.exit("缺少图标: %s" % src)

        w, h, data = convert(src)
        print("%-12s %dx%d  %d bytes -> %s" % (fname, w, h, len(data), sym))

        c_parts.append(
            "// %s  (%dx%d)\n"
            "const LV_ATTRIBUTE_MEM_ALIGN uint8_t %s_data[] = {\n%s\n};\n"
            "const lv_img_dsc_t %s = {\n"
            "    .header.always_zero = 0,\n"
            "    .header.w = %d,\n"
            "    .header.h = %d,\n"
            "    .data_size = %d,\n"
            "    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n"
            "    .data = %s_data,\n"
            "};\n"
            % (fname, w, h, sym, fmt_bytes(data), sym, w, h, len(data), sym)
        )
        h_decls.append("LV_IMG_DECLARE(%s);" % sym)

    with open(OUT_C, "w", encoding="utf-8", newline="\n") as f:
        f.write(
            "// 由 scripts/gen_weather_icons.py 生成，请勿手改。\n"
            "// 源图: %s\n"
            "// 重新生成: python scripts/gen_weather_icons.py\n"
            "//\n"
            "// 注意: 这个文件刻意不放在 src/ui/images/ 下 —— 那里是 SquareLine 的\n"
            "// 输出目录，重新导出时不在 filelist.txt 里的文件会被清掉。\n"
            "\n"
            '#include "weather_icons.h"\n\n'
            "%s" % (ASSETS_DIR, "\n".join(c_parts))
        )

    with open(OUT_H, "w", encoding="utf-8", newline="\n") as f:
        f.write(
            "// 由 scripts/gen_weather_icons.py 生成，请勿手改。\n"
            "\n"
            "#ifndef WEATHER_ICONS_H\n"
            "#define WEATHER_ICONS_H\n"
            "\n"
            "#include <lvgl.h>\n"
            "\n"
            "%s\n"
            "\n"
            "#endif // WEATHER_ICONS_H\n" % "\n".join(h_decls)
        )

    print("\n已写出 %s 和 %s" % (OUT_C, OUT_H))


if __name__ == "__main__":
    main()
