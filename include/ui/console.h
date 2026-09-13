#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>

class Console {
public:
    Console(const sf::Font& font,
            unsigned fontSize,
            unsigned lineHeight,
            unsigned maxLines,
            bool autoScroll,
            bool blinkCursor,
            sf::Vector2u size);

    void handleTextEntered(char32_t unicode);
    void handleKeyPressed(sf::Keyboard::Key key);
    void handleMouseWheel(float delta);

    void appendText(const std::string& text);
    std::string waitForLine();

    void shutdown();
    bool isShutdown() const;

    void render(sf::RenderTarget& target);

private:
    void submitCurrentInput();
    void flushOutputBuffer();

    const sf::Font& font_;
    unsigned maxLines_;
    unsigned lineHeight_;
    bool     autoScroll_;
    bool     blinkCursor_;

    std::deque<std::string> lines_;
    std::string currentInput_;
    std::string outputBuffer_;

    std::vector<std::string> history_;
    int  historyIndex_ = -1;
    int  scrollOffset_ = 0;

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::string pendingLine_;
    bool lineReady_ = false;
    bool shutdown_ = false;

    sf::Text text_;
    sf::RectangleShape inputLine_;

    sf::Clock blinkClock_;
};