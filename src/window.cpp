#include "window.h"

Window::Window(unsigned width, unsigned height,
               const std::string& title, bool fullscreen)
    : title_(title) {
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    window_.create(
        sf::VideoMode({width, height}),
        title_,
        fullscreen ? sf::State::Fullscreen : sf::State::Windowed,
        settings);

    window_.setVerticalSyncEnabled(vsync_);
    applyView();
}

void Window::applyView() {
    auto size = window_.getSize();
    window_.setView(sf::View(sf::FloatRect(
        {0.f, 0.f},
        {static_cast<float>(size.x), static_cast<float>(size.y)})));
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

void Window::recreate(unsigned width, unsigned height, bool fullscreen) {
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    window_.create(
        sf::VideoMode({width, height}),
        title_,
        fullscreen ? sf::State::Fullscreen : sf::State::Windowed,
        settings);

    window_.setVerticalSyncEnabled(vsync_);
    applyView();
}

void Window::setVsync(bool enabled) {
    vsync_ = enabled;
    window_.setVerticalSyncEnabled(vsync_);
}

sf::RenderWindow& Window::native() { return window_; }