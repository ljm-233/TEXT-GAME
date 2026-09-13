#include "window.h"

Window::Window(unsigned width, unsigned height, const std::string& title) {
    // 1. 创建 ContextSettings 并设置抗锯齿级别
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8; // 常用 4 或 8，硬件不支持时会自动降级

    // 2. 通过 create() 创建窗口并应用设置
    window_.create(sf::VideoMode({width, height}), title, sf::State::Windowed, settings);

    window_.setVerticalSyncEnabled(true);
}

bool Window::isOpen() const { return window_.isOpen(); }
void Window::close()        { window_.close(); }
void Window::clear()        { window_.clear(sf::Color::Black); }
void Window::display()      { window_.display(); }

void Window::pollEvents(const std::function<void(const sf::Event&)>& handler) {
    while (const auto event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        }
        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            // 让 view 和窗口实际尺寸保持一致，避免内容被拉伸
            window_.setView(sf::View(sf::FloatRect(
                {0.f, 0.f},
                {static_cast<float>(resized->size.x),
                 static_cast<float>(resized->size.y)})));
        }
        if (handler) handler(*event);
    }
}

bool Window::isKeyPressed(sf::Keyboard::Key key) const {
    return sf::Keyboard::isKeyPressed(key);
}

void Window::draw(const sf::Drawable& drawable) {
    window_.draw(drawable);
}

sf::RenderWindow& Window::native() { return window_; }