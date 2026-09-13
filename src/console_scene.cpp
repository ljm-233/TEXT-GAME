#include "console_scene.h"
#include "calculator.h"
#include <iostream>

ConsoleScene::ConsoleScene(std::shared_ptr<Background> background,
                           const sf::Font& font,
                           std::shared_ptr<Logger> logger)
    : background_(std::move(background)),
      logger_(std::move(logger)) {

    // 用当前窗口尺寸初始化 Console
    console_ = std::make_unique<Console>(font, sf::Vector2u{1280, 720});

    // 重定向 cin/cout/cerr
    consoleBuf_ = makeConsoleStreamBuf(console_.get());
    oldCin_  = std::cin.rdbuf(consoleBuf_.get());
    oldCout_ = std::cout.rdbuf(consoleBuf_.get());
    oldCerr_ = std::cerr.rdbuf(consoleBuf_.get());

    startCalculator();
    logger_->info("进入控制台场景");
}

ConsoleScene::~ConsoleScene() {
    stopWorker();

    // 恢复标准流
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

void ConsoleScene::update(float /*dt*/) {
    // 计算器跑完后自动提示
    if (workerDone_ && nextScene_ == SceneId::None) {
        // 什么也不做，让用户按 ESC 回主菜单
    }
}

void ConsoleScene::render(Window& window) {
    auto& rt = window.native();
    auto size = rt.getSize();
    float w = static_cast<float>(size.x);
    float h = static_cast<float>(size.y);

    // 背景模糊：先把背景画到一个小尺寸 RenderTexture，再放大回主窗口
    if (!blurReady_ || size.x != lastSize_.x || size.y != lastSize_.y) {
        unsigned bw = std::max(1u, size.x / 8);
        unsigned bh = std::max(1u, size.y / 8);
        (void)blurTarget_.resize({bw, bh});
        blurTarget_.setSmooth(true);
        lastSize_ = size;
        blurReady_ = true;
    }

    // 用和主窗口一致的坐标空间绘制到 blurTarget_
    blurTarget_.setView(sf::View(sf::FloatRect(
        {0.f, 0.f}, {w, h})));
    blurTarget_.clear(sf::Color::Black);
    if (background_) background_->render(blurTarget_);
    blurTarget_.display();

    // 把模糊后的纹理放大到主窗口
    rt.clear(sf::Color::Black);
    sf::Sprite blurSprite(blurTarget_.getTexture());
    blurSprite.setScale({w / blurTarget_.getSize().x,
                         h / blurTarget_.getSize().y});
    rt.draw(blurSprite);

    // 叠半透明黑，让文字可读
    sf::RectangleShape overlay({w, h});
    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    rt.draw(overlay);

    // 画终端内容
    console_->render(rt);

    rt.display();
}