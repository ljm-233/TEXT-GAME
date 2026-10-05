#pragma once
#include <SFML/Graphics/Image.hpp>

/// 壁纸缩略图：把原图等比缩到"最长边不超过 maxDim"。
///
/// 为什么要自己缩而不是让 sprite 去缩放显示：壁纸原图最长边到 3840，
/// 一张解码后就是 33MB。设置里一次性列出 5 张，如果各自上传一张全尺寸
/// 纹理再靠 setScale 显示小图，显存会直接吃掉 160MB+ —— 而这些像素
/// 一个都不会真的显示出来。先在 CPU 上缩到 256，一张只剩 147KB。
///
/// 用**盒式平均**（每个目标像素取源图对应矩形块的平均值）而不是最近邻：
/// 3840 → 256 是 15 倍降采样，最近邻会明显出现锯齿与摩尔纹，
/// 而壁纸缩略图恰恰是看"这张图大概什么样"，锯齿会直接影响判断。
///
/// `sf::Image` **不是 `sf::GlResource`** —— 不需要 GL 上下文，所以这个函数
/// 能在无 DISPLAY 环境里直接单元测试（见 tests/test_wallpaper_thumbnail.cpp）。
///
/// - 源图为空 / maxDim 为 0 → 返回空图
/// - 原图本来就小于 maxDim → 原样返回（**不放大**）
/// - 宽高比保持不变，且至少 1x1
sf::Image makeThumbnail(const sf::Image& src, unsigned maxDim);

/// 壁纸缩略图的最长边。256 足够在设置里显示成 ~160x90 的行内预览，
/// 单张内存 256*256*4 = 256KB（实际按宽高比更小）。
inline constexpr unsigned kThumbnailMaxDim = 256;
