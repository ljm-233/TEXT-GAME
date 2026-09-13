#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>

class Console {
public:
    Console(const sf::Font& font, sf::Vector2u size);

    // 主线程：输入事件
    void handleTextEntered(char32_t unicode);
    void handleKeyPressed(sf::Keyboard::Key key);

    // worker 线程：输出
    void appendText(const std::string& text);

    // worker 线程：阻塞等待用户提交一行（按下回车）
    std::string waitForLine();

    // 主线程：关闭（让 waitForLine 提前返回，worker 能退出）
    void shutdown();
    bool isShutdown() const;

    // 主线程：渲染
    void render(sf::RenderTarget& target);

private:
    void submitCurrentInput();
    void flushOutputBuffer();

    const sf::Font& font_;

    std::deque<std::string> lines_;
    std::string currentInput_;
    std::string outputBuffer_;

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::string pendingLine_;
    bool lineReady_ = false;
    bool shutdown_ = false;

    sf::Text text_;
    sf::RectangleShape inputLine_;
};