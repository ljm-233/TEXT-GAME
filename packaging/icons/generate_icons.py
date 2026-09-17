#!/usr/bin/env python3
"""
生成游戏图标（苦力怕风格，和 window.cpp 的 makeWindowIcon 一致）。

输出：
    packaging/icons/16x16.png
    packaging/icons/32x32.png
    packaging/icons/48x48.png
    packaging/icons/64x64.png
    packaging/icons/128x128.png
    packaging/icons/256x256.png
    packaging/icons/text-game.ico  （多尺寸 Windows 图标）
    packaging/icons/text-game.png  （256x256，.desktop 用）

依赖：pip install pillow
"""

from pathlib import Path
from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parent


def draw_icon(size: int) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    s = size / 64.0   # 基于 64x64 设计

    def rect(x0, y0, x1, y1, **kw):
        d.rectangle(
            [int(x0 * s), int(y0 * s), int(x1 * s), int(y1 * s)],
            **kw,
        )

    body   = (95, 190, 95, 255)
    border = (45, 110, 45, 255)
    black  = (15, 15, 15, 255)

    # 身体
    rect(8, 8, 55, 55, fill=body, outline=border, width=max(1, int(3 * s)))

    # 眼睛
    rect(16, 20, 23, 27, fill=black)
    rect(40, 20, 47, 27, fill=black)

    # 嘴：横线
    rect(20, 36, 43, 39, fill=black)

    # 嘴：两个竖
    rect(20, 36, 23, 47, fill=black)
    rect(40, 36, 43, 47, fill=black)

    return img


def main():
    sizes = [16, 32, 48, 64, 128, 256]

    # 生成 PNG
    for s in sizes:
        img = draw_icon(s)
        out = OUT / f"{s}x{s}.png"
        img.save(out)
        print(f"[OK] {out}")

    # 主图标（256x256，.desktop 引用）
    main_icon = OUT / "text-game.png"
    draw_icon(256).save(main_icon)
    print(f"[OK] {main_icon}")

    # ICO（Windows，多尺寸）
    ico_path = OUT / "text-game.ico"
    draw_icon(256).save(
        ico_path,
        format="ICO",
        sizes=[(s, s) for s in sizes],
    )
    print(f"[OK] {ico_path}")


if __name__ == "__main__":
    main()