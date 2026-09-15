#include "level.h"
#include "camera.h"
#include "utf8.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

bool Level::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    return loadFromString(ss.str());
}

bool Level::loadFromString(const std::string& text) {
    tiles_.clear();
    enemySpawns_.clear();
    coinSpawns_.clear();
    jumpPadSpawns_.clear();
    checkpointSpawns_.clear();
    movingPlatformSpawns_.clear();
    verticalPlatformSpawns_.clear();
    hasGoal_ = false;

    std::istringstream iss(text);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    if (lines.empty()) return false;

    height_ = static_cast<int>(lines.size());
    width_ = 0;
    for (const auto& l : lines)
        width_ = std::max(width_, static_cast<int>(l.size()));

    tiles_.assign(static_cast<size_t>(width_ * height_), ' ');

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < static_cast<int>(lines[y].size()); ++x) {
            char c = lines[y][x];
            float px = static_cast<float>(x * tileSize_);
            float py = static_cast<float>(y * tileSize_);
            switch (c) {
                case 'P': playerSpawn_ = {px, py}; c = ' '; break;
                case 'E': enemySpawns_.push_back({px, py}); c = ' '; break;
                case 'C': coinSpawns_.push_back({px, py}); c = ' '; break;
                case 'J': jumpPadSpawns_.push_back({px, py}); c = ' '; break;
                case 'S': checkpointSpawns_.push_back({px, py}); c = ' '; break;
                case 'M': movingPlatformSpawns_.push_back({px, py}); c = ' '; break;
                case 'V': verticalPlatformSpawns_.push_back({px, py}); c = ' '; break;
                case 'G': goalPos_ = {px, py}; hasGoal_ = true; c = ' '; break;
                default: break;
            }
            tiles_[static_cast<size_t>(y * width_ + x)] = c;
        }
    }
    return true;
}

char Level::tileAt(int tx, int ty) const {
    if (tx < 0 || tx >= width_ || ty < 0 || ty >= height_) return ' ';
    return tiles_[static_cast<size_t>(ty * width_ + tx)];
}

bool Level::isSolid(int tx, int ty) const {
    return tileAt(tx, ty) == '#';
}

