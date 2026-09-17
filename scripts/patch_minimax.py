"""
编译前把 include/secrets.h 里的 MINIMAX_API_KEY 写进 baidu-xiaozhi 库的
xiaozhi_minimax.h。

为什么需要这个脚本
------------------
语音助手的回答由 MiniMax 大模型生成，key 写死在库头文件里：

    #define MiniMaxKey "MiniMaxKey"   // 上游默认是占位符

这个宏没有 #ifndef 保护，platformio.ini 的 -D 覆盖不了它，只能改文件。
但库文件在 .pio/libdeps/ 下，属于构建产物目录：删掉 .pio、执行
pio pkg update、或者换台电脑重新拉依赖，改动都会丢。丢了之后的表现是
语音助手静默失效——界面正常、开关能开，但永远返回 <error>，只有串口
日志能看出问题。所以这件事必须自动化。

工作原理
--------
在 buildprog 目标开始前，把 secrets.h 里的 key 写进库头文件。
补丁是幂等的，且只在内容真的变化时才写盘，因此不会触发无谓的重新编译。

如果 secrets.h 不存在或还没填 key，脚本只打印提示并跳过——不会让编译失败。
"""

import re
from pathlib import Path

Import("env")

PROJECT_DIR = Path(env["PROJECT_DIR"])
SECRETS_H = PROJECT_DIR / "include" / "secrets.h"
HEADER_NAME = "xiaozhi_minimax.h"

# #define MiniMaxKey "任意内容"  →  捕获前后引号，便于原地替换
MACRO_RE = re.compile(r'(#define\s+MiniMaxKey\s+")[^"]*(")')

# secrets.h 里的 MINIMAX_API_KEY
KEY_RE = re.compile(r'^\s*#define\s+MINIMAX_API_KEY\s+"([^"]*)"', re.M)

# 模板里未填写的值，出现这些就认为用户还没配置
PLACEHOLDERS = {"", "MiniMaxKey", "你的MiniMax_API_KEY"}


def read_minimax_key():
    """从 include/secrets.h 读 key，读不到返回 None。"""
    if not SECRETS_H.is_file():
        print("[patch_minimax] 未找到 include/secrets.h，跳过 MiniMax key 注入")
        print("[patch_minimax]   → 先执行: cp include/secrets.example.h include/secrets.h")
        return None

    match = KEY_RE.search(SECRETS_H.read_text(encoding="utf-8"))
    if not match:
        print("[patch_minimax] secrets.h 里没有 MINIMAX_API_KEY，跳过")
        return None

    key = match.group(1)
    if key in PLACEHOLDERS:
        print("[patch_minimax] MINIMAX_API_KEY 还是占位符，跳过（语音助手将不可用）")
        return None

    return key


def find_header():
    """在 .pio/libdeps/ 下定位 xiaozhi_minimax.h，找不到返回 None。"""
    libdeps = PROJECT_DIR / ".pio" / "libdeps"
    if not libdeps.is_dir():
        return None

    # 优先当前环境的目录，再退回全局搜索
    env_dir = libdeps / env["PIOENV"]
    root = env_dir if env_dir.is_dir() else libdeps

    for path in root.rglob(HEADER_NAME):
        return path

    if root is not libdeps:
        for path in libdeps.rglob(HEADER_NAME):
            return path

    return None


def patch_minimax(*args, **kwargs):
    """入口。签名兼容 SCons 的 (source, target, env) 回调。"""
    key = read_minimax_key()
    if key is None:
        return

    header = find_header()
    if header is None:
        print("[patch_minimax] 未找到 %s（依赖还没装好？），跳过" % HEADER_NAME)
        return

    text = header.read_text(encoding="utf-8")
    patched, count = MACRO_RE.subn(lambda m: m.group(1) + key + m.group(2), text, count=1)

    if count == 0:
        print("[patch_minimax] %s 里没有 MiniMaxKey 宏，库结构可能变了" % header)
        return

    if patched == text:
        return  # 已经是目标值，不写盘，避免触碰时间戳引起全量重编

    header.write_text(patched, encoding="utf-8")
    print("[patch_minimax] 已注入 MiniMax key -> %s" % header)


# 在脚本加载时（即 platformio.ini 处理阶段）执行。
# 此时依赖已经安装完毕，改动会赶在编译开始前落盘并被正常纳入。
patch_minimax()
