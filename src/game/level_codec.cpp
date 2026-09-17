#include "level_codec.h"
#include <cctype>
#include <cstdint>
#include <sstream>

namespace {

// ============ RLE ============
// 格式：<count><char>，count 是十进制数字，char 是非数字字符
// 关卡字符集 (# P E C J S M V K L ^ space) 里没有数字，不会冲突

std::string rleEncode(const std::string& s) {
    if (s.empty()) return {};
    std::ostringstream out;
    std::size_t i = 0;
    while (i < s.size()) {
        char c = s[i];
        std::size_t j = i;
        while (j < s.size() && s[j] == c) ++j;
        out << (j - i) << c;
        i = j;
    }
    return out.str();
}

std::string rleDecode(const std::string& s) {
    std::string out;
    std::size_t i = 0;
    while (i < s.size()) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return {};
        int n = 0;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
            n = n * 10 + (s[i] - '0');
            ++i;
            if (n > 100000) return {};
        }
        if (i >= s.size()) return {};
        char c = s[i];
        ++i;
        out.append(static_cast<std::size_t>(n), c);
    }
    return out;
}

// ============ Base64 ============
const char* kB64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string b64Encode(const std::string& in) {
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    std::size_t i = 0;
    while (i + 2 < in.size()) {
        std::uint32_t v = (std::uint8_t)in[i] << 16
                        | (std::uint8_t)in[i + 1] << 8
                        | (std::uint8_t)in[i + 2];
        out += kB64[(v >> 18) & 0x3F];
        out += kB64[(v >> 12) & 0x3F];
        out += kB64[(v >> 6) & 0x3F];
        out += kB64[v & 0x3F];
        i += 3;
    }
    if (i < in.size()) {
        std::uint32_t v = (std::uint8_t)in[i] << 16;
        if (i + 1 < in.size()) v |= (std::uint8_t)in[i + 1] << 8;
        out += kB64[(v >> 18) & 0x3F];
        out += kB64[(v >> 12) & 0x3F];
        out += (i + 1 < in.size()) ? kB64[(v >> 6) & 0x3F] : '=';
        out += '=';
    }
    return out;
}

int b64Val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::string b64Decode(const std::string& in) {
    std::string out;
    std::uint32_t buf = 0;
    int bits = 0;
    for (char c : in) {
        if (c == '=') break;
        int v = b64Val(c);
        if (v < 0) return {};
        buf = (buf << 6) | static_cast<std::uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += static_cast<char>((buf >> bits) & 0xFF);
        }
    }
    return out;
}

} // namespace

namespace LevelCodec {

constexpr const char* kPrefix = "TG1:";

std::string encode(const std::string& levelText) {
    std::string rle = rleEncode(levelText);
    return std::string(kPrefix) + b64Encode(rle);
}

std::string decode(const std::string& code) {
    if (code.size() < 4) return {};
    if (code.compare(0, 4, kPrefix) != 0) return {};

    std::string clean;
    for (char c : code.substr(4))
        if (!std::isspace(static_cast<unsigned char>(c))) clean += c;

    std::string rle = b64Decode(clean);
    if (rle.empty()) return {};
    return rleDecode(rle);
}

bool isValid(const std::string& code) {
    return !decode(code).empty();
}

} // namespace LevelCodec