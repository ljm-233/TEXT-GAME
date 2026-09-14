#pragma once
#include "background.h"
#include "console.h"
#include "logging.h"
#include "preferences.h"
#include "save_manager.h"
#include "scene.h"
#include <atomic>
#include <memory>
#include <streambuf>
#include <thread>

std::unique_ptr<std::streambuf> makeConsoleStreamBuf(Console* c);

class ConsoleScene : public Scene {
public:
    ConsoleScene(std::shared_ptr<Background> background,
                 std::shared_ptr<Preferences> preferences,
                 std::shared_ptr<SaveManager> saveManager, const sf::Font& font,
                 std::shared_ptr<Logger> logger);

    ~ConsoleScene() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override;

private:
    void startCommandLoop();
    void stopWorker();
    void dispatchCommand(const std::string& line);
    void printWelcome();

    std::shared_ptr<Background> background_;
    std::shared_ptr<Preferences> preferences_;
    std::shared_ptr<SaveManager> saveManager_;
    std::shared_ptr<Logger> logger_;

    std::unique_ptr<Console> console_;
    std::unique_ptr<std::streambuf> consoleBuf_;
    std::streambuf* oldCin_ = nullptr;
    std::streambuf* oldCout_ = nullptr;
    std::streambuf* oldCerr_ = nullptr;

    std::thread worker_;
    std::atomic<bool> workerDone_{false};
    std::atomic<int> pendingScene_{static_cast<int>(SceneId::None)};
    void playConsoleSound();
};