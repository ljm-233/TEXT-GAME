#pragma once
#include "scene.h"
#include "background.h"
#include "console.h"
#include <memory>
#include <thread>
#include <atomic>
#include <streambuf>

std::unique_ptr<std::streambuf> makeConsoleStreamBuf(Console* c);

class ConsoleScene : public Scene {
public:
    ConsoleScene(std::shared_ptr<Background> background,
                 const sf::Font& font,
                 std::shared_ptr<Logger> logger);

    ~ConsoleScene() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    void startCalculator();
    void stopWorker();

    std::shared_ptr<Background> background_;
    std::shared_ptr<Logger>     logger_;

    std::unique_ptr<Console> console_;
    std::unique_ptr<std::streambuf> consoleBuf_;
    std::streambuf* oldCin_  = nullptr;
    std::streambuf* oldCout_ = nullptr;
    std::streambuf* oldCerr_ = nullptr;

    std::thread       worker_;
    std::atomic<bool> workerDone_{false};

    SceneId nextScene_ = SceneId::None;
};