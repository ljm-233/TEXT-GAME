#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "button.h"

class ConfirmDialog {
public:
    enum class Mode { YesNo, Info };
    enum class Result { None, Yes, No, Ok };

    ConfirmDialog(const sf::Font& font,
                  const std::string& message,
                  sf::Vector2f windowSize,
                  Mode mode = Mode::YesNo);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);
    Result consumeResult();
    void relayout(sf::Vector2f windowSize);

private:
    Mode mode_ = Mode::YesNo;
    sf::RectangleShape backdrop_;
    sf::RectangleShape panel_;
    sf::Text message_;
    std::unique_ptr<Button> yesButton_;
    std::unique_ptr<Button> noButton_;
    Result result_ = Result::None;
    sf::Vector2f windowSize_;
};