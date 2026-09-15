#include "console_scene.h"
#include "calculator.h"
#include "sound_manager.h"
#include "strings.h"
#include "theme.h"
#include "utf8.h"
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
    int fontSize = preferences_->getInt("console_font_size", 18);
    int historyLines = preferences_->getInt("console_history_lines", 200);
    int lineHeight = preferences_->getInt("console_line_height", 26);
    bool autoScroll = preferences_->getBool("console_auto_scroll", true);
    bool blinkCursor = preferences_->getBool("console_blink_cursor", true);

    console_ = std::make_unique<Console>(font, static_cast<unsigned>(fontSize),
                                         static_cast<unsigned>(lineHeight),
                                         static_cast<unsigned>(historyLines), autoScroll,
                                         blinkCursor, sf::Vector2u{1280, 720});
    // 提示符
    int promptIdx = preferences_->getInt("console_prompt", 0);
    static const char* prompts[] = {"> ", "$ ", "λ ", "❯ "};
    if (promptIdx < 0 || promptIdx > 3)
        promptIdx = 0;
    console_->setPrompt(prompts[promptIdx]);

    // RAII 重定向——构造失败也不会污染全局 iostream
    redirect_ = std::make_unique<ConsoleStreamRedirect>(console_.get());

    printWelcome();
    startCommandLoop();
    logger_->info("进入控制台场景");
}

ConsoleScene::~ConsoleScene() {
    stopWorker();
    // redirect_ 析构时自动恢复 cin/cout/cerr
}

void ConsoleScene::printWelcome() {
    std::cout << "TEXT-GAME 控制台\n";
    std::cout << "输入 help 查看可用命令\n";
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
        std::cout << "可用命令:\n";
        std::cout << "  help              显示帮助\n";
        std::cout << "  clear             清空屏幕\n";
        std::cout << "  echo <text>       回显文本\n";
        std::cout << "  version           显示版本\n";
        std::cout << "  calc              启动计算器\n";
        std::cout << "  scene <name>      切换场景 (main/save/settings/quit)\n";
        std::cout << "  log <level>       设置日志级别 (trace/debug/info/warn/error)\n";
        std::cout << "  theme <name>      切换主题 (dark/blue/light)\n";
        std::cout << "  save list         列出所有存档\n";
        std::cout << "  exit              关闭控制台\n";
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
        std::cout << "[启动计算器...]\n";
        Calculator calc(logger_);
        calc.run();
        std::cout << "[计算器已退出]\n";
    }

    // ===== scene =====
    else if (cmd == "scene") {
        if (tokens.size() < 2) {
            std::cout << "用法: scene <main|save|settings|quit>\n";
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
            std::cout << "未知场景: " << name << '\n';
        }
    }

    // ===== log =====
    else if (cmd == "log") {
        if (tokens.size() < 2) {
            std::cout << "用法: log <trace|debug|info|warn|error>\n";
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
            std::cout << "未知级别: " << level << '\n';
            return;
        }
        logger_->setMinLevel(lv);
        preferences_->setInt("log_level", static_cast<int>(lv));
        std::cout << "日志级别已切换: " << level << '\n';
    }

    // ===== theme =====
    else if (cmd == "theme") {
        if (tokens.size() < 2) {
            std::cout << "用法: theme <dark|blue|light>\n";
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
            std::cout << "未知主题: " << name << '\n';
            return;
        }
        setTheme(id);
        preferences_->setInt("theme", static_cast<int>(id));
        std::cout << "主题已切换。返回主菜单再进入生效。\n";
    }

    // ===== save =====
    else if (cmd == "save") {
        if (tokens.size() < 2 || tokens[1] == "list") {
            auto saves = saveManager_->listSaves();
            if (saves.empty()) {
                std::cout << "没有存档。\n";
            } else {
                std::cout << "共 " << saves.size() << " 个存档:\n";
                for (size_t i = 0; i < saves.size(); ++i) {
                    std::cout << "  " << (i + 1) << ". " << saves[i].name << "  ("
                              << saves[i].filename << ")\n";
                }
            }
        } else {
            std::cout << "用法: save list\n";
        }
    }

    // ===== exit =====
    else if (cmd == "exit") {
        pendingScene_ = static_cast<int>(SceneId::Back);
    }

    // ===== 未知命令 =====
    else {
        std::cout << "未知命令: " << cmd << "。输入 help 查看帮助。\n";
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
    auto& rt = window.native();
    auto size = rt.getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    rt.clear(sf::Color::Black);
    if (background_)
        background_->render(rt);

    int mask = preferences_->getInt("console_mask", 160);
    mask = std::max(0, std::min(255, mask));
    sf::RectangleShape overlay(sf::Vector2f{w, h});
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(mask)));
    rt.draw(overlay);

    int panelAlpha = preferences_->getInt("console_panel_alpha", 220);
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