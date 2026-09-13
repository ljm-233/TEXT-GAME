#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Window {
public:
    Window(unsigned width, unsigned height,
           const std::string& title, bool fullscreen = false);

    bool isOpen() const;
    void close();

    void clear();
    void display();

    void pollEvents(const std::function<void(const sf::Event&)>& handler = nullptr);

    bool isKeyPressed(sf::Keyboard::Key key) const;
    void draw(const sf::Drawable& drawable);

    // 重建窗口（分辨率 / 全屏切换时用）
    void recreate(unsigned width, unsigned height, bool fullscreen);

    sf::RenderWindow& native();

private:
    void applyView();

    sf::RenderWindow window_;
    std::string title_;
};