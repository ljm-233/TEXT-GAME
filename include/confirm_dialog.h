#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "button.h"

class ConfirmDialog {
public:
    enum class Result { None, Yes, No };

    ConfirmDialog(const sf::Font& font,
                  const std::string& message,
                  sf::Vector2f windowSize);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);

    // 消费式读取结果
    Result consumeResult();

    // 布局依赖窗口尺寸，若窗口重建需要更新
    void relayout(sf::Vector2f windowSize);

private:
    sf::RectangleShape backdrop_;
    sf::RectangleShape panel_;
    sf::Text message_;
    std::unique_ptr<Button> yesButton_;
    std::unique_ptr<Button> noButton_;
    Result result_ = Result::None;
    sf::Vector2f windowSize_;
};