#include "console_scene.h"
#include "calculator.h"
#include <iostream>

ConsoleScene::ConsoleScene(std::shared_ptr<Background> background,
                           const sf::Font& font,
                           std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      logger_(std::move(logger)) {

    console_ = std::make_unique<Console>(font, sf::Vector2u{1280, 720});

    consoleBuf_ = makeConsoleStreamBuf(console_.get());
    oldCin_  = std::cin.rdbuf(consoleBuf_.get());
    oldCout_ = std::cout.rdbuf(consoleBuf_.get());
    oldCerr_ = std::cerr.rdbuf(consoleBuf_.get());

    startCalculator();
    logger_->info("进入控制台场景");
}

ConsoleScene::~ConsoleScene() {
    stopWorker();

    if (oldCin_)  std::cin.rdbuf(oldCin_);
    if (oldCout_) std::cout.rdbuf(oldCout_);
    if (oldCerr_) std::cerr.rdbuf(oldCerr_);
}

void ConsoleScene::startCalculator() {
    auto calc = std::make_shared<Calculator>(logger_);
    worker_ = std::thread([this, calc]() {
        calc->run();
        workerDone_ = true;
    });
}

void ConsoleScene::stopWorker() {
    if (console_) console_->shutdown();
    if (worker_.joinable()) worker_.join();
}

void ConsoleScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::MainMenu;
            return;
        }
        console_->handleKeyPressed(kp->code);
    }
    if (const auto* te = event.getIf<sf::Event::TextEntered>()) {
        console_->handleTextEntered(te->unicode);
    }
}

void ConsoleScene::update(float /*dt*/) {}

void ConsoleScene::render(Window& window) {
    auto& rt = window.native();
    auto size = rt.getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    // 1. 正常画背景铺满整个窗口
    rt.clear(sf::Color::Black);
    if (background_) background_->render(rt);

    // 2. 半透明黑遮罩，让背景"变暗"
    sf::RectangleShape overlay({w, h});
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    rt.draw(overlay);

    // 3. 画终端内容
    console_->render(rt);

    rt.display();
}