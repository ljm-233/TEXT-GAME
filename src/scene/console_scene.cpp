#include "console_scene.h"
#include "scene/console/calculator.h"
#include "sound_manager.h"
#include "achievement.h"
#include "utils/text_strings.h"
#include "theme.h"
#include "utils/utf8.h"
#include "config/keys.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

// ============================================================
// ConsoleStreamRedirect
// ============================================================

ConsoleStreamRedirect::ConsoleStreamRedirect(Console* console) {
    consoleBuf_ = makeConsoleStreamBuf(console);
    oldCin_  = std::cin.rdbuf(consoleBuf_.get());
    oldCout_ = std::cout.rdbuf(consoleBuf_.get());
    oldCerr_ = std::cerr.rdbuf(consoleBuf_.get());
}

ConsoleStreamRedirect::~ConsoleStreamRedirect() {
    if (oldCin_)  std::cin.rdbuf(oldCin_);
    if (oldCout_) std::cout.rdbuf(oldCout_);
    if (oldCerr_) std::cerr.rdbuf(oldCerr_);
}

// ============================================================
// ConsoleScene
// ============================================================

ConsoleScene::ConsoleScene(std::shared_ptr<Background> background,
                           std::shared_ptr<Preferences> preferences,
                           std::shared_ptr<SaveManager> saveManager, const sf::Font& font,
                           std::shared_ptr<Logger> logger)
      : background_(std::move(background)),
        preferences_(std::move(preferences)),
        saveManager_(std::move(saveManager)),
        logger_(std::move(logger)) {
    int fontSize = preferences_->getInt(ConfigKey::kConsoleFontSize, 18);
    int historyLines = preferences_->getInt(ConfigKey::kConsoleHistoryLines, 200);
    int lineHeight = preferences_->getInt(ConfigKey::kConsoleLineHeight, 26);
    bool autoScroll = preferences_->getBool(ConfigKey::kConsoleAutoScroll, true);
    bool blinkCursor = preferences_->getBool(ConfigKey::kConsoleBlinkCursor, true);

    console_ = std::make_unique<Console>(font, static_cast<unsigned>(fontSize),
                                         static_cast<unsigned>(lineHeight),
                                         static_cast<unsigned>(historyLines), autoScroll,
                                         blinkCursor, sf::Vector2u{1280, 720});
    // 提示符
    int promptIdx = preferences_->getInt(ConfigKey::kConsolePrompt, 0);
    static const char* prompts[] = {"> ", "$ ", "λ ", "❯ "};
    if (promptIdx < 0 || promptIdx > 3)
        promptIdx = 0;
    console_->setPrompt(prompts[promptIdx]);

    logger_->info("控制台场景已创建（等待 onEnter）");
}

void ConsoleScene::attach() {
    if (!redirect_) {
        redirect_ = std::make_unique<ConsoleStreamRedirect>(console_.get());
    }
    if (!worker_.joinable()) {
        console_->resetShutdown();
        workerDone_ = false;
        startCommandLoop();
    }
}

void ConsoleScene::onEnter() {
    pendingScene_ = static_cast<int>(SceneId::None);

    // ⭐ 每次进入才重定向 iostream + 启动 worker
    attach();

    printWelcome();

    // ⭐ 进入控制台即算使用
    AchievementManager::instance().unlock("console_used");
}

void ConsoleScene::onResume() {
    // ⭐ 场景常驻 + 清理挂在 onPause 上，所以被 pop 回来时要重新装回去。
    //    走得到这条路：在控制台里敲 `scene settings` 会 push 出设置场景，
    //    再返回时 console 拿到的就是 onResume（不是 onEnter）。
    pendingScene_ = static_cast<int>(SceneId::None);
    attach();
}

