#include "window.h"

Window::Window(unsigned width, unsigned height, const std::string& title)
    : window_(sf::VideoMode({width, height}), title) {
    window_.setVerticalSyncEnabled(true);
}

bool Window::isOpen() const { return window_.isOpen(); }
void Window::close()        { window_.close(); }
void Window::clear()        { window_.clear(sf::Color::Black); }
void Window::display()      { window_.display(); }

void Window::pollEvents() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        }
    }
}

bool Window::isKeyPressed(sf::Keyboard::Key key) const {
    return sf::Keyboard::isKeyPressed(key);
}

void Window::draw(const sf::Drawable& drawable) {
    window_.draw(drawable);
}

sf::RenderWindow& Window::native() { return window_; }