void Level::render(sf::RenderTarget& target,
                   float camLeft, float camTop,
                   float camW,    float camH) const {
    int ts = tileSize_;
    float tsF = static_cast<float>(ts);
    float time = animClock_.getElapsedTime().asSeconds();

    int left   = std::max(0, static_cast<int>(camLeft / ts));
    int right  = std::min(width_,  static_cast<int>((camLeft + camW) / ts) + 1);
    int top    = std::max(0, static_cast<int>(camTop / ts));
    int bottom = std::min(height_, static_cast<int>((camTop + camH) / ts) + 1);

    // ============================================================
    // 伪 3D 瓦片
    // ============================================================
    const sf::Color kBody  (80, 80, 100);
    const sf::Color kTop   (130, 130, 155);
    const sf::Color kLeft  (100, 100, 120);
    const sf::Color kRight (50, 50, 70);
    const sf::Color kBottom(40, 40, 60);

    sf::RectangleShape rect({tsF, tsF});

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            if (tiles_[static_cast<size_t>(y * width_ + x)] != '#') continue;

            float px = static_cast<float>(x) * tsF;
            float py = static_cast<float>(y) * tsF;

            // 主体
            rect.setSize({tsF, tsF});
            rect.setFillColor(kBody);
            rect.setPosition({px, py});
            target.draw(rect);

            if (pseudo3D_) {
                // 顶面高光：只有上方是空的时候画
                if (!isSolid(x, y - 1)) {
                    sf::RectangleShape topStrip({tsF, 5.f});
                    topStrip.setFillColor(kTop);
                    topStrip.setPosition({px, py});
                    target.draw(topStrip);
                }

                // 左侧高光
                if (!isSolid(x - 1, y)) {
                    sf::RectangleShape leftStrip({4.f, tsF});
                    leftStrip.setFillColor(kLeft);
                    leftStrip.setPosition({px, py});
                    target.draw(leftStrip);
                }

                // 右侧阴影
                if (!isSolid(x + 1, y)) {
                    sf::RectangleShape rightStrip({4.f, tsF});
                    rightStrip.setFillColor(kRight);
                    rightStrip.setPosition({px + tsF - 4.f, py});
                    target.draw(rightStrip);
                }

                // 底部阴影
                if (!isSolid(x, y + 1)) {
                    sf::RectangleShape bottomStrip({tsF, 4.f});
                    bottomStrip.setFillColor(kBottom);
                    bottomStrip.setPosition({px, py + tsF - 4.f});
                    target.draw(bottomStrip);
                }
            }
        }
    }

    // ============================================================
    // 出生点图标
    // ============================================================
    {
        float cx = playerSpawn_.x + tsF * 0.5f;
        float cy = playerSpawn_.y + tsF * 0.5f;

        float pulse = 1.f + std::sin(time * 3.f) * 0.15f;

        float r1 = tsF * 0.45f * pulse;
        sf::CircleShape ring(r1);
        ring.setOrigin({r1, r1});
        ring.setPosition({cx, cy});
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.5f);
        ring.setOutlineColor(sf::Color(120, 200, 255, 220));
        target.draw(ring);

        float pulse2 = 1.f + std::sin(time * 3.f + 1.5f) * 0.10f;
        float r2 = tsF * 0.28f * pulse2;
        sf::CircleShape ring2(r2);
        ring2.setOrigin({r2, r2});
        ring2.setPosition({cx, cy});
        ring2.setFillColor(sf::Color::Transparent);
        ring2.setOutlineThickness(1.5f);
        ring2.setOutlineColor(sf::Color(160, 220, 255, 180));
        target.draw(ring2);

        float dotAlpha = 180.f + std::sin(time * 4.f) * 60.f;
        float r3 = tsF * 0.14f;
        sf::CircleShape dot(r3);
        dot.setOrigin({r3, r3});
        dot.setPosition({cx, cy});
        dot.setFillColor(sf::Color(200, 235, 255,
            static_cast<std::uint8_t>(std::clamp(dotAlpha, 0.f, 255.f))));
        target.draw(dot);
    }

    // ============================================================
    // 终点图标
    // ============================================================
    if (hasGoal_) {
        float gx = goalPos_.x;
        float gy = goalPos_.y;

        float haloPulse = 1.f + std::sin(time * 2.2f) * 0.12f;
        float rH1 = tsF * 0.9f * haloPulse;
        sf::CircleShape halo(rH1);
        halo.setOrigin({rH1, rH1});
        halo.setPosition({gx + tsF * 0.5f, gy + tsF * 0.5f});
        halo.setFillColor(sf::Color(255, 220, 80, 60));
        target.draw(halo);

        float haloPulse2 = 1.f + std::sin(time * 2.2f + 1.f) * 0.10f;
        float rH2 = tsF * 0.6f * haloPulse2;
        sf::CircleShape halo2(rH2);
        halo2.setOrigin({rH2, rH2});
        halo2.setPosition({gx + tsF * 0.5f, gy + tsF * 0.5f});
        halo2.setFillColor(sf::Color(255, 240, 120, 110));
        target.draw(halo2);

        sf::RectangleShape pole({3.f, tsF * 1.5f});
        pole.setPosition({gx + tsF * 0.35f, gy - tsF * 0.3f});
        pole.setFillColor(sf::Color(230, 230, 240));
        pole.setOutlineThickness(1.f);
        pole.setOutlineColor(sf::Color(120, 120, 140));
        target.draw(pole);

        float flagWave = std::sin(time * 5.f) * 3.f;
        float flagTop = gy - tsF * 0.25f + std::sin(time * 4.f) * 1.5f;

        sf::ConvexShape flag;
        flag.setPointCount(3);
        flag.setPoint(0, {gx + tsF * 0.35f + 3.f, flagTop});
        flag.setPoint(1, {gx + tsF * 0.35f + 3.f, flagTop + tsF * 0.7f});
        flag.setPoint(2, {gx + tsF * 1.35f + flagWave, flagTop + tsF * 0.35f});
        flag.setFillColor(sf::Color(230, 60, 60));
        flag.setOutlineThickness(1.f);
        flag.setOutlineColor(sf::Color(140, 20, 20));
        target.draw(flag);

        if (font_) {
            float labelBob = std::sin(time * 2.f) * 4.f;
            sf::Text label(*font_, sf::String("GOAL"), 20);
            label.setFillColor(sf::Color(255, 240, 100));
            label.setOutlineThickness(2.f);
            label.setOutlineColor(sf::Color(80, 40, 0));

            auto b = label.getLocalBounds();
            label.setOrigin({b.position.x + b.size.x / 2.f,
                             b.position.y + b.size.y / 2.f});
            label.setPosition({gx + tsF * 0.5f, gy - tsF * 1.1f + labelBob});
            target.draw(label);
        }
    }
}