#pragma once
#include "button.h"
#include "text_input.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// 新建存档对话框：包含一个输入框 + 创建/取消按钮
class NewSaveDialog {
public:
    enum class Result { None, Created, Cancelled };

    NewSaveDialog(const sf::Font& font, const std::string& defaultName,
                  sf::Vector2f windowSize);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderTarget& target);
    Result consumeResult();

    void relayout(sf::Vector2f windowSize);
    const std::string& getName() const;

private:
    sf::RectangleShape backdrop_;
    sf::RectangleShape panel_;
    sf::Text title_;
    sf::Text hint_;

    std::unique_ptr<TextInput> input_;
    std::unique_ptr<Button> createButton_;
    std::unique_ptr<Button> cancelButton_;

    Result result_ = Result::None;
    sf::Vector2f windowSize_;
};