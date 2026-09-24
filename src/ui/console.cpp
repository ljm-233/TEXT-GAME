#include "console.h"
#include "text_strings.h"
#include "ui_scale.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <streambuf>

namespace {

std::string utf8Encode(char32_t cp) {
    std::string out;
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return out;
}

void popUtf8Char(std::string& s) {
    if (s.empty())
        return;
    size_t i = s.size();
    while (i > 0) {
        --i;
        if ((s[i] & 0xC0) != 0x80)
            break;
    }
    s.erase(i);
}

} // namespace

class ConsoleStreamBuf : public std::streambuf {
public:
    explicit ConsoleStreamBuf(Console* c)
          : console_(c) {}

protected:
    int_type overflow(int_type c) override {
        if (c != traits_type::eof()) {
            char ch = static_cast<char>(c);
            console_->appendText(std::string(1, ch));
        }
        return c;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override {
        console_->appendText(std::string(s, static_cast<size_t>(n)));
        return n;
    }

    int sync() override { return 0; }

    int_type underflow() override {
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());
        if (console_->isShutdown())
            return traits_type::eof();
        std::string line = console_->waitForLine();
        if (console_->isShutdown())
            return traits_type::eof();
        line += '\n';
        lineBuffer_ = std::move(line);
        char* base = lineBuffer_.data();
        setg(base, base, base + lineBuffer_.size());
        return traits_type::to_int_type(*gptr());
    }

private:
    Console* console_;
    std::string lineBuffer_;
};

std::unique_ptr<std::streambuf> makeConsoleStreamBuf(Console* c) {
    return std::make_unique<ConsoleStreamBuf>(c);
}

Console::Console(const sf::Font& font, unsigned fontSize, unsigned lineHeight,
                 unsigned maxLines, bool autoScroll, bool blinkCursor,
                 sf::Vector2u /*size*/)
      : font_(font),
        maxLines_(maxLines),
        lineHeight_(lineHeight),
        autoScroll_(autoScroll),
        blinkCursor_(blinkCursor),
        text_(font, sf::String(), scaledFontSize(fontSize)) {
    text_.setFillColor(sf::Color(220, 220, 220));
    inputLine_.setFillColor(sf::Color(20, 20, 30, 220));
    inputLine_.setOutlineThickness(1.f);
    inputLine_.setOutlineColor(sf::Color(80, 80, 110));

    lines_.push_back(Str::ConsoleTitle);
    lines_.push_back(Str::ConsoleHint);
    lines_.push_back("");
}

void Console::appendText(const std::string& text) {
    std::lock_guard<std::mutex> lock(mtx_);
    size_t i = 0;
    while (i < text.size()) {
        char c = text[i];
        if (c == '\x1b' && i + 1 < text.size() && text[i + 1] == '[') {
            i += 2;
            while (i < text.size()) {
                char e = text[i];
                if ((e >= 'a' && e <= 'z') || (e >= 'A' && e <= 'Z')) {
                    ++i;
                    break;
                }
                ++i;
            }
        } else {
            outputBuffer_ += c;
            ++i;
        }
    }
}

void Console::flushOutputBuffer() {
    size_t pos;
    while ((pos = outputBuffer_.find('\n')) != std::string::npos) {
        std::string line = outputBuffer_.substr(0, pos);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines_.push_back(line);
        if (lines_.size() > maxLines_)
            lines_.pop_front();
        outputBuffer_.erase(0, pos + 1);
    }
    // 新内容到达时，如果处于自动滚动模式，把偏移归零
    if (autoScroll_)
        scrollOffset_ = 0;
}

void Console::submitCurrentInput() {
    std::string line;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        flushOutputBuffer();
        line = currentInput_;
        if (!line.empty()) {
            history_.push_back(line);
            if (history_.size() > 100)
                history_.erase(history_.begin());
        }
        lines_.push_back(prompt_ + line);
        if (lines_.size() > maxLines_)
            lines_.pop_front();
        currentInput_.clear();
        pendingLine_ = line;
        lineReady_ = true;
        historyIndex_ = -1;
        scrollOffset_ = 0;
    }
    cv_.notify_one();
}

void Console::handleTextEntered(char32_t unicode) {
    if (unicode < 32 || unicode == 127)
        return;
    std::lock_guard<std::mutex> lock(mtx_);
    currentInput_ += utf8Encode(unicode);
}

