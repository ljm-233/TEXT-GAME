#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>

class Console {
public:
    Console(const sf::Font& font, sf::Vector2u size);

    void handleTextEntered(char32_t unicode);
    void handleKeyPressed(sf::Keyboard::Key key);

    void appendText(const std::string& text);

    std::string waitForLine();

    void shutdown();
    bool isShutdown() const;

    void render(sf::RenderTarget& target);

private:
    void submitCurrentInput();
    void flushOutputBuffer();

    const sf::Font& font_;

    std::deque<std::string> lines_;
    std::string currentInput_;
    std::string outputBuffer_;

    std::vector<std::string> history_;
    int historyIndex_ = -1;

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::string pendingLine_;
    bool lineReady_ = false;
    bool shutdown_ = false;

    sf::Text text_;
    sf::RectangleShape inputLine_;
};