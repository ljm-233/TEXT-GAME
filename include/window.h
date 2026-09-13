#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Window {
public:
    Window(unsigned width, unsigned height, const std::string& title);

    bool isOpen() const;
    void close();

    void clear();
    void display();

    // 支持传入事件回调；传 nullptr 表示只处理关闭事件
    void pollEvents(const std::function<void(const sf::Event&)>& handler = nullptr);

    bool isKeyPressed(sf::Keyboard::Key key) const;
    void draw(const sf::Drawable& drawable);

    sf::RenderWindow& native();

private:
    sf::RenderWindow window_;
};