#pragma once
#include "background.h"
#include "console.h"
#include "log/logger.h"
#include "preferences.h"
#include "save_manager.h"
#include "scene.h"
#include <atomic>
#include <memory>
#include <streambuf>
#include <thread>

std::unique_ptr<std::streambuf> makeConsoleStreamBuf(Console* c);

// RAII 包装：构造时重定向 cin/cout/cerr 到 Console，
// 析构时恢复。即使构造函数中途抛异常，也能正确恢复。
class ConsoleStreamRedirect {
public:
    explicit ConsoleStreamRedirect(Console* console);
    ~ConsoleStreamRedirect();

    ConsoleStreamRedirect(const ConsoleStreamRedirect&) = delete;
    ConsoleStreamRedirect& operator=(const ConsoleStreamRedirect&) = delete;

private:
    std::unique_ptr<std::streambuf> consoleBuf_;
    std::streambuf* oldCin_  = nullptr;
    std::streambuf* oldCout_ = nullptr;
    std::streambuf* oldCerr_ = nullptr;
};

class ConsoleScene : public Scene {
public:
    ConsoleScene(std::shared_ptr<Background> background,
                 std::shared_ptr<Preferences> preferences,
                 std::shared_ptr<SaveManager> saveManager, const sf::Font& font,
                 std::shared_ptr<Logger> logger);

    ~ConsoleScene() override;

    void onEnter() override;
    void onResume() override;  // ⭐ onPause 拆掉的流重定向 / worker 要装回来
    void onPause() override;   // ⭐ 真正的清理点：离开场景（pop / push / replace）走的是 onPause
    void onExit() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override;

private:
    void attach();           // 装上流重定向 + worker（onEnter / onResume 共用）
    void startCommandLoop();
    void stopWorker();
    void dispatchCommand(const std::string& line);
    void printWelcome();

    std::shared_ptr<Background> background_;
    std::shared_ptr<Preferences> preferences_;
    std::shared_ptr<SaveManager> saveManager_;
    std::shared_ptr<Logger> logger_;

    std::unique_ptr<Console> console_;
    std::unique_ptr<ConsoleStreamRedirect> redirect_;

    std::thread worker_;
    std::atomic<bool> workerDone_{false};
    std::atomic<int> pendingScene_{static_cast<int>(SceneId::None)};
};