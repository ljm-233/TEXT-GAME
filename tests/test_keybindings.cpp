#include "doctest.h"
#include "infrastructure/keybindings.h"

#include <set>
#include <string>
#include <vector>

// 全局键位表。它是单例、没有 GlResource 成员（只有一个 sf::Keyboard::Key
// 数组），所以能在无界面环境直接测。
//
// 「恢复默认按键」这个设置项就落在 resetToDefaults() 上。它的另一半
// （把 5 个键写回配置）在 ControlsTab 里，且键名走 ConfigKey 常量 ——
// 写错字编译不过，所以那半边的风险是编译期挡住的，不需要测试来兜。

TEST_CASE("键位表 - 默认键位是 A / D / 空格 / ESC / R") {
    KeyBindings& kb = KeyBindings::instance();
    kb.resetToDefaults();

    // 把默认值写死在测试里是**故意**的：这是一套要长期保持稳定的东西，
    // 哪天被人顺手改了（尤其是"改成方向键"这种看着更合理的改动），
    // 老玩家的肌肉记忆就没了，而不会有任何东西报错。
    // 真要改默认键位，请连这条测试一起改 —— 那时改动是有意识的。
    CHECK(kb.get(KeyBindings::MoveLeft) == sf::Keyboard::Key::A);
    CHECK(kb.get(KeyBindings::MoveRight) == sf::Keyboard::Key::D);
    CHECK(kb.get(KeyBindings::Jump) == sf::Keyboard::Key::Space);
    CHECK(kb.get(KeyBindings::Pause) == sf::Keyboard::Key::Escape);
    CHECK(kb.get(KeyBindings::Restart) == sf::Keyboard::Key::R);
}

TEST_CASE("键位表 - 改过之后 resetToDefaults 能回到默认") {
    KeyBindings& kb = KeyBindings::instance();

    kb.set(KeyBindings::Jump, sf::Keyboard::Key::Z);
    CHECK(kb.get(KeyBindings::Jump) == sf::Keyboard::Key::Z);

    kb.resetToDefaults();
    CHECK(kb.get(KeyBindings::Jump) == sf::Keyboard::Key::Space);

    // 每个动作都要有非 Unknown 的默认键 —— 漏掉一个的话那个动作就永远
    // 绑不上键，而且是静默的（按下没反应，谁也不知道为什么）
    for (int i = 0; i < KeyBindings::Count; ++i) {
        const auto a = static_cast<KeyBindings::Action>(i);
        const bool bound = kb.get(a) != sf::Keyboard::Key::Unknown;
        CHECK_MESSAGE(bound, "动作 " << KeyBindings::actionName(a) << " 没有默认键");
    }
}

TEST_CASE("键位表 - 默认键位互不重复") {
    KeyBindings& kb = KeyBindings::instance();
    kb.resetToDefaults();

    // 两个动作绑同一个人键，就有一个动作永远按不出来
    for (int i = 0; i < KeyBindings::Count; ++i) {
        for (int j = i + 1; j < KeyBindings::Count; ++j) {
            const auto a = static_cast<KeyBindings::Action>(i);
            const auto b = static_cast<KeyBindings::Action>(j);
            const bool same = kb.get(a) == kb.get(b);
            CHECK_MESSAGE(!same, KeyBindings::actionName(a)
                                     << " 与 " << KeyBindings::actionName(b)
                                     << " 的默认键位相同");
        }
    }
}

TEST_CASE("键位表 - 每个动作都有名字，且不重名") {
    std::set<std::string> names;
    for (int i = 0; i < KeyBindings::Count; ++i) {
        const char* n = KeyBindings::actionName(static_cast<KeyBindings::Action>(i));
        CHECK(n != nullptr);
        const bool nonEmpty = n && *n != '\0';
        CHECK(nonEmpty);
        if (nonEmpty)
            names.insert(n);
    }
    CHECK(names.size() == static_cast<std::size_t>(KeyBindings::Count));
}

TEST_CASE("键位表 - 方向键是固定的兜底键，且不受改键影响") {
    // 只给移动与跳跃兜底；暂停/重开不需要，给它们兜底反而会撞别的键
    CHECK(KeyBindings::fallbackKey(KeyBindings::MoveLeft) == sf::Keyboard::Key::Left);
    CHECK(KeyBindings::fallbackKey(KeyBindings::MoveRight) == sf::Keyboard::Key::Right);
    CHECK(KeyBindings::fallbackKey(KeyBindings::Jump) == sf::Keyboard::Key::Up);
    CHECK(KeyBindings::fallbackKey(KeyBindings::Pause) == sf::Keyboard::Key::Unknown);
    CHECK(KeyBindings::fallbackKey(KeyBindings::Restart) == sf::Keyboard::Key::Unknown);
}

TEST_CASE("键位表 - 兜底键与默认键位不重复，兜底键之间也不重复") {
    KeyBindings& kb = KeyBindings::instance();
    kb.resetToDefaults();

    std::vector<sf::Keyboard::Key> seen;
    for (int i = 0; i < KeyBindings::Count; ++i) {
        const auto a = static_cast<KeyBindings::Action>(i);
        const auto fb = KeyBindings::fallbackKey(a);
        if (fb == sf::Keyboard::Key::Unknown)
            continue;

        // 兜底键撞上任何一个可配置键位都会让"改键后失效"
        for (int j = 0; j < KeyBindings::Count; ++j) {
            const auto b = static_cast<KeyBindings::Action>(j);
            CHECK_MESSAGE(kb.get(b) != fb, KeyBindings::actionName(a)
                                               << " 的兜底键与 "
                                               << KeyBindings::actionName(b)
                                               << " 的默认键位撞了");
        }
        for (auto prev : seen)
            CHECK(prev != fb);
        seen.push_back(fb);
    }
    CHECK(seen.size() == 3);
}
