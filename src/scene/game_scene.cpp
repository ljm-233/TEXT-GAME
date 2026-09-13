#include "game_scene.h"
#include "strings.h"
#include "utf8.h"

GameScene::GameScene(std::shared_ptr<Background> background,
                     const sf::Font& font,
                     std::shared_ptr<Logger> logger,
                     SaveInfo save)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      save_(std::move(save)),
      infoText_(font, "", 28) {
    infoText_.setFillColor(sf::Color::White);

    std::string content =
        std::string(Str::GameNotImpl) + "\n\n" +
        Str::CurrentSave + save_.name + "\n\n" +
        Str::EscToSaveSelect;
    infoText_.setString(toSf(content));

    logger_->info("进入游戏场景，存档: " + save_.filename);
}

void GameScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
        }
    }
}

void GameScene::update(float /*dt*/) {}

void GameScene::render(Window& window) {
    window.clear();
    if (background_) background_->render(window.native());

    auto size = window.native().getSize();
    auto b = infoText_.getLocalBounds();
    infoText_.setOrigin({b.position.x + b.size.x / 2.f,
                         b.position.y + b.size.y / 2.f});
    infoText_.setPosition({static_cast<float>(size.x) / 2.f,
                           static_cast<float>(size.y) / 2.f});
    window.native().draw(infoText_);
}