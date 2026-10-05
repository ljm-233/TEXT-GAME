#include "settings_codec.h"

#include <cctype>
#include <cstdint>
#include <string>

// 分享码：TS1:<base64(rle(设置文本))>
//
// 这里刻意不依赖 LevelCodec：两个格式的 RLE 不同（见下），而且分成两个
// namespace 后各自的容错策略可以独立演进，不会因为改一个动了另一个。

namespace {

// ============ RLE ============
// 格式：<count>:<char>
//
// ⚠️ 关卡那边是「<count><char>」——靠"关卡字符集里没有数字"才能定界。
// 设置文本里数字是常态（fps_limit=144 / volume=80），沿用那个格式立刻歧义：
// "144" 后面到底还有没有字符？所以这里显式插一个 ':' 当分隔符，读法是
// "先读计数、再读一个 ':'、最后无条件收下一个字节" —— 字符本身是不是数字
// 都不影响解析。
//
// 文本里出现 ':' 也无所谓：它只会作为"分隔符之后的那一个字节"被原样收下。
//
// count 有单段上限、总量也有上限：分享码是**别人粘贴进来的不可信输入**，
// 一个几百字节的码靠 RLE 能要求解码出几个 G —— 上限让最坏情况有界，
// 超了就按"解不开"返回空串。

constexpr long kMaxRunLength = 100000;                    // 单段上限
// 第一个操作数就用 size_t：否则乘法在 unsigned 里算完再拓宽，
// 值一大就悄悄溢出（clang-tidy 的 implicit-widening 就是抓这个）
constexpr std::size_t kMaxDecodedSize = std::size_t{4} * 1024 * 1024;

std::string rleEncode(const std::string& s) {
    std::string out;
    std::size_t i = 0;
    while (i < s.size()) {
        const char c = s[i];
        std::size_t j = i;
        while (j < s.size() && s[j] == c)
            ++j;
        out += std::to_string(j - i);
        out += ':';
        out += c;
        i = j;
    }
    return out;
}

std::string rleDecode(const std::string& s) {
    std::string out;
    std::size_t i = 0;
    while (i < s.size()) {
        // 计数位：必须是数字，否则这段长度字段已经坏了
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return {};
        long n = 0;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
            n = n * 10 + (s[i] - '0');
            ++i;
            if (n > kMaxRunLength)
                return {}; // 荒谬的计数，别去分配
        }
        // 分隔符：没有它说明长度字段被截断（或根本是别的格式）
        if (i >= s.size() || s[i] != ':')
            return {};
        ++i;
        if (i >= s.size())
            return {}; // ':' 之后没有字符
        const char c = s[i];
        ++i;

        if (n <= 0)
            return {}; // "0:x" 语法合法但产不出字符，一律当坏码
        if (out.size() + static_cast<std::size_t>(n) > kMaxDecodedSize)
            return {};
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
        std::uint32_t v = (std::uint8_t)in[i] << 16 | (std::uint8_t)in[i + 1] << 8 |
                          (std::uint8_t)in[i + 2];
        out += kB64[(v >> 18) & 0x3F];
        out += kB64[(v >> 12) & 0x3F];
        out += kB64[(v >> 6) & 0x3F];
        out += kB64[v & 0x3F];
        i += 3;
    }
    if (i < in.size()) {
        std::uint32_t v = (std::uint8_t)in[i] << 16;
        if (i + 1 < in.size())
            v |= (std::uint8_t)in[i + 1] << 8;
        out += kB64[(v >> 18) & 0x3F];
        out += kB64[(v >> 12) & 0x3F];
        out += (i + 1 < in.size()) ? kB64[(v >> 6) & 0x3F] : '=';
        out += '=';
    }
    return out;
}

int b64Val(char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    return -1; // '=' 在调用处提前收尾，其余一律非法
}

std::string b64Decode(const std::string& in) {
    std::string out;
    std::uint32_t buf = 0;
    int bits = 0;
    for (char c : in) {
        if (c == '=')
            break; // 与 LevelCodec 一致：补位之后的内容不再看
        int v = b64Val(c);
        if (v < 0)
            return {};
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

namespace SettingsCodec {

// TS1: 是设置分享码；TG1: 归关卡（LevelCodec），两边不能互相认领
constexpr const char* kPrefix = "TS1:";
constexpr std::size_t kPrefixLen = 4;

std::string encode(const std::string& settingsText) {
    return std::string(kPrefix) + b64Encode(rleEncode(settingsText));
}

std::string decode(const std::string& code) {
    if (code.size() < kPrefixLen)
        return {};
    if (code.compare(0, kPrefixLen, kPrefix) != 0)
        return {};

    // 粘贴来源（聊天软件、论坛）常把长码折行、塞空格，先丢掉这些空白。
    // 设置文本本身在 base64 里面，不受影响 —— 所以这里丢空白是安全的。
    std::string clean;
    for (std::size_t i = kPrefixLen; i < code.size(); ++i) {
        const char c = code[i];
        if (!std::isspace(static_cast<unsigned char>(c)))
            clean += c;
    }

    const std::string rle = b64Decode(clean);
    if (rle.empty())
        return {}; // 空 base64 / 混了非法字符
    return rleDecode(rle);
}

bool isValid(const std::string& code) {
    // 唯一权威判定就是 decode：分成两套逻辑迟早会出现"isValid 说行、
    // decode 却解不出"的静默矛盾。
    return !decode(code).empty();
}

} // namespace SettingsCodec
