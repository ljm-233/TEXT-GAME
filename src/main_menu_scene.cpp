#include "main_menu_scene.h"

MainMenuScene::MainMenuScene(std::shared_ptr<Background> background)
    : background_(std::move(background)) {}

void MainMenuScene::handleEvent(const sf::Event& /*event*/) {
    // 暂时不响应任何按键/点击，主界面一直显示
    // 以后做菜单时，这里改成响应菜单项的选择
}

void MainMenuScene::render(Window& window) {
    window.clear();                     // 纯黑，充当黑边
    if (background_) {
        background_->render(window.native());
    }
    window.display();
}