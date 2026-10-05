#include "doctest.h"
#include "settings_codec.h"

#include <string>

// 设置分享码编解码（TS1:<base64(rle(设置文本))>）。
//
// 全程纯字符串处理，不碰 SFML、不碰 GL，所以这个文件在无 DISPLAY 下也能跑。
//
// 与关卡分享码（LevelCodec）最本质的差别在 RLE：设置文本里**数字是常态**
// （fps_limit=144），关卡那套「<计数><字符>」的定界前提在这里直接不成立。
// 所以下面专门钉了数字相关的用例 —— 只拿 "key=value" 的普通文本测往返，
// 是抓不住"数字被当成计数吃掉"这类静默损坏的。

namespace {

/// 一份像样的设置文本：多行、带数字、含注释行与空行
const char* kSampleSettings = "# 画面\n"
                              "resolution_index=2\n"
                              "vsync=true\n"
                              "fps_limit=144\n"
                              "\n"
                              "volume=80\n"
                              "player_name=Test\n";

/// 模拟从聊天软件里粘贴：码被折行、被塞进空格和制表符
std::string messUp(const std::string& code) {
    std::string messy;
    for (std::size_t i = 0; i < code.size(); ++i) {
        if (i == 10 || i == 25)
            messy += '\n';
        if (i == 15)
            messy += ' ';
        if (i == 30)
            messy += '\t';
        messy += code[i];
    }
    return messy;
}

} // namespace

// ============================================================
// 往返一致性
// ============================================================

TEST_CASE("设置分享码 - 编解码往返不改变设置文本") {
    const std::string original = kSampleSettings;

    const std::string code = SettingsCodec::encode(original);

    const bool hasPrefix = code.rfind("TS1:", 0) == 0; // 用的是设置前缀
    CHECK(hasPrefix);
    CHECK(SettingsCodec::decode(code) == original);
}

TEST_CASE("设置分享码 - 数字不会被 RLE 当成计数吃掉") {
    // ⚠️ 这一组是重点：关卡那套 RLE 在这里会把 "144" 解释成计数，
    //    于是设了 144 帧的玩家导入后拿到别的值，而且一路不报错。
    const char* cases[] = {
        "0",                        // 单个数字
        "144",                      // 纯数字串
        "111111111",                // 一长串相同数字
        "fps_limit=144\nvolume=80", // 真实设置行
        "k=1\nk2=22\nk3=333",       // 越来越多的数字
        "=:=:",                     // 分隔符本身出现在文本里
        ":::",                      // 全是分隔符
    };

    for (const char* c : cases) {
        const std::string original = c;
        CAPTURE(original);
        CHECK(SettingsCodec::decode(SettingsCodec::encode(original)) == original);
    }
}

TEST_CASE("设置分享码 - 含中文与换行的设置文本能往返") {
    const std::string original = "player_name=小明\n"
                                 "# 语言\n"
                                 "language=简体中文\n"
                                 "note=换行\n也在值里面\n";

    const std::string code = SettingsCodec::encode(original);

    CHECK(SettingsCodec::decode(code) == original);
    CHECK(SettingsCodec::isValid(code));
}

TEST_CASE("设置分享码 - 码里的换行与空格会被忽略") {
    const std::string original = kSampleSettings;
    const std::string code = SettingsCodec::encode(original);

    // 折行/空格只影响码本身，设置在 base64 里面，必须原样回来
    CHECK(SettingsCodec::decode(messUp(code)) == original);
}

// ============================================================
// 空输入与非法输入
// ============================================================

TEST_CASE("设置分享码 - 空输入与只有前缀都安全") {
    const std::string emptyCode = SettingsCodec::encode("");
    CHECK(emptyCode == "TS1:"); // 编码确定：空文本就是一个空壳码

    CHECK(SettingsCodec::decode("").empty());
    CHECK(SettingsCodec::decode("TS1:").empty());
    CHECK_FALSE(SettingsCodec::isValid("TS1:"));

    // 空文本的码与"解不开"长得一样 —— 与 LevelCodec 的取舍一致：
    // 空设置本来也没有导入的意义，宁可当成坏码。
    CHECK(SettingsCodec::decode(SettingsCodec::encode("")).empty());
}

