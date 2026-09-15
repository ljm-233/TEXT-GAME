#include "doctest.h"
#include "strings.h"
#include <cstring>

TEST_CASE("Str - 常用字符串非空") {
    CHECK(std::strlen(Str::Yes)  > 0);
    CHECK(std::strlen(Str::No)   > 0);
    CHECK(std::strlen(Str::Back) > 0);
    CHECK(std::strlen(Str::On)   > 0);
    CHECK(std::strlen(Str::Off)  > 0);
}

TEST_CASE("Str - 主菜单字符串非空") {
    CHECK(std::strlen(Str::StartGame)  > 0);
    CHECK(std::strlen(Str::Calculator) > 0);
    CHECK(std::strlen(Str::Settings)   > 0);
    CHECK(std::strlen(Str::ExitGame)   > 0);
}

TEST_CASE("Str - Tab 字符串非空") {
    CHECK(std::strlen(Str::TabDisplay)   > 0);
    CHECK(std::strlen(Str::TabInterface) > 0);
    CHECK(std::strlen(Str::TabGraphics)  > 0);
    CHECK(std::strlen(Str::TabAudioLog)  > 0);
    CHECK(std::strlen(Str::TabOther)     > 0);
}

TEST_CASE("Str - 设置标签非空") {
    CHECK(std::strlen(Str::LabelResolution) > 0);
    CHECK(std::strlen(Str::LabelFullscreen) > 0);
    CHECK(std::strlen(Str::LabelVsync)      > 0);
    CHECK(std::strlen(Str::LabelSound)      > 0);
    CHECK(std::strlen(Str::LabelGamepad)    > 0);
}

TEST_CASE("Str - 主题名非空") {
    CHECK(std::strlen(Str::ThemeDark)  > 0);
    CHECK(std::strlen(Str::ThemeBlue)  > 0);
    CHECK(std::strlen(Str::ThemeLight) > 0);
}