// ⭐ 清理必须挂在 onPause 上：SceneManager 离开一个场景走的是
//    pop / push / replace，它们都只调 onPause()；onExit() 只在程序退出
//    （~SceneManager）时才调。原来的清理只写在 onExit 里，于是离开控制台后
//    cin/cout/cerr 一直指向控制台缓冲区（终端里再也看不到日志）、worker
//    线程也一直活着 —— 而且因为 attach() 里有 !redirect_ / !joinable()
//    判断，再进控制台看着还是正常的，所以这个 bug 是静默的。
void ConsoleScene::onPause() {
    stopWorker();
    redirect_.reset();   // ⭐ 恢复 cin/cout/cerr
}

void ConsoleScene::onExit() {
    onPause();
}

ConsoleScene::~ConsoleScene() {
    stopWorker();
    // redirect_ 析构时自动恢复 cin/cout/cerr
}

void ConsoleScene::printWelcome() {
    std::cout << Str::T(Str::ConsoleWelcomeTitle) << "\n";
    std::cout << Str::T(Str::ConsoleWelcomeHint) << "\n";
    std::cout << "\n";
}

void ConsoleScene::startCommandLoop() {
    worker_ = std::thread([this]() {
        while (true) {
            std::string line = console_->waitForLine();
            if (console_->isShutdown())
                break;
            if (!line.empty()) {
                dispatchCommand(line);
            }
        }
        workerDone_ = true;
    });
}

void ConsoleScene::stopWorker() {
    if (console_)
        console_->shutdown();
    if (worker_.joinable())
        worker_.join();
}

SceneId ConsoleScene::nextScene() const {
    int v = pendingScene_.load();
    return static_cast<SceneId>(v);
}

// ============================================================
// 命令分发
// ============================================================

void ConsoleScene::dispatchCommand(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string t;
    while (iss >> t)
        tokens.push_back(t);
    if (tokens.empty())
        return;

    const std::string& cmd = tokens[0];

    // ===== help =====
    if (cmd == "help") {
        std::cout << Str::T(Str::ConsoleHelpText);
        std::cout << "\n";
    }

    // ===== clear =====
    else if (cmd == "clear") {
        console_->clear();
    }

    // ===== echo =====
    else if (cmd == "echo") {
        std::string text;
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (i > 1)
                text += ' ';
            text += tokens[i];
        }
        std::cout << text << '\n';
    }

    // ===== version =====
    else if (cmd == "version") {
        std::cout << Str::AboutTitle << " " << PROJECT_VERSION << " (" << BUILD_DATE
                  << ")\n";
    }

    // ===== calc =====
    else if (cmd == "calc") {
        std::cout << Str::T(Str::ConsoleCalcStart) << "\n";
        Calculator calc(logger_);
        calc.run();
        std::cout << Str::T(Str::ConsoleCalcEnd) << "\n";
    }

    // ===== scene =====
    else if (cmd == "scene") {
        if (tokens.size() < 2) {
            std::cout << Str::T(Str::ConsoleUsageScene) << "\n";
            return;
        }
        const std::string& name = tokens[1];
        if (name == "main")
            pendingScene_ = static_cast<int>(SceneId::MainMenu);
        else if (name == "save")
            pendingScene_ = static_cast<int>(SceneId::SaveSelect);
        else if (name == "settings")
            pendingScene_ = static_cast<int>(SceneId::Settings);
        else if (name == "quit")
            pendingScene_ = static_cast<int>(SceneId::Exit);
        else {
            std::cout << Str::T(Str::ConsoleUnknownScene) << name << '\n';
        }
    }

    // ===== log =====
    else if (cmd == "log") {
        if (tokens.size() < 2) {
            std::cout << Str::T(Str::ConsoleUsageLog) << "\n";
            return;
        }
        const std::string& level = tokens[1];
        LogLevel lv;
        if (level == "trace")
            lv = LogLevel::Trace;
        else if (level == "debug")
            lv = LogLevel::Debug;
        else if (level == "info")
            lv = LogLevel::Info;
        else if (level == "warn")
            lv = LogLevel::Warn;
        else if (level == "error")
            lv = LogLevel::Error;
        else {
            std::cout << Str::T(Str::ConsoleUnknownLevel) << level << '\n';
            return;
        }
        logger_->setMinLevel(lv);
        preferences_->setInt(ConfigKey::kLogLevel, static_cast<int>(lv));
        std::cout << Str::T(Str::ConsoleLogLevelChanged) << level << '\n';
    }

    // ===== theme =====
    else if (cmd == "theme") {
        if (tokens.size() < 2) {
            std::cout << Str::T(Str::ConsoleUsageTheme) << "\n";
            return;
        }
        const std::string& name = tokens[1];
        ThemeId id;
        if (name == "dark")
            id = ThemeId::Dark;
        else if (name == "blue")
            id = ThemeId::Blue;
        else if (name == "light")
            id = ThemeId::Light;
        else {
            std::cout << Str::T(Str::ConsoleUnknownTheme) << name << '\n';
            return;
        }
        setTheme(id);
        preferences_->setInt(ConfigKey::kTheme, static_cast<int>(id));
        std::cout << Str::T(Str::ConsoleThemeChanged) << "\n";
    }

    // ===== save =====
    else if (cmd == "save") {
        if (tokens.size() < 2 || tokens[1] == "list") {
            auto saves = saveManager_->listSaves();
            if (saves.empty()) {
                std::cout << Str::T(Str::ConsoleNoSaves) << "\n";
            } else {
                std::cout << Str::T(Str::ConsoleSavesPrefix) << saves.size()
                          << Str::T(Str::ConsoleSavesSuffix) << "\n";
                for (size_t i = 0; i < saves.size(); ++i) {
                    std::cout << "  " << (i + 1) << ". " << saves[i].name << "  ("
                              << saves[i].filename << ")\n";
                }
            }
        } else {
            std::cout << Str::T(Str::ConsoleUsageSave) << "\n";
        }
    }

    // ===== exit =====
    else if (cmd == "exit") {
        pendingScene_ = static_cast<int>(SceneId::Back);
    }

    // ===== 未知命令 =====
    else {
        std::cout << Str::T(Str::ConsoleUnknownCmd1) << cmd
                  << Str::T(Str::ConsoleUnknownCmd2) << "\n";
        SoundManager::instance().playHurt();
        return;
    }

    // 命令执行成功音效
    if (cmd != "help" && cmd != "clear" && cmd != "echo" && cmd != "save")
        SoundManager::instance().playCoin();
}