TEST_CASE("设置分享码 - 前缀不对时一概不认") {
    const char* bad[] = {
        "TS1", // 比前缀还短
        "XXXX:abcd",
        "ts1:abcd", // 大小写敏感
        "TS2:abcd",
        " TG1:abcd",    // 前面带空白
        "TG1:MToxYQ==", // 关卡分享码：设置这边不能去认领
    };

    for (const char* c : bad) {
        CAPTURE(c);
        CHECK(SettingsCodec::decode(c).empty());
    }

    // 反过来也必须成立：设置码不能长成关卡码的样子，否则 TG1 那边会误收
    const std::string settingsCode = SettingsCodec::encode(kSampleSettings);
    const bool looksLikeLevel = settingsCode.rfind("TG1:", 0) == 0;
    CHECK_FALSE(looksLikeLevel);
}

TEST_CASE("设置分享码 - base64 里混入非法字符时拒绝") {
    const char* bad[] = {
        "TS1:!!!!",
        "TS1:abcd!!!!", // 前面合法、后面混入非法字符
        "TS1:####",           "TS1:****",
        "TS1:中文不是base64", // 非 ASCII 字节同样非法
    };

    for (const char* c : bad) {
        CAPTURE(c);
        CHECK(SettingsCodec::decode(c).empty());
        CHECK_FALSE(SettingsCodec::isValid(c));
    }
}

TEST_CASE("设置分享码 - 被截断的码不会被当成有效设置") {
    const std::string original = kSampleSettings;
    const std::string code = SettingsCodec::encode(original);

    // 砍一半之后要么解不出、要么解出来跟原文不一样 —— 总之不能"看起来成功"
    const std::string half = code.substr(0, code.size() / 2);
    CHECK(SettingsCodec::decode(half) != original);

    // 只留前缀同样是失败的
    CHECK(SettingsCodec::decode(code.substr(0, 4)).empty());
}

TEST_CASE("设置分享码 - RLE 长度字段损坏时安全拒绝") {
    // 下面这些都是"前缀对 + base64 合法"，坏在 RLE 语法上。
    // 字符串都写在注释里，方便手算核对。
    const char* bad[] = {
        "TS1:OTk5OTk5OTk5OTp4", // "9999999999:x" —— 计数远超上限
        "TS1:MDp4",             // "0:x" —— 计数为 0，产不出字符
        "TS1:MTAwMDAxOng=",     // "100001:x" —— 刚过单段上限
        "TS1:YWJj",             // "abc" —— 开头就不是计数
        "TS1:MTI6",             // "12:" —— ':' 后面没有字符（被截断）
        "TS1:MTJ4",             // "12x" —— 缺 ':' 分隔符（关卡旧格式）
        "TS1:MWE=",             // "1a" —— 关卡旧格式，这里不接受
    };

    for (const char* c : bad) {
        CAPTURE(c);
        CHECK(SettingsCodec::decode(c).empty());
        CHECK_FALSE(SettingsCodec::isValid(c));
    }
}

TEST_CASE("设置分享码 - 超长输入不崩且能往返") {
    // 40 万字符：既有能压的重复段，也有几乎压不动的数字交替段
    std::string big;
    big.reserve(400000);
    for (int i = 0; i < 20000; ++i) {
        big += "key=value\n";
        big += "0123456789";
    }

    const std::string code = SettingsCodec::encode(big);

    const bool hasPrefix = code.rfind("TS1:", 0) == 0;
    CHECK(hasPrefix);
    CHECK(SettingsCodec::decode(code) == big);
}

TEST_CASE("设置分享码 - isValid 与 decode 的结论一致") {
    const char* samples[] = {
        "",          // 空
        "TS1:",      // 只有前缀
        "XXXX:abcd", // 前缀不对
        "TS1:!!!!",  // base64 非法
        "TS1:YWJj",  // base64 合法但不是 RLE
        "TS1:MDp4",  // RLE 计数为 0
    };

    for (const char* c : samples) {
        CAPTURE(c);
        const bool valid = SettingsCodec::isValid(c);
        const bool decodable = !SettingsCodec::decode(c).empty();
        CHECK(valid == decodable);
    }

    // 好码两边都必须认
    const std::string good = SettingsCodec::encode(kSampleSettings);
    const bool goodValid = SettingsCodec::isValid(good);
    const bool goodDecodable = !SettingsCodec::decode(good).empty();
    CHECK(goodValid);
    CHECK(goodDecodable);
}
