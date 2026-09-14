#include "level.h"
#include "camera.h"
#include <algorithm>
#include <fstream>
#include <sstream>

bool Level::loadFromString(const std::string& text) {
    tiles_.clear();
    enemySpawns_.clear();
    coinSpawns_.clear();
    hasGoal_ = false;

    std::istringstream iss(text);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    if (lines.empty())
        return false;

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
            case 'P':
                playerSpawn_ = {px, py};
                c = ' ';
                break;
            case 'E':
                enemySpawns_.push_back({px, py});
                c = ' ';
                break;
            case 'C':
                coinSpawns_.push_back({px, py});
                c = ' ';
                break;
            case 'G':
                goalPos_ = {px, py};
                hasGoal_ = true;
                c = ' ';
                break;
            default:
                break;
            }
            tiles_[static_cast<size_t>(y * width_ + x)] = c;
        }
    }
    return true;
}

bool Level::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in)
        return false;
    std::stringstream ss;
    ss << in.rdbuf();
    return loadFromString(ss.str());
}

char Level::tileAt(int tx, int ty) const {
    if (tx < 0 || tx >= width_ || ty < 0 || ty >= height_)
        return ' ';
    return tiles_[static_cast<size_t>(ty * width_ + tx)];
}

bool Level::isSolid(int tx, int ty) const {
    return tileAt(tx, ty) == '#';
}

void Level::render(sf::RenderTarget& target, float camLeft, float camTop, float camW,
                   float camH) const {
    int ts = tileSize_;

    int left = std::max(0, static_cast<int>(camLeft / ts));
    int right = std::min(width_, static_cast<int>((camLeft + camW) / ts) + 1);
    int top = std::max(0, static_cast<int>(camTop / ts));
    int bottom = std::min(height_, static_cast<int>((camTop + camH) / ts) + 1);

    sf::RectangleShape rect({static_cast<float>(ts), static_cast<float>(ts)});
    rect.setFillColor(sf::Color(80, 80, 100));
    rect.setOutlineThickness(1.f);
    rect.setOutlineColor(sf::Color(60, 60, 80));

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            if (tiles_[static_cast<size_t>(y * width_ + x)] != '#')
                continue;
            // 世界坐标，直接画
            rect.setPosition({static_cast<float>(x * ts), static_cast<float>(y * ts)});
            target.draw(rect);
        }
    }
}