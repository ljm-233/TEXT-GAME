#include "game_world.h"
#include <algorithm>

GameWorld::GameWorld(std::unique_ptr<Level> level)
    : level_(std::move(level)),
      player_(level_->playerSpawn()) {
    camera_.setLevelBounds(static_cast<float>(level_->pixelWidth()),
                           static_cast<float>(level_->pixelHeight()));
    camera_.snapTo(player_.bounds().center());
}

void GameWorld::setViewSize(float w, float h) {
    camera_.setViewSize(w, h);
}

void GameWorld::handleEvent(const sf::Event& event) {
    player_.handleEvent(event);
}

void GameWorld::update(float dt) {
    constexpr float kFixedStep = 1.f / 120.f;
    static float accumulator = 0.f;

    accumulator += dt;
    int iterations = 0;
    while (accumulator >= kFixedStep && iterations < 8) {
        player_.update(kFixedStep, *level_);
        accumulator -= kFixedStep;
        ++iterations;
    }

    camera_.follow(player_.bounds().center(), dt);
}

void GameWorld::render(sf::RenderTarget& target) {
    Vec2 camTL = camera_.position();
    level_->render(target,
                   camTL.x, camTL.y,
                   camera_.viewWidth(), camera_.viewHeight());
    player_.render(target);
}

void GameWorld::reset() {
    player_.respawn(level_->playerSpawn());
    camera_.snapTo(player_.bounds().center());
}