void Console::handleKeyPressed(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Backspace) {
        std::lock_guard<std::mutex> lock(mtx_);
        popUtf8Char(currentInput_);
    } else if (key == sf::Keyboard::Key::Enter) {
        submitCurrentInput();
    } else if (key == sf::Keyboard::Key::Up) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (history_.empty())
            return;
        if (historyIndex_ == -1)
            historyIndex_ = static_cast<int>(history_.size()) - 1;
        else if (historyIndex_ > 0)
            --historyIndex_;
        currentInput_ = history_[historyIndex_];
    } else if (key == sf::Keyboard::Key::Down) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (historyIndex_ == -1)
            return;
        if (historyIndex_ + 1 < static_cast<int>(history_.size())) {
            ++historyIndex_;
            currentInput_ = history_[historyIndex_];
        } else {
            historyIndex_ = -1;
            currentInput_.clear();
        }
    } else if (key == sf::Keyboard::Key::PageUp) {
        std::lock_guard<std::mutex> lock(mtx_);
        scrollOffset_ += 5;
        int maxOff = std::max(0, static_cast<int>(lines_.size()) - 5);
        scrollOffset_ = std::min(scrollOffset_, maxOff);
    } else if (key == sf::Keyboard::Key::PageDown) {
        std::lock_guard<std::mutex> lock(mtx_);
        scrollOffset_ = std::max(0, scrollOffset_ - 5);
    }
}

void Console::handleMouseWheel(float delta) {
    std::lock_guard<std::mutex> lock(mtx_);
    // delta > 0 表示向上滚
    scrollOffset_ += static_cast<int>(delta);
    scrollOffset_ = std::max(0, scrollOffset_);
    int maxOff = std::max(0, static_cast<int>(lines_.size()) - 5);
    scrollOffset_ = std::min(scrollOffset_, maxOff);
}

std::string Console::waitForLine() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return lineReady_ || shutdown_; });
    if (shutdown_)
        return "";
    std::string line = pendingLine_;
    pendingLine_.clear();
    lineReady_ = false;
    return line;
}

void Console::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        shutdown_ = true;
    }
    cv_.notify_all();
}

void Console::resetShutdown() {
    std::lock_guard<std::mutex> lock(mtx_);
    shutdown_ = false;
    lineReady_ = false;
    pendingLine_.clear();
}

void Console::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    lines_.clear();
    outputBuffer_.clear();
    scrollOffset_ = 0;
}

bool Console::isShutdown() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return shutdown_;
}

void Console::render(sf::RenderTarget& target) {
    auto vs = target.getView().getSize();
    float w = vs.x;
    float h = vs.y;

    const float margin = 24.f;
    const float lineH = static_cast<float>(lineHeight_);
    const float padX = 12.f;
    const float inputH = 40.f;

    float outputTop = margin;
    float outputBottom = h - inputH - margin - 12.f;

    std::deque<std::string> display;
    std::string tail;
    std::string inputDisplay;
    int offset;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        flushOutputBuffer();
        tail = outputBuffer_;
        display = lines_;
        inputDisplay = prompt_ + currentInput_;
        offset = scrollOffset_;
    }

    int maxLines = static_cast<int>((outputBottom - outputTop) / lineH);
    if (maxLines < 1)
        maxLines = 1;

    // 显示窗口：末尾往前 offset 行
    int total = static_cast<int>(display.size());
    int endIdx = std::max(0, total - offset);
    int startIdx = std::max(0, endIdx - maxLines);
    for (int i = 0; i < startIdx; ++i)
        display.pop_front();
    while (static_cast<int>(display.size()) > endIdx - startIdx && !display.empty()) {
        display.pop_back();
    }

    // 未换行的尾部：只在 offset==0 时并入最后一行
    if (!tail.empty() && offset == 0) {
        if (display.empty())
            display.push_back(tail);
        else
            display.back() += tail;
    }

    text_.setFillColor(sf::Color(220, 220, 220));
    float y = outputTop;
    for (const auto& line : display) {
        text_.setString(sf::String::fromUtf8(line.begin(), line.end()));
        text_.setPosition({margin, y});
        target.draw(text_);
        y += lineH;
    }

    // 滚动条指示（当偏离底部时显示）
    if (offset > 0) {
        std::string hint = "↑ " + std::to_string(offset);
        text_.setString(sf::String::fromUtf8(hint.begin(), hint.end()));
        text_.setFillColor(sf::Color(180, 180, 200));
        auto b = text_.getLocalBounds();
        text_.setPosition({w - b.size.x - 24.f, 6.f});
        target.draw(text_);
    }

    // 输入行
    inputLine_.setSize({w - 2 * margin, inputH});
    inputLine_.setPosition({margin, h - inputH - margin});
    target.draw(inputLine_);

    // 光标闪烁
    std::string cursor;
    if (blinkCursor_) {
        auto ms = blinkClock_.getElapsedTime().asMilliseconds();
        cursor = ((ms / 500) % 2 == 0) ? "▊" : " ";
    }
    inputDisplay += cursor;

    text_.setFillColor(sf::Color(230, 230, 230));
    text_.setString(sf::String::fromUtf8(inputDisplay.begin(), inputDisplay.end()));
    text_.setPosition({margin + padX, h - inputH - margin + 8.f});
    target.draw(text_);
}