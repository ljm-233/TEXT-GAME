#include "game_scene.h"
#include "strings.h"
#include "utf8.h"
#include <algorithm>
#include <sstream>

namespace {

// ===== 演示关卡（内嵌）=====
constexpr const char* kDemoLevel = R"(
################################################
#                                              #
#                                              #
#    P                                         #
#          #####                               #
#                                              #
#                         ########             #
#      ####                                    #
#                ##########                    #
#                                              #
#     #                    ###                 #
#       ####               ###                 #
#             ##                               #
#                 #####                        #
################################################
)";

}

GameScene::GameScene(std::shared_ptr<Background> background,
                     const sf::Font& font,
                     std::shared_ptr<Logger> logger,
                     SaveInfo save)
    : background_(std::move(background)),
      logger_(std::move(logger)),
      save_(std::move(save)),
      hudText_(font, sf::String(), 20) {

    hudText_.setFillColor(sf::Color::White);

    auto level = std::make_unique<Level>();
    if (!level->loadFromString(kDemoLevel)) {
        logger_->error("关卡加载失败");
    } else {
        logger_->info("关卡已加载: " +
            std::to_string(level->width()) + "x" +
            std::to_string(level->height()) + " 瓦片");
    }

    world_ = std::make_unique<GameWorld>(std::move(level));
    world_->setViewSize(kLogicalW, kLogicalH);

    logger_->info("进入游戏场景，存档: " + save_.filename);
}

void GameScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
        if (kp->code == sf::Keyboard::Key::R) {
            world_->reset();
            return;
        }
    }
    world_->handleEvent(event);
}

void GameScene::update(float dt) {
    world_->update(dt);
}

void GameScene::render(Window& window) {
    auto& rt = window.native();
    auto winSize = rt.getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    // ===== 阶段 1：全屏背景 =====
    rt.setView(rt.getDefaultView());
    rt.clear(sf::Color::Black);
    if (background_) background_->render(rt);

    // ===== 阶段 2：游戏世界 =====
    // 固定 1280x720 逻辑分辨率，按窗口尺寸等比缩放并居中
    sf::View view(sf::FloatRect({0.f, 0.f}, {kLogicalW, kLogicalH}));

    float scale = std::min(winW / kLogicalW, winH / kLogicalH);
    float vpW = kLogicalW * scale / winW;
    float vpH = kLogicalH * scale / winH;
    float vpX = (1.f - vpW) * 0.5f;
    float vpY = (1.f - vpH) * 0.5f;
    view.setViewport(sf::FloatRect({vpX, vpY}, {vpW, vpH}));

    Vec2 camCenter = world_->cameraCenter();
    view.setCenter({camCenter.x, camCenter.y});
    rt.setView(view);

    // 直接画世界坐标
    world_->render(rt);

    // ===== 阶段 3：HUD =====
    rt.setView(rt.getDefaultView());

    std::string hud =
        "存档: " + save_.name +
        "   WASD/方向键 移动，Space/W 跳跃，R 重生，ESC 返回";
    hudText_.setString(toSf(hud));
    hudText_.setPosition({20.f, 16.f});
    rt.draw(hudText_);
}