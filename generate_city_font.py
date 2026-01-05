#!/usr/bin/env python3
"""
生成包含城市名称的中文字体
需要安装 lv_font_conv: pip install lvgl-font-converter
或从 https://github.com/lvgl/lv_font_conv/releases 下载
"""

import subprocess
import sys
import os

# 字体参数
FONT_SIZE = 20  # 字体大小
BPP = 4  # 位深度
FONT_TTF = "C:/Windows/Fonts/msyh.ttc"  # 微软雅黑字体，可根据需要修改
OUTPUT_FILE = "main/display/ui_font_city.c"

# 包含的字符
# ASCII 范围
ASCII_RANGE = "0x20-0x7f"

# 额外的中文字符（城市名称等）
CHINESE_CHARS = (
    "多云晴阴雪天大中小转雨雷阵雾冰雹霾周一二三四五六日"
    "杭州北京上海广州深圳成都重庆武汉西安南京天津苏州"
    "长沙郑州东莞青岛沈阳宁波昆明合肥厦门福州大连"
    "哈尔滨长春石家庄南昌济南南宁贵阳太原海口兰州呼和浩特银川西宁拉萨乌鲁木齐"
    "今功德圆满"
)

def generate_font():
    """使用 lv_font_conv 生成字体"""

    # 构建命令
    cmd = [
        "lv_font_conv",
        f"--size {FONT_SIZE}",
        f"--bpp {BPP}",
        f"--font {FONT_TTF}",
        f"--format lvgl",
        f"--range {ASCII_RANGE}",
        f"--symbols {CHINESE_CHARS}",
        "--no-compress",
        "--no-prefilter",
        "-o", OUTPUT_FILE
    ]

    # 打印命令
    print("Generating font...")
    print("Command:", " ".join(cmd))
    print("\nCharacters:", CHINESE_CHARS)

    # 执行命令
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print("\nFont generated successfully!")
        print(f"Output file: {OUTPUT_FILE}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"\nError generating font:")
        print(f"Return code: {e.returncode}")
        print(f"Stdout: {e.stdout}")
        print(f"Stderr: {e.stderr}")
        return False
    except FileNotFoundError:
        print("\nError: lv_font_conv not found!")
        print("Please install lv_font_conv from:")
        print("  https://github.com/lvgl/lv_font_conv/releases")
        print("\nOr install via pip:")
        print("  pip install lvgl-font-converter")
        return False

def main():
    if not os.path.exists(FONT_TTF):
        print(f"Error: Font file not found: {FONT_TTF}")
        print("Please update the FONT_TTF path in this script.")
        return 1

    if generate_font():
        return 0
    else:
        return 1

if __name__ == "__main__":
    sys.exit(main())
