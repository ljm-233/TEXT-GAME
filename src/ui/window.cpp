#include "window.h"
#include <cstdint>

namespace {

// 手工像素生成窗口图标（不依赖 OpenGL / RenderTexture）
sf::Image makeWindowIcon() {
    constexpr unsigned kSize = 64;
    sf::Image img({kSize, kSize}, sf::Color::Transparent);

    auto setPixel = [&](unsigned x, unsigned y, sf::Color c) {
        if (x < kSize && y < kSize) {
            img.setPixel({x, y}, c);
        }
    };

    const sf::Color kBody   (95, 190, 95);
    const sf::Color kBorder (45, 110, 45);
    const sf::Color kBlack  (15, 15, 15);

    // 绿色方块身体
    for (unsigned y = 8; y < 56; ++y)
        for (unsigned x = 8; x < 56; ++x)
            setPixel(x, y, kBody);

    // 深绿边框
    for (unsigned x = 8; x < 56; ++x) {
        setPixel(x, 8, kBorder);
        setPixel(x, 55, kBorder);
    }
    for (unsigned y = 8; y < 56; ++y) {
        setPixel(8, y, kBorder);
        setPixel(55, y, kBorder);
    }

    // 眼睛
    for (unsigned y = 20; y < 28; ++y)
        for (unsigned x = 16; x < 24; ++x)
            setPixel(x, y, kBlack);
    for (unsigned y = 20; y < 28; ++y)
        for (unsigned x = 40; x < 48; ++x)
            setPixel(x, y, kBlack);

    // 嘴横线
    for (unsigned x = 20; x < 44; ++x)
        for (unsigned y = 36; y < 40; ++y)
            setPixel(x, y, kBlack);

    // 嘴两个竖
    for (unsigned y = 36; y < 48; ++y) {
        for (unsigned x = 20; x < 24; ++x)
            setPixel(x, y, kBlack);
        for (unsigned x = 40; x < 44; ++x)
            setPixel(x, y, kBlack);
    }

    return img;
}

} // namespace

Window::Window(unsigned width, unsigned height, const std::string& title, bool fullscreen,
               unsigned antiAliasing)
      : title_(title),
        antiAliasing_(antiAliasing) {
    sf::ContextSettings settings;
    settings.antiAliasingLevel = antiAliasing_;

    window_.create(sf::VideoMode({width, height}), title_,
                   fullscreen ? sf::State::Fullscreen : sf::State::Windowed, settings);

    window_.setVerticalSyncEnabled(vsync_);
    applyView();
    applyIcon();
}

void Window::applyView() {
    auto size = window_.getSize();
    window_.setView(sf::View(sf::FloatRect(
        {0.f, 0.f}, {static_cast<float>(size.x), static_cast<float>(size.y)})));
}

void Window::applyIcon() {
    static const sf::Image icon = makeWindowIcon();
    window_.setIcon(icon);
}

bool Window::isOpen() const {
    return window_.isOpen();
}
void Window::close() {
    window_.close();
}
void Window::clear() {
    window_.clear(sf::Color::Black);
}
void Window::display() {
    window_.display();
}

void Window::pollEvents(const std::function<void(const sf::Event&)>& handler) {
    while (const auto event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        }
        if (handler)
            handler(*event);
    }
}

bool Window::isKeyPressed(sf::Keyboard::Key key) const {
    return sf::Keyboard::isKeyPressed(key);
}

void Window::draw(const sf::Drawable& drawable) {
    window_.draw(drawable);
}

void Window::recreate(unsigned width, unsigned height, bool fullscreen) {
    sf::ContextSettings settings;
    settings.antiAliasingLevel = antiAliasing_;

    window_.create(sf::VideoMode({width, height}), title_,
                   fullscreen ? sf::State::Fullscreen : sf::State::Windowed, settings);

    window_.setVerticalSyncEnabled(vsync_);
    window_.setFramerateLimit(framerateLimit_);
    applyView();
    applyIcon();
}

void Window::setTitle(const std::string& title) {
    title_ = title;
    window_.setTitle(title_);
}

void Window::setVsync(bool enabled) {
    vsync_ = enabled;
    window_.setVerticalSyncEnabled(vsync_);
}

void Window::setFramerateLimit(unsigned limit) {
    framerateLimit_ = limit;
    window_.setFramerateLimit(framerateLimit_);
}

sf::RenderWindow& Window::native() {
    return window_;
}

bool Window::isFocused() const {
    return window_.hasFocus();
}