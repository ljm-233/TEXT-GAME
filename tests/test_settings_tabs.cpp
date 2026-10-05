#include "doctest.h"
#include "config/keys.h"
#include "scene/settings_tab_id.h"

#include <algorithm>
#include <set>
#include <string>

// 设置页的「归属」检查。
//
// 0.3.8 重做设置系统时最典型的翻车是：新加了一个配置键、却忘了归到某一页，
// 或者归错了页 —— 这两种都是**完全静默**的（界面上根本看不到那个键，
// 或者它出现在一个没人会去找的地方）。这两条测试把它变成会红的。

TEST_CASE("设置归属 - 每个配置键都恰好属于一页") {
    std::vector<std::string> unowned;
    std::vector<std::string> duplicated;

    const auto& deliberatelyUnowned = unownedKeys();
    for (const char* key : ConfigKey::allKeys()) {
        int hits = 0;
        for (int i = 0; i < static_cast<int>(kSettingsTabCount); ++i) {
            const auto& keys = keysForTab(static_cast<SettingsTab>(i));
            if (std::find(keys.begin(), keys.end(), std::string(key)) != keys.end())
                ++hits;
        }
        const bool onUnownedList =
            std::find(deliberatelyUnowned.begin(), deliberatelyUnowned.end(),
                      std::string(key)) != deliberatelyUnowned.end();
        if (hits == 0 && !onUnownedList)
            unowned.push_back(key);
        else if (hits > 1)
            duplicated.push_back(key);
        else if (hits == 1 && onUnownedList)
            duplicated.push_back(key); // 又归类又说"不归类"，矛盾
    }

    // 有键不属于任何一页：要么忘了归类，要么它就该在「高级」页
    CHECK_MESSAGE(
        unowned.empty(), "这些键没有归属的设置页：" << [&] {
            std::string s;
            for (const auto& k : unowned)
                s += "\n      " + k;
            return s;
        }());

    // 同一个键被两页认领：两边都会显示它，改一处另一处不同步
    CHECK_MESSAGE(
        duplicated.empty(), "这些键被多个设置页同时认领：" << [&] {
            std::string s;
            for (const auto& k : duplicated)
                s += "\n      " + k;
            return s;
        }());
}

TEST_CASE("设置归属 - 表里不能有 keys.h 里不存在的键（防拼错）") {
    std::set<std::string> known;
    for (const char* key : ConfigKey::allKeys())
        known.insert(key);

    std::vector<std::string> typos;
    for (int i = 0; i < static_cast<int>(kSettingsTabCount); ++i) {
        for (const char* key : keysForTab(static_cast<SettingsTab>(i)))
            if (known.count(key) == 0)
                typos.push_back(key);
    }

    // 归属表里写了一个不存在的键名，那一条「恢复本页默认」就是空操作，
    // 而且永远不会有任何提示
    CHECK_MESSAGE(
        typos.empty(), "归属表里有 keys.h 里不存在的键：" << [&] {
            std::string s;
            for (const auto& k : typos)
                s += "\n      " + k;
            return s;
        }());
}

TEST_CASE("设置归属 - tabForKey 与 keysForTab 互为逆运算") {
    for (int i = 0; i < static_cast<int>(kSettingsTabCount); ++i) {
        const auto t = static_cast<SettingsTab>(i);
        for (const char* key : keysForTab(t))
            CHECK(tabForKey(key) == std::optional<SettingsTab>(t));
    }
    CHECK(tabForKey("这个键不存在") == std::nullopt);
}

TEST_CASE("设置归属 - 每页都有非空标签，且不重名") {
    std::set<std::string> labels;
    for (int i = 0; i < static_cast<int>(kSettingsTabCount); ++i) {
        const char* label = settingsTabLabel(static_cast<SettingsTab>(i));
        CHECK(label != nullptr);
        const bool nonEmpty = label && *label != '\0';
        CHECK(nonEmpty);
        if (nonEmpty)
            labels.insert(label);
    }
    // 两页同名的话，Tab 栏上会出现两个一模一样的按钮，用户根本分不清
    CHECK(labels.size() == static_cast<std::size_t>(kSettingsTabCount));
}

TEST_CASE("设置归属 - 机器绑定的键不出现在可携带白名单里") {
    // 这条把「导入别人的设置」的安全边界钉住：分辨率/窗口模式/ui_scale 这些
    // 一旦被导入，可能开出用户根本用不了的窗口，而设置界面正是他改回来的地方。
    const char* kForbidden[] = {
        ConfigKey::kResolutionIndex,
        ConfigKey::kFullscreen,
        ConfigKey::kWindowMode,
        ConfigKey::kVsync,
        ConfigKey::kFpsLimit,
        ConfigKey::kAntiAliasing,
        ConfigKey::kUiScale,
        ConfigKey::kFontScale,
        ConfigKey::kRememberWindowSize,
        ConfigKey::kCurrentWallpaper,
    };
    for (const char* key : kForbidden) {
        const bool portable = ConfigKey::isPortable(key);
        CHECK_MESSAGE(!portable, "这个键不该可携带：" << key);
    }

    // 反过来，观感/音量类的键必须在白名单里（否则导出等于什么都没导）
    const char* kMustBePortable[] = {
        ConfigKey::kTheme,   ConfigKey::kAudioMasterVolume, ConfigKey::kPostSaturation,
        ConfigKey::kKeyJump, ConfigKey::kInitialLives,      ConfigKey::kUiSoundEnabled,
    };
    for (const char* key : kMustBePortable) {
        const bool portable = ConfigKey::isPortable(key);
        CHECK_MESSAGE(portable, "这个键应该可携带：" << key);
    }

    CHECK(ConfigKey::portableKeyCount() > 0);
    CHECK(ConfigKey::portableKeyCount() < static_cast<int>(ConfigKey::allKeys().size()));
}