// ============================================================
// 事件 / 更新 / 渲染
// ============================================================

void ConsoleScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            pendingScene_ = static_cast<int>(SceneId::Back);
            return;
        }
        console_->handleKeyPressed(kp->code);
    }
    if (const auto* te = event.getIf<sf::Event::TextEntered>()) {
        console_->handleTextEntered(te->unicode);
    }
    if (const auto* ws = event.getIf<sf::Event::MouseWheelScrolled>()) {
        console_->handleMouseWheel(ws->delta);
    }
}

void ConsoleScene::update(float /*dt*/) {}

void ConsoleScene::render(Window& window) {
    auto& rt = window.target();
    auto size = window.native().getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    rt.clear(sf::Color::Black);
    if (background_)
        background_->render(rt);

    int mask = preferences_->getInt(ConfigKey::kConsoleMask, 160);
    mask = std::max(0, std::min(255, mask));
    sf::RectangleShape overlay(sf::Vector2f{w, h});
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(mask)));
    rt.draw(overlay);

    int panelAlpha = preferences_->getInt(ConfigKey::kConsolePanelAlpha, 220);
    panelAlpha = std::max(0, std::min(255, panelAlpha));

    const float pad = 16.f;
    sf::RectangleShape panel(sf::Vector2f{w - pad * 2, h - pad * 2});
    panel.setPosition({pad, pad});
    panel.setFillColor(sf::Color(5, 5, 10, static_cast<std::uint8_t>(panelAlpha)));
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(70, 70, 100));
    rt.draw(panel);

    console_->render(rt);
}