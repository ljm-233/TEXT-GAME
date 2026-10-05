#pragma once
#include <string>

// 把「设置文本」编码成可分享/可粘贴的短码。
//
// 设置文本就是若干行 "key=value"（与 config 文件同格式）。
// 格式：TS1:<base64(rle(text))>  —— 与关卡分享码同一套思路，只是前缀不同：
// 前缀必须区分开，否则一段设置文本会被关卡那边当成关卡去解（反之亦然）。
//
// 为什么不直接复用 LevelCodec：关卡 RLE 用「<计数><字符>」定界，前提是
// 关卡字符集里没有数字；设置文本里数字满地都是（fps_limit=144），
// 那个前提不成立。这里的 RLE 多一个 ':' 分隔符，见 .cpp 顶部。
namespace SettingsCodec {

/// 把「设置文本」编码成可分享/可粘贴的短码。
std::string encode(const std::string& settingsText);

/// 解不开 / 前缀不对 / 长度不对 一律返回**空串**（不要抛异常）。
std::string decode(const std::string& code);

/// 只做格式与可解性校验。
bool isValid(const std::string& code);

} // namespace SettingsCodec
