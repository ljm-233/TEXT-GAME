#pragma once
#include <string>

// 把 ASCII 关卡文本编码成可分享的短字符串
// 格式：TG1:<base64(rle(level_text))>
namespace LevelCodec {

std::string encode(const std::string& levelText);
std::string decode(const std::string& code);
bool isValid(const std::string& code);

} // namespace LevelCodec