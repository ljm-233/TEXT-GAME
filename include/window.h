#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class Window {
public:
    Window(unsigned width, unsigned height, const std::string& title);

    bool isOpen() const;
    void close();

    void clear();                                  // 清屏
    void display();                                // 呈现
    void pollEvents();                             // 处理事件
    bool isKeyPressed(sf::Keyboard::Key key) const; // 键盘查询
    void draw(const sf::Drawable& drawable);       // 画任何可绘制对象

    sf::RenderWindow& native();                    // 需要底层时用

private:
    sf::RenderWindow window_;
};