#include "console_scene.h"
#include "calculator.h"
#include "strings.h"
#include "utf8.h"
#include <iostream>

ConsoleScene::ConsoleScene(std::shared_ptr<Background>  background,
                           std::shared_ptr<Preferences> preferences,
                           const sf::Font&              font,
                           std::shared_ptr<Logger>      logger)
    : background_(std::move(background)),
      preferences_(std::move(preferences)),
      logger_(std::move(logger)) {

    int fontSize     = preferences_->getInt("console_font_size", 18);
    int historyLines = preferences_->getInt("console_history_lines", 200);
    bool autoScroll  = preferences_->getBool("console_auto_scroll", true);
    bool blinkCursor = preferences_->getBool("console_blink_cursor", true);

    console_ = std::make_unique<Console>(
        font,
        static_cast<unsigned>(fontSize),
        static_cast<unsigned>(historyLines),
        autoScroll,
        blinkCursor,
        sf::Vector2u{1280, 720});

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
            nextScene_ = SceneId::Back;
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
    if (background_) background_->render(rt);

    // 遮罩强度从 Preferences 读
    int mask = preferences_->getInt("console_mask", 160);
    mask = std::max(0, std::min(255, mask));

    sf::RectangleShape overlay({w, h});
    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(mask)));
    rt.draw(overlay);

    const float pad = 16.f;
    sf::RectangleShape panel({w - pad * 2, h - pad * 2});
    panel.setPosition({pad, pad});
    panel.setFillColor(sf::Color(5, 5, 10, 215));
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(70, 70, 100));
    rt.draw(panel);

    console_->render(rt);
}