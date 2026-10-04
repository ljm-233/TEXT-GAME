#include "doctest.h"
#include "level_codec.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// 分享码编解码（TG1:<base64(rle(关卡文本))>）。
//
// 全程纯字符串处理，不碰 SFML、不碰 GL，所以这个文件在无 DISPLAY 下也能跑。
// 这类"编解码往返"的东西最怕的是**静默损坏**：编码没报错、解码也没报错，
// 但解出来的关卡少了一块地形。所以下面大部分用例都在钉往返一致性。

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

/// 一个真实形状的小关卡（含换行与各种 tile 字符）
const char* kSampleLevel =
    "####################\n"
    "#                  #\n"
    "#  P   C C    E    #\n"
    "#                  #\n"
    "####################";

} // namespace

// ============================================================
// 往返一致性
// ============================================================

TEST_CASE("LevelCodec - 编解码往返不改变关卡") {
    const std::string original = kSampleLevel;

    const std::string code = LevelCodec::encode(original);

    CHECK(code.rfind("TG1:", 0) == 0);           // 带前缀
    CHECK(LevelCodec::decode(code) == original);
}

TEST_CASE("LevelCodec - 单字符 / 全同 / 极短输入都能往返") {
    const char* cases[] = {
        " ",                 // 一个空格
        "#",                 // 一个字符
        "################",  // 全是同一个字符
        "ab",                // 两个不同字符
        "abc",               // 长度 3（base64 正好一组）
        "abcd",              // 长度 4（base64 补一个 '='）
        "abcde",             // 长度 5（base64 补两个 '='）
    };

    for (const char* c : cases) {
        const std::string original = c;
        CAPTURE(original);
        CHECK(LevelCodec::decode(LevelCodec::encode(original)) == original);
    }
}

TEST_CASE("LevelCodec - 真实关卡文件能往返") {
    const std::filesystem::path levelPath =
        std::filesystem::path(PROJECT_ROOT) / "assets/levels/level1.txt";
    REQUIRE(std::filesystem::exists(levelPath));

    const std::string original = readFile(levelPath);
    REQUIRE_FALSE(original.empty());

    const std::string code = LevelCodec::encode(original);

    CHECK(LevelCodec::decode(code) == original);
    CHECK(LevelCodec::isValid(code));
}

TEST_CASE("LevelCodec - 压缩确实起作用（关卡越长越明显）") {
    // 一整行都是 '#'
    const std::string repetitive(2000, '#');

    const std::string code = LevelCodec::encode(repetitive);

    // RLE 会把它压成 "2000#"，base64 之后远短于原文
    CHECK(code.size() < repetitive.size() / 4);
    CHECK(LevelCodec::decode(code) == repetitive);
}

TEST_CASE("LevelCodec - 编解码是确定的（同一输入同一输出）") {
    const std::string original = kSampleLevel;

    CHECK(LevelCodec::encode(original) == LevelCodec::encode(original));
}

// ============================================================
// 空格容错
// ============================================================

TEST_CASE("LevelCodec - 分享码里的空白会被忽略") {
    const std::string original = kSampleLevel;
    const std::string code = LevelCodec::encode(original);

    // 模拟从聊天软件里粘贴：被折行、被塞进空格
    std::string messy;
    for (std::size_t i = 0; i < code.size(); ++i) {
        if (i == 10 || i == 25) messy += '\n';
        if (i == 15) messy += ' ';
        if (i == 30) messy += '\t';
        messy += code[i];
    }

    CHECK(LevelCodec::decode(messy) == original);
}

// ============================================================
// 非法输入
// ============================================================

TEST_CASE("LevelCodec - 非法的分享码返回空串而不是垃圾数据") {
    const char* bad[] = {
        "",                 // 空
        "TG1",              // 比前缀还短
        "TG1:",             // 只有前缀（解码出空 RLE）
        "XXXX:abcd",        // 前缀不对
        "tg1:abcd",         // 前缀大小写敏感
        "TG1:!!!!",         // base64 里没有 '!'
        "TG1:abcd!!!!",     // 中间混入非法字符
        "TG1:####",         // '#' 不是 base64 字符
        "TG1:AAAA",         // 合法 base64，但不是合法的 RLE
    };

    for (const char* c : bad) {
        CAPTURE(c);
        CHECK(LevelCodec::decode(c).empty());
        CHECK_FALSE(LevelCodec::isValid(c));
    }
}

TEST_CASE("LevelCodec - 合法 base64 但内容不是 RLE 时也要拒绝") {
    // "AAAA" 解码出三个 0 字节；RLE 要求 <数字><字符>，首字节不是数字 → 拒绝
    CHECK(LevelCodec::decode("TG1:AAAA").empty());
    CHECK(LevelCodec::decode("TG1:Zm9v").empty());   // "foo"：首字符不是数字
}

TEST_CASE("LevelCodec - 截断的分享码不会被当成有效关卡") {
    const std::string code = LevelCodec::encode(kSampleLevel);

    // 砍掉尾巴之后，要么解不出来，要么解出来跟原文不一样 —— 总之不能"看起来成功"
    const std::string truncated = code.substr(0, code.size() / 2);
    const std::string decoded = LevelCodec::decode(truncated);

    CHECK(decoded != kSampleLevel);
}

TEST_CASE("LevelCodec - isValid 与 decode 的判定一致") {
    const std::string good = LevelCodec::encode(kSampleLevel);
    CHECK(LevelCodec::isValid(good));
    CHECK_FALSE(LevelCodec::decode(good).empty());

    const std::string bad = "TG1:这不是base64";
    CHECK_FALSE(LevelCodec::isValid(bad));
    CHECK(LevelCodec::decode(bad).empty());
}

// ============================================================
// RLE 的边界
// ============================================================

TEST_CASE("LevelCodec - 超长重复段不会被解码成超大字符串") {
    // RLE 里带一个荒谬的计数。解码端有上限保护，不能让它去分配几个 G。
    // 这里只要求"不崩、且不返回原文"，不要求具体错误形式。
    const std::string evil = LevelCodec::encode("x");   // 只是拿一个合法前缀
    (void)evil;

    CHECK(LevelCodec::decode("TG1:OTk5OTk5OTk5I3g=").empty());   // "999999999#x"
    CHECK(LevelCodec::decode("TG1:MCM").empty());                // "0#"
}

TEST_CASE("LevelCodec - 计数为 0 的 RLE 段不会凭空生成字符") {
    // "0#" 是合法的 RLE 语法但产出空串 —— 空结果会被 decode 当成失败
    CHECK(LevelCodec::decode("TG1:MCM").empty());
}
