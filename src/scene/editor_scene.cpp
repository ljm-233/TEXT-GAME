#include "editor_scene.h"
#include "text_strings.h"
#include "level_codec.h"
#include "level.h"
#include "level_validator.h"
#include "utf8.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

// ============================================================
// 笔刷定义——加新元素只需要在这里加一行
// ============================================================
struct BrushItem {
    char ch;
    const char* name;
    sf::Color color;
};

const BrushItem kBrushes[] = {
    {'#', "墙",   sf::Color(120, 120, 145)},
    {' ', "橡皮", sf::Color(180, 70, 70)},
    {'P', "玩家", sf::Color(100, 180, 255)},
    {'E', "敌人", sf::Color(240, 80, 80)},
    {'C', "金币", sf::Color(255, 215, 65)},
    {'G', "终点", sf::Color(80, 240, 120)},
    {'^', "尖刺", sf::Color(200, 200, 210)},
    {'J', "跳台", sf::Color(0, 200, 255)},
    {'S', "存档", sf::Color(255, 128, 0)},
    {'K', "钥匙", sf::Color(255, 210, 60)},
    {'L', "门",   sf::Color(160, 100, 60)},
    {'M', "横台", sf::Color(180, 100, 200)},
    {'V', "竖台", sf::Color(180, 100, 200)},
};

constexpr int kBrushCount = static_cast<int>(sizeof(kBrushes) / sizeof(kBrushes[0]));

constexpr float kButtonW = 84.f;
constexpr float kButtonH = 60.f;
constexpr float kButtonGap = 6.f;

// ============================================================
// 图标绘制
// ============================================================
void drawTileIcon(sf::RenderTarget& rt, char ch, float x, float y, float s) {
    const float cx = x + s * 0.5f;
    const float cy = y + s * 0.5f;

    switch (ch) {
        case 'P': {
            sf::RectangleShape body({s * 0.55f, s * 0.55f});
            body.setOrigin({s * 0.275f, s * 0.275f});
            body.setPosition({cx, cy});
            body.setFillColor(sf::Color(95, 190, 95));
            body.setOutlineThickness(2.f);
            body.setOutlineColor(sf::Color(45, 110, 45));
            rt.draw(body);

            sf::RectangleShape eye({s * 0.09f, s * 0.09f});
            eye.setFillColor(sf::Color(15, 15, 15));
            eye.setPosition({cx - s * 0.17f, cy - s * 0.08f});
            rt.draw(eye);
            eye.setPosition({cx + s * 0.08f, cy - s * 0.08f});
            rt.draw(eye);
            break;
        }
        case 'E': {
            sf::RectangleShape body({s * 0.66f, s * 0.5f});
            body.setOrigin({s * 0.33f, s * 0.25f});
            body.setPosition({cx, cy});
            body.setFillColor(sf::Color(220, 80, 80));
            body.setOutlineThickness(2.f);
            body.setOutlineColor(sf::Color(140, 40, 40));
            rt.draw(body);

            sf::RectangleShape eye({s * 0.14f, s * 0.14f});
            eye.setFillColor(sf::Color(255, 255, 255));
            eye.setPosition({cx - s * 0.22f, cy - s * 0.1f});
            rt.draw(eye);
            eye.setPosition({cx + s * 0.08f, cy - s * 0.1f});
            rt.draw(eye);

            sf::RectangleShape brow({s * 0.14f, s * 0.04f});
            brow.setFillColor(sf::Color(100, 20, 20));
            brow.setPosition({cx - s * 0.22f, cy - s * 0.17f});
            rt.draw(brow);
            brow.setPosition({cx + s * 0.08f, cy - s * 0.17f});
            rt.draw(brow);
            break;
        }
        case 'C': {
            float r = s * 0.28f;
            sf::CircleShape coin(r);
            coin.setOrigin({r, r});
            coin.setPosition({cx, cy});
            coin.setFillColor(sf::Color(255, 215, 65));
            coin.setOutlineThickness(2.f);
            coin.setOutlineColor(sf::Color(170, 115, 15));
            rt.draw(coin);

            float ir = r * 0.55f;
            sf::CircleShape inner(ir);
            inner.setOrigin({ir, ir});
            inner.setPosition({cx, cy});
            inner.setFillColor(sf::Color::Transparent);
            inner.setOutlineThickness(1.5f);
            inner.setOutlineColor(sf::Color(240, 190, 45));
            rt.draw(inner);
            break;
        }
        case 'G': {
            sf::RectangleShape pole({s * 0.08f, s * 0.72f});
            pole.setPosition({cx - s * 0.18f, cy - s * 0.36f});
            pole.setFillColor(sf::Color(230, 230, 240));
            pole.setOutlineThickness(1.f);
            pole.setOutlineColor(sf::Color(120, 120, 140));
            rt.draw(pole);

            sf::ConvexShape flag;
            flag.setPointCount(3);
            flag.setPoint(0, {cx - s * 0.1f, cy - s * 0.36f});
            flag.setPoint(1, {cx - s * 0.1f, cy - s * 0.02f});
            flag.setPoint(2, {cx + s * 0.34f, cy - s * 0.19f});
            flag.setFillColor(sf::Color(230, 60, 60));
            flag.setOutlineThickness(1.f);
            flag.setOutlineColor(sf::Color(140, 20, 20));
            rt.draw(flag);
            break;
        }
        case '^': {
            for (int i = 0; i < 3; ++i) {
                float sx = x + s * 0.02f + static_cast<float>(i) * s * 0.33f;
                sf::ConvexShape spike;
                spike.setPointCount(3);
                spike.setPoint(0, {sx, y + s});
                spike.setPoint(1, {sx + s * 0.155f, y + s * 0.15f});
                spike.setPoint(2, {sx + s * 0.31f, y + s});
                spike.setFillColor(sf::Color(200, 200, 210));
                spike.setOutlineThickness(1.f);
                spike.setOutlineColor(sf::Color(120, 120, 140));
                rt.draw(spike);
            }
            break;
        }
        case 'J': {
            sf::RectangleShape base({s * 0.8f, s * 0.35f});
            base.setOrigin({s * 0.4f, s * 0.175f});
            base.setPosition({cx, cy + s * 0.2f});
            base.setFillColor(sf::Color(60, 200, 90));
            base.setOutlineThickness(2.f);
            base.setOutlineColor(sf::Color(30, 120, 50));
            rt.draw(base);

            sf::ConvexShape arrow;
            arrow.setPointCount(3);
            arrow.setPoint(0, {cx, cy - s * 0.36f});
            arrow.setPoint(1, {cx - s * 0.15f, cy - s * 0.08f});
            arrow.setPoint(2, {cx + s * 0.15f, cy - s * 0.08f});
            arrow.setFillColor(sf::Color(180, 255, 180));
            arrow.setOutlineThickness(1.f);
            arrow.setOutlineColor(sf::Color(30, 120, 50));
            rt.draw(arrow);
            break;
        }
        case 'S': {
            sf::RectangleShape pole({s * 0.1f, s * 0.7f});
            pole.setPosition({cx - s * 0.05f, cy - s * 0.35f});
            pole.setFillColor(sf::Color(150, 150, 160));
            pole.setOutlineThickness(1.f);
            pole.setOutlineColor(sf::Color(80, 80, 90));
            rt.draw(pole);

            sf::RectangleShape flag({s * 0.42f, s * 0.28f});
            flag.setPosition({cx + s * 0.05f, cy - s * 0.3f});
            flag.setFillColor(sf::Color(80, 220, 100));
            flag.setOutlineThickness(1.f);
            flag.setOutlineColor(sf::Color(40, 120, 60));
            rt.draw(flag);
            break;
        }
        case 'K': {
            float r = s * 0.13f;
            sf::CircleShape ring(r);
            ring.setOrigin({r, r});
            ring.setPosition({cx - s * 0.08f, cy - s * 0.18f});
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.f);
            ring.setOutlineColor(sf::Color(255, 210, 60));
            rt.draw(ring);

            sf::RectangleShape shaft({s * 0.08f, s * 0.42f});
            shaft.setPosition({cx - s * 0.12f, cy - s * 0.05f});
            shaft.setFillColor(sf::Color(255, 210, 60));
            rt.draw(shaft);

            sf::RectangleShape tooth({s * 0.16f, s * 0.07f});
            tooth.setPosition({cx - s * 0.04f, cy + s * 0.2f});
            tooth.setFillColor(sf::Color(255, 210, 60));
            rt.draw(tooth);
            tooth.setPosition({cx - s * 0.04f, cy + s * 0.32f});
            rt.draw(tooth);
            break;
        }
        case 'L': {
            sf::RectangleShape frame({s * 0.68f, s * 0.72f});
            frame.setOrigin({s * 0.34f, s * 0.36f});
            frame.setPosition({cx, cy});
            frame.setFillColor(sf::Color(90, 60, 30));
            frame.setOutlineThickness(2.f);
            frame.setOutlineColor(sf::Color(50, 30, 15));
            rt.draw(frame);

            sf::RectangleShape panel({s * 0.5f, s * 0.6f});
            panel.setOrigin({s * 0.25f, s * 0.3f});
            panel.setPosition({cx, cy});
            panel.setFillColor(sf::Color(140, 90, 50));
            rt.draw(panel);

            float kr = s * 0.055f;
            sf::CircleShape knob(kr);
            knob.setOrigin({kr, kr});
            knob.setPosition({cx + s * 0.16f, cy});
            knob.setFillColor(sf::Color(255, 210, 60));
            knob.setOutlineThickness(1.f);
            knob.setOutlineColor(sf::Color(120, 90, 20));
            rt.draw(knob);
            break;
        }
        case 'M':
        case 'V': {
            sf::RectangleShape plat({s * 0.86f, s * 0.36f});
            plat.setOrigin({s * 0.43f, s * 0.18f});
            plat.setPosition({cx, cy});
            plat.setFillColor(sf::Color(160, 120, 80));
            plat.setOutlineThickness(2.f);
            plat.setOutlineColor(sf::Color(90, 60, 30));
            rt.draw(plat);

            sf::RectangleShape stripe({s * 0.86f, s * 0.06f});
            stripe.setOrigin({s * 0.43f, s * 0.03f});
            stripe.setPosition({cx, cy - s * 0.11f});
            stripe.setFillColor(sf::Color(220, 180, 120));
            rt.draw(stripe);

            sf::ConvexShape arrow;
            arrow.setPointCount(3);
            if (ch == 'M') {
                arrow.setPoint(0, {cx + s * 0.32f, cy + s * 0.4f});
                arrow.setPoint(1, {cx + s * 0.18f, cy + s * 0.28f});
                arrow.setPoint(2, {cx + s * 0.18f, cy + s * 0.5f});
            } else {
                arrow.setPoint(0, {cx + s * 0.32f, cy - s * 0.45f});
                arrow.setPoint(1, {cx + s * 0.2f,  cy - s * 0.3f});
                arrow.setPoint(2, {cx + s * 0.44f, cy - s * 0.3f});
            }
            arrow.setFillColor(sf::Color(220, 220, 240));
            arrow.setOutlineThickness(1.f);
            arrow.setOutlineColor(sf::Color(80, 80, 100));
            rt.draw(arrow);
            break;
        }
        default: {
            float r = s * 0.2f;
            sf::CircleShape dot(r);
            dot.setOrigin({r, r});
            dot.setPosition({cx, cy});
            dot.setFillColor(sf::Color(150, 150, 150));
            rt.draw(dot);
            break;
        }
    }
}

} // namespace

EditorScene::EditorScene(std::shared_ptr<Background>  background,
                         std::shared_ptr<Preferences> preferences,
                         const sf::Font&              font,
                         std::shared_ptr<Logger>      logger)
    : background_(std::move(background)),
      preferences_(std::move(preferences)),
      logger_(std::move(logger)),
      font_(&font),
      hudText_(font, sf::String(), 18),
      hintText_(font, sf::String(), 15),
      flashDraw_(font, sf::String(), 24) {

    hudText_.setFillColor(sf::Color(240, 240, 250));
    hudText_.setOutlineThickness(2.f);
    hudText_.setOutlineColor(sf::Color(0, 0, 0, 180));

    hintText_.setFillColor(sf::Color(200, 200, 220));
    hintText_.setOutlineThickness(2.f);
    hintText_.setOutlineColor(sf::Color(0, 0, 0, 180));

    flashDraw_.setFillColor(sf::Color(120, 255, 150));
    flashDraw_.setOutlineThickness(2.f);
    flashDraw_.setOutlineColor(sf::Color(0, 80, 0));
}

void EditorScene::onEnter() {
    nextScene_ = SceneId::None;
    scanLevelFiles();

    savePath_ = preferences_->assetFile("levels/editor.txt").string();
    for (std::size_t i = 0; i < levelFiles_.size(); ++i) {
        if (levelFiles_[i] == savePath_) {
            currentFileIdx_ = static_cast<int>(i);
            break;
        }
    }

    loadFile();
}

// ============================================================
// 文件扫描
// ============================================================

void EditorScene::scanLevelFiles() {
    levelFiles_.clear();
    auto dir = preferences_->assetFile("levels");

    if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".txt") continue;
            levelFiles_.push_back(entry.path().string());
        }
    }
    std::sort(levelFiles_.begin(), levelFiles_.end());

    std::string editorPath = preferences_->assetFile("levels/editor.txt").string();
    if (std::find(levelFiles_.begin(), levelFiles_.end(), editorPath) == levelFiles_.end()) {
        levelFiles_.push_back(editorPath);
        std::sort(levelFiles_.begin(), levelFiles_.end());
    }
}

std::string EditorScene::currentFileName() const {
    if (levelFiles_.empty()) return savePath_;
    if (currentFileIdx_ < 0 || currentFileIdx_ >= static_cast<int>(levelFiles_.size()))
        return savePath_;
    return std::filesystem::path(levelFiles_[currentFileIdx_]).filename().string();
}

void EditorScene::switchToNextFile() {
    if (levelFiles_.empty()) return;
    saveFile();
    currentFileIdx_ = (currentFileIdx_ + 1) % static_cast<int>(levelFiles_.size());
    savePath_ = levelFiles_[currentFileIdx_];
    loadFile();
    showFlash(Str::T(Str::EditorSwitchTo) + currentFileName(), 1.5f);
}

// ============================================================
// 撤销
// ============================================================

void EditorScene::pushUndo() {
    if (!undoStack_.empty() && undoStack_.back() == lines_) return;
    undoStack_.push_back(lines_);
    if (undoStack_.size() > kMaxUndo) {
        undoStack_.erase(undoStack_.begin());
    }
}

void EditorScene::undo() {
    if (undoStack_.empty()) {
        showFlash(Str::T(Str::EditorNoUndo), 0.8f);
        return;
    }
    lines_ = undoStack_.back();
    undoStack_.pop_back();
    refreshDimensions();
    geometryDirty_ = true;
    reachDirty_ = true;
    levelIconsDirty_ = true;
    showFlash(Str::T(Str::EditorUndone), 0.8f);
}

void EditorScene::refreshDimensions() {
    height_ = static_cast<int>(lines_.size());
    width_ = 0;
    for (const auto& l : lines_)
        width_ = std::max(width_, static_cast<int>(l.size()));
    for (auto& l : lines_) l.resize(width_, ' ');
}

// ============================================================
// 尺寸调整
// ============================================================

void EditorScene::resizeLevel(int newW, int newH) {
    newW = std::max(8, std::min(300, newW));
    newH = std::max(6, std::min(100, newH));

    if (newW == width_ && newH == height_) return;

    pushUndo();

    std::vector<std::string> newLines(newH, std::string(newW, ' '));

    int copyW = std::min(width_,  newW);
    int copyH = std::min(height_, newH);

    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            if (y < static_cast<int>(lines_.size()) &&
                x < static_cast<int>(lines_[y].size())) {
                newLines[y][x] = lines_[y][x];
            }
        }
    }

    lines_ = std::move(newLines);
    width_ = newW;
    height_ = newH;
    geometryDirty_ = true;
    reachDirty_ = true;
    levelIconsDirty_ = true;

    showFlash(Str::T(Str::EditorHudSize) + std::to_string(newW) + " x " + std::to_string(newH), 1.0f);
}

// ============================================================
// 文件读写
// ============================================================

void EditorScene::loadFile() {
    lines_.clear();

    std::ifstream in(savePath_);
    if (!in) {
        width_ = 40;
        height_ = 22;
        lines_.assign(height_, std::string(width_, ' '));
        for (int x = 0; x < width_; ++x) {
            lines_[0][x] = '#';
            lines_[height_ - 1][x] = '#';
        }
        for (int y = 0; y < height_; ++y) {
            lines_[y][0] = '#';
            lines_[y][width_ - 1] = '#';
        }
        lines_[2][3] = 'P';
        for (int x = 3; x <= 6; ++x) lines_[3][x] = '#';

        logger_->info("编辑器：创建新关卡 " + savePath_);
    } else {
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines_.push_back(line);
        }
        refreshDimensions();

        logger_->info("编辑器：加载 " + savePath_ + " " +
                      std::to_string(width_) + "x" + std::to_string(height_));
    }

    undoStack_.clear();
    geometryDirty_ = true;
    reachDirty_ = true;
    levelIconsDirty_ = true;
}

void EditorScene::saveFile() {
    std::ofstream out(savePath_);
    if (!out) {
        logger_->error("编辑器：无法写入 " + savePath_);
        showFlash(Str::T(Str::EditorSaveFailed), 2.0f);
        return;
    }
    for (const auto& line : lines_)
        out << line << '\n';
    out.flush();

    logger_->info("编辑器：已保存到 " + savePath_);
    showFlash(Str::T(Str::EditorSaved), 1.2f);
}

void EditorScene::showFlash(const std::string& text, float duration) {
    flashText_ = text;
    flashTimer_ = duration;
}

void EditorScene::exportShareCode() {
    std::string text;
    for (const auto& l : lines_) {
        text += l;
        text += '\n';
    }
    std::string code = LevelCodec::encode(text);

    auto path = preferences_->savesDir() / "share_code.txt";
    std::ofstream out(path);
    if (!out) {
        showFlash("导出失败", 2.0f);
        return;
    }
    out << code;
    out.flush();

    showFlash("已导出 (" + std::to_string(code.size()) + " 字符)", 2.0f);
    logger_->info("分享码已导出到 " + path.string() +
                  " (" + std::to_string(code.size()) + " 字符)");
}

void EditorScene::importShareCode() {
    auto path = preferences_->savesDir() / "share_code.txt";
    std::ifstream in(path);
    if (!in) {
        showFlash("找不到 share_code.txt", 2.0f);
        return;
    }

    std::string code;
    std::getline(in, code);
    if (code.empty()) {
        showFlash("share_code.txt 为空", 2.0f);
        return;
    }

    std::string text = LevelCodec::decode(code);
    if (text.empty()) {
        showFlash("分享码无效", 2.0f);
        return;
    }

    pushUndo();
    lines_.clear();
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines_.push_back(line);
    }
    refreshDimensions();
    geometryDirty_ = true;
    reachDirty_ = true;
    levelIconsDirty_ = true;

    showFlash("已导入 " + std::to_string(width_) + " x " + std::to_string(height_), 2.0f);
    logger_->info("分享码已导入 (" + std::to_string(code.size()) + " 字符)");
}

// ============================================================
// 瓦片访问
// ============================================================

char* EditorScene::tileAt(int tx, int ty) {
    if (tx < 0 || tx >= width_ || ty < 0 || ty >= height_) return nullptr;
    if (ty >= static_cast<int>(lines_.size())) return nullptr;
    if (tx >= static_cast<int>(lines_[ty].size())) return nullptr;
    return &lines_[ty][tx];
}

const char* EditorScene::tileAt(int tx, int ty) const {
    if (tx < 0 || tx >= width_ || ty < 0 || ty >= height_) return nullptr;
    if (ty >= static_cast<int>(lines_.size())) return nullptr;
    if (tx >= static_cast<int>(lines_[ty].size())) return nullptr;
    return &lines_[ty][tx];
}

void EditorScene::paintCell(int tx, int ty, char newChar) {
    char* p = tileAt(tx, ty);
    if (!p) return;
    if (*p != newChar) {
        *p = newChar;
        geometryDirty_ = true;
        reachDirty_ = true;
        levelIconsDirty_ = true;
    }
}

void EditorScene::paintLine(sf::Vector2i from, sf::Vector2i to, char newChar) {
    int x0 = from.x, y0 = from.y;
    int x1 = to.x,   y1 = to.y;
    int dx =  std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        paintCell(x0, y0, newChar);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void EditorScene::paintRect(sf::Vector2i a, sf::Vector2i b, char newChar) {
    int x0 = std::min(a.x, b.x), x1 = std::max(a.x, b.x);
    int y0 = std::min(a.y, b.y), y1 = std::max(a.y, b.y);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            paintCell(x, y, newChar);
}

void EditorScene::enforceUniquePlayer(int keepX, int keepY) {
    bool kept = (keepX >= 0 && keepY >= 0);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            char* c = tileAt(x, y);
            if (!c || *c != 'P') continue;
            if (keepX >= 0 && keepY >= 0) {
                if (x == keepX && y == keepY) continue;
                *c = ' ';
                geometryDirty_ = true;
            } else {
                if (!kept) { kept = true; }
                else { *c = ' '; geometryDirty_ = true; }
            }
        }
    }
}

void EditorScene::paintAt(sf::Vector2i tile) {
    char newChar = rightDown_ ? ' ' : brush_;
    paintCell(tile.x, tile.y, newChar);
    if (newChar == 'P') enforceUniquePlayer(tile.x, tile.y);
}

// ============================================================
// 几何重建
// ============================================================

void EditorScene::rebuildGeometry() {
    if (!geometryDirty_) return;

    const float ts = static_cast<float>(tileSize_);

    tileVA_.clear();
    tileVA_.setPrimitiveType(sf::PrimitiveType::Triangles);

    const sf::Color kBody (70, 70, 90);
    const sf::Color kTop  (120, 120, 145);

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const char* c = tileAt(x, y);
            if (!c || *c != '#') continue;

            float px = static_cast<float>(x) * ts;
            float py = static_cast<float>(y) * ts;

            tileVA_.append(sf::Vertex{{px, py}, kBody});
            tileVA_.append(sf::Vertex{{px + ts, py}, kBody});
            tileVA_.append(sf::Vertex{{px + ts, py + ts}, kBody});

            tileVA_.append(sf::Vertex{{px, py}, kBody});
            tileVA_.append(sf::Vertex{{px + ts, py + ts}, kBody});
            tileVA_.append(sf::Vertex{{px, py + ts}, kBody});

            const char* above = tileAt(x, y - 1);
            if (!above || *above != '#') {
                tileVA_.append(sf::Vertex{{px, py}, kTop});
                tileVA_.append(sf::Vertex{{px + ts, py}, kTop});
                tileVA_.append(sf::Vertex{{px + ts, py + 5.f}, kTop});

                tileVA_.append(sf::Vertex{{px, py}, kTop});
                tileVA_.append(sf::Vertex{{px + ts, py + 5.f}, kTop});
                tileVA_.append(sf::Vertex{{px, py + 5.f}, kTop});
            }
        }
    }

    geometryDirty_ = false;
}

// ============================================================
// 视图 / 坐标转换（支持 zoom）
// ============================================================

sf::View EditorScene::buildWorldView(sf::Vector2u winSize) const {
    float w = static_cast<float>(winSize.x);
    float h = static_cast<float>(winSize.y);
    float scale = std::min(w / kLogicalW, h / kLogicalH);

    float vpW = kLogicalW * scale / w;
    float vpH = kLogicalH * scale / h;
    float vpX = (1.f - vpW) * 0.5f;
    float vpY = (1.f - vpH) * 0.5f;

    // 视野尺寸随 zoom 缩小（zoom>1 放大）
    float viewW = kLogicalW / zoom_;
    float viewH = kLogicalH / zoom_;

    sf::View v(sf::FloatRect(camera_, {viewW, viewH}));
    v.setViewport(sf::FloatRect({vpX, vpY}, {vpW, vpH}));
    return v;
}

sf::Vector2i EditorScene::screenToTile(sf::Vector2i pixel, sf::Vector2u winSize) const {
    float w = static_cast<float>(winSize.x);
    float h = static_cast<float>(winSize.y);
    float scale = std::min(w / kLogicalW, h / kLogicalH);

    float vpPixW = kLogicalW * scale;
    float vpPixH = kLogicalH * scale;
    float vpPixX = (w - vpPixW) * 0.5f;
    float vpPixY = (h - vpPixH) * 0.5f;

    // 像素偏移 / (zoom * scale) = 逻辑单位偏移
    float sx = (static_cast<float>(pixel.x) - vpPixX) / (zoom_ * scale);
    float sy = (static_cast<float>(pixel.y) - vpPixY) / (zoom_ * scale);

    float wx = camera_.x + sx;
    float wy = camera_.y + sy;

    int tx = static_cast<int>(std::floor(wx / tileSize_));
    int ty = static_cast<int>(std::floor(wy / tileSize_));
    return {tx, ty};
}

// ============================================================
// 笔刷面板
// ============================================================

sf::FloatRect EditorScene::brushButtonRect(int idx, sf::Vector2u winSize) const {
    const float winW = static_cast<float>(winSize.x);
    const float winH = static_cast<float>(winSize.y);
    const float totalW = kBrushCount * kButtonW + (kBrushCount - 1) * kButtonGap;
    const float startX = (winW - totalW) * 0.5f;
    const float barY = winH - kBrushBarHeight;
    const float btnY = barY + (kBrushBarHeight - kButtonH) * 0.5f;
    return sf::FloatRect(
        {startX + idx * (kButtonW + kButtonGap), btnY},
        {kButtonW, kButtonH});
}

int EditorScene::hitTestBrushBar(sf::Vector2i pixel, sf::Vector2u winSize) const {
    for (int i = 0; i < kBrushCount; ++i) {
        if (brushButtonRect(i, winSize).contains(
                {static_cast<float>(pixel.x), static_cast<float>(pixel.y)})) {
            return i;
        }
    }
    return -1;
}

// ============================================================
// 事件
// ============================================================

void EditorScene::handleEvent(const sf::Event& event) {
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) {
            nextScene_ = SceneId::Back;
            return;
        }
        if (kp->code == sf::Keyboard::Key::S && kp->control) {
            saveFile();
            return;
        }
        if (kp->code == sf::Keyboard::Key::Z && kp->control) {
            undo();
            return;
        }
        if (kp->code == sf::Keyboard::Key::E && kp->control) {
            exportShareCode();
            return;
        }
        if (kp->code == sf::Keyboard::Key::I && kp->control) {
            importShareCode();
            return;
        }

        // 网格开关
        if (kp->code == sf::Keyboard::Key::G && !kp->control) {
            showGrid_ = !showGrid_;
            showFlash(showGrid_ ? Str::T(Str::EditorGridOn)
                                : Str::T(Str::EditorGridOff), 0.8f);
            return;
        }

        // ⭐ 可达性可视化开关
        if (kp->code == sf::Keyboard::Key::T && !kp->control) {
            showReachability_ = !showReachability_;
            reachDirty_ = true;
            showFlash(showReachability_ ? Str::T(Str::EditorReachOn)
                                        : Str::T(Str::EditorReachOff), 0.8f);
            return;
        }

        // 缩放：0 键重置为 100%
        if (kp->code == sf::Keyboard::Key::Num0 &&
            (kp->control || kp->system)) {
            zoom_ = 1.0f;
            showFlash(Str::T(Str::EditorZoomReset), 0.8f);
            return;
        }

        if (kp->code == sf::Keyboard::Key::LBracket) {
            resizeLevel(width_ - 4, height_);
            return;
        }
        if (kp->code == sf::Keyboard::Key::RBracket) {
            resizeLevel(width_ + 4, height_);
            return;
        }
        if (kp->code == sf::Keyboard::Key::Hyphen) {
            resizeLevel(width_, height_ - 2);
            return;
        }
        if (kp->code == sf::Keyboard::Key::Equal) {
            resizeLevel(width_, height_ + 2);
            return;
        }

        if (kp->code == sf::Keyboard::Key::N && kp->control) {
            switchToNextFile();
            return;
        }
    }

    auto shiftDown = []() {
        return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
            || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    };

    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            int idx = hitTestBrushBar(mb->position, lastWinSize_);
            if (idx >= 0) {
                brush_ = kBrushes[idx].ch;
                return;
            }
        }

        if (mb->button == sf::Mouse::Button::Left) {
            pushUndo();
            leftDown_ = true;
            auto t = screenToTile(mb->position, lastWinSize_);
            dragStartTile_ = t;
            dragLastTile_  = t;
            // Shift 按下：矩形模式，松手时才填充
            if (!shiftDown()) {
                paintAt(t);
            }
        }
        if (mb->button == sf::Mouse::Button::Right) {
            if (hitTestBrushBar(mb->position, lastWinSize_) >= 0) return;
            pushUndo();
            rightDown_ = true;
            auto t = screenToTile(mb->position, lastWinSize_);
            dragStartTile_ = t;
            dragLastTile_  = t;
            paintAt(t);
        }
        if (mb->button == sf::Mouse::Button::Middle) {
            middleDown_ = true;
            middleAnchor_ = {static_cast<float>(mb->position.x),
                             static_cast<float>(mb->position.y)};
        }
    }

    if (const auto* mb = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            if (leftDown_ && shiftDown()
                && dragStartTile_.x >= 0) {
                auto t = screenToTile(mb->position, lastWinSize_);
                paintRect(dragStartTile_, t, brush_);
                if (brush_ == 'P') enforceUniquePlayer(-1, -1);
            }
            leftDown_ = false;
            dragStartTile_ = {-1, -1};
            dragLastTile_  = {-1, -1};
        }
        if (mb->button == sf::Mouse::Button::Right) {
            rightDown_ = false;
            dragStartTile_ = {-1, -1};
            dragLastTile_  = {-1, -1};
        }
        if (mb->button == sf::Mouse::Button::Middle) middleDown_ = false;
    }

    if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        if (hitTestBrushBar(mm->position, lastWinSize_) >= 0) return;
        auto t = screenToTile(mm->position, lastWinSize_);

        if (leftDown_) {
            if (!shiftDown()) {
                paintLine(dragLastTile_, t, brush_);
                if (brush_ == 'P') enforceUniquePlayer(t.x, t.y);
                dragLastTile_ = t;
            } else {
                // 矩形模式：只更新预览终点
                dragLastTile_ = t;
            }
        }
        if (rightDown_) {
            paintLine(dragLastTile_, t, ' ');
            dragLastTile_ = t;
        }
    }

    if (const auto* ws = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (ws->wheel != sf::Mouse::Wheel::Vertical) return;

        // Ctrl + 滚轮 = 缩放
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl)) {

            sf::Vector2i mp = ws->position;

            float w = static_cast<float>(lastWinSize_.x);
            float h = static_cast<float>(lastWinSize_.y);
            float scale = std::min(w / kLogicalW, h / kLogicalH);

            float vpPixW = kLogicalW * scale;
            float vpPixH = kLogicalH * scale;
            float vpPixX = (w - vpPixW) * 0.5f;
            float vpPixY = (h - vpPixH) * 0.5f;

            float dx = static_cast<float>(mp.x) - vpPixX;
            float dy = static_cast<float>(mp.y) - vpPixY;

            // 鼠标下的世界坐标（缩放前）
            float wx = camera_.x + dx / (zoom_ * scale);
            float wy = camera_.y + dy / (zoom_ * scale);

            float factor = (ws->delta > 0.f) ? 1.15f : (1.f / 1.15f);
            zoom_ = std::clamp(zoom_ * factor, 0.25f, 5.0f);

            // 让鼠标下的世界坐标保持不动
            camera_.x = wx - dx / (zoom_ * scale);
            camera_.y = wy - dy / (zoom_ * scale);
        } else {
            // 普通滚轮：垂直移动摄像机
            float step = 3.f * tileSize_ / zoom_;
            camera_.y -= ws->delta * step;
        }
    }
}

// ============================================================
// 更新
// ============================================================

void EditorScene::update(float dt) {
    if (flashTimer_ > 0.f) flashTimer_ -= dt;

    const float speed = 400.f * dt / zoom_;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        camera_.x -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        camera_.x += speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        camera_.y -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))
        camera_.y += speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        camera_.y += speed;

    // clamp 考虑 zoom（视野大小）
    float viewW = kLogicalW / zoom_;
    float viewH = kLogicalH / zoom_;
    float maxX = static_cast<float>(width_  * tileSize_) - viewW * 0.5f;
    float maxY = static_cast<float>(height_ * tileSize_) - viewH * 0.5f;
    camera_.x = std::clamp(camera_.x, -viewW * 0.3f, std::max(-viewW * 0.3f, maxX));
    camera_.y = std::clamp(camera_.y, -viewH * 0.3f, std::max(-viewH * 0.3f, maxY));
}

// ============================================================
// 渲染
// ============================================================

void EditorScene::render(Window& window) {
    auto& rt = window.target();
    auto winSize = window.native().getSize();
    lastWinSize_ = winSize;

    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    sf::View screenView(sf::FloatRect({0.f, 0.f}, {winW, winH}));
    rt.setView(screenView);
    rt.clear(sf::Color(30, 30, 40));
    if (background_) background_->render(rt);

    // ===== 世界层 =====
    sf::View worldView = buildWorldView(winSize);
    rt.setView(worldView);

    // 网格（可关）
    if (showGrid_) {
        const float ts = static_cast<float>(tileSize_);
        sf::VertexArray grid;
        grid.setPrimitiveType(sf::PrimitiveType::Lines);
        const sf::Color kLine(60, 60, 80);

        for (int x = 0; x <= width_; ++x) {
            float px = static_cast<float>(x) * ts;
            grid.append(sf::Vertex{{px, 0.f}, kLine});
            grid.append(sf::Vertex{{px, static_cast<float>(height_) * ts}, kLine});
        }
        for (int y = 0; y <= height_; ++y) {
            float py = static_cast<float>(y) * ts;
            grid.append(sf::Vertex{{0.f, py}, kLine});
            grid.append(sf::Vertex{{static_cast<float>(width_) * ts, py}, kLine});
        }
        rt.draw(grid);
    }

    rebuildGeometry();
    if (tileVA_.getVertexCount() > 0)
        rt.draw(tileVA_);

    // ⭐ 可达性可视化叠加
    if (showReachability_) {
        if (reachDirty_) {
            Level tmp;
            std::string text;
            for (const auto& l : lines_) { text += l; text += '\n'; }
            tmp.loadFromString(text);
            reachReport_ = LevelValidator::validate(tmp);
            reachDirty_ = false;
        }

        const float tsF = static_cast<float>(tileSize_);
        sf::RectangleShape hl({tsF, tsF});

        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                sf::Color color = sf::Color::Transparent;
                bool has = false;

                // 平台顶面
                if (reachReport_.allPlatformTops.count({x, y})) {
                    color = reachReport_.reachablePlatformTops.count({x, y})
                        ? sf::Color(80, 220, 100, 100)    // 可达：淡绿
                        : sf::Color(230, 70, 70, 100);    // 不可达：淡红
                    has = true;
                }

                // spawn 元素
                if (reachReport_.reachableSpawns.count({x, y})) {
                    color = sf::Color(60, 240, 120, 150); // 可达 spawn：亮绿
                    has = true;
                }
                for (const auto& u : reachReport_.unreachable) {
                    if (u.tileX == x && u.tileY == y) {
                        color = sf::Color(255, 50, 50, 170); // 不可达 spawn：亮红
                        has = true;
                        break;
                    }
                }

                // 出生点
                if (reachReport_.spawnTile.first == x
                    && reachReport_.spawnTile.second == y) {
                    color = sf::Color(80, 160, 255, 180);
                    has = true;
                }

                if (has) {
                    hl.setPosition({x * tsF, y * tsF});
                    hl.setFillColor(color);
                    rt.draw(hl);
                }
            }
        }
    }

    // ⭐ Shift 矩形拖拽预览
    if (leftDown_ && dragStartTile_.x >= 0) {
        bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)
                  || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
        if (shift) {
            const float tsF = static_cast<float>(tileSize_);
            int x0 = std::min(dragStartTile_.x, dragLastTile_.x);
            int x1 = std::max(dragStartTile_.x, dragLastTile_.x);
            int y0 = std::min(dragStartTile_.y, dragLastTile_.y);
            int y1 = std::max(dragStartTile_.y, dragLastTile_.y);

            sf::RectangleShape rect({
                (x1 - x0 + 1) * tsF,
                (y1 - y0 + 1) * tsF
            });
            rect.setPosition({x0 * tsF, y0 * tsF});
            rect.setFillColor(sf::Color(255, 240, 120, 80));
            rect.setOutlineThickness(2.f / zoom_);
            rect.setOutlineColor(sf::Color(255, 240, 120, 220));
            rt.draw(rect);
        }
    }

    // 统计（纯循环，便宜）
    int statPlayer = 0, statEnemy = 0, statCoin = 0, statGoal = 0;
    int statSpike = 0, statJump = 0, statCheckpoint = 0;
    int statKey = 0, statDoor = 0, statPlatform = 0;

    const float ts = static_cast<float>(tileSize_);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const char* c = tileAt(x, y);
            if (!c || *c == ' ' || *c == '#') continue;
            switch (*c) {
                case 'P': ++statPlayer; break;
                case 'E': ++statEnemy; break;
                case 'C': ++statCoin; break;
                case 'G': ++statGoal; break;
                case '^': ++statSpike; break;
                case 'J': ++statJump; break;
                case 'S': ++statCheckpoint; break;
                case 'K': ++statKey; break;
                case 'L': ++statDoor; break;
                case 'M': case 'V': ++statPlatform; break;
                default: break;
            }
        }
    }

    // ⭐ 特殊元素图标预渲染
    if (levelIconsRTAvailable_) {
        unsigned rtw = static_cast<unsigned>(width_ * tileSize_);
        unsigned rth = static_cast<unsigned>(height_ * tileSize_);
        if (rtw == 0 || rth == 0 || rtw > 4096 || rth > 4096) {
            levelIconsRTAvailable_ = false;
        } else if (levelIconsDirty_
                   || rtw != levelIconsRTW_
                   || rth != levelIconsRTH_) {
            if (levelIconsRT_.resize({rtw, rth})) {
                levelIconsRT_.clear(sf::Color::Transparent);
                for (int y = 0; y < height_; ++y) {
                    for (int x = 0; x < width_; ++x) {
                        const char* c = tileAt(x, y);
                        if (!c || *c == ' ' || *c == '#') continue;
                        drawTileIcon(levelIconsRT_, *c,
                                     static_cast<float>(x) * ts,
                                     static_cast<float>(y) * ts,
                                     ts);
                    }
                }
                levelIconsRT_.display();
                levelIconsDirty_ = false;
                levelIconsRTW_ = rtw;
                levelIconsRTH_ = rth;
            } else {
                levelIconsRTAvailable_ = false;
            }
        }
    }

    if (levelIconsRTAvailable_) {
        sf::Sprite s(levelIconsRT_.getTexture());
        rt.draw(s);
    } else {
        // 大关卡退回到每帧绘制
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const char* c = tileAt(x, y);
                if (!c || *c == ' ' || *c == '#') continue;
                drawTileIcon(rt, *c,
                             static_cast<float>(x) * ts,
                             static_cast<float>(y) * ts,
                             ts);
            }
        }
    }

    {
        sf::RectangleShape border({static_cast<float>(width_) * ts,
                                   static_cast<float>(height_) * ts});
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineThickness(2.f);
        border.setOutlineColor(sf::Color(120, 120, 160));
        rt.draw(border);
    }

    // 鼠标悬停高亮
    {
        auto mp = sf::Mouse::getPosition(window.native());
        if (hitTestBrushBar(mp, winSize) < 0) {
            auto t = screenToTile(mp, winSize);
            if (t.x >= 0 && t.x < width_ && t.y >= 0 && t.y < height_) {
                sf::RectangleShape hl({ts, ts});
                hl.setPosition({static_cast<float>(t.x) * ts,
                                static_cast<float>(t.y) * ts});
                hl.setFillColor(sf::Color(255, 255, 255, 40));
                hl.setOutlineThickness(2.f / zoom_);
                hl.setOutlineColor(sf::Color(255, 240, 120, 220));
                rt.draw(hl);
            }
        }
    }

    // ===== HUD 层 =====
    rt.setView(screenView);

    sf::RectangleShape topBar({winW, 84.f});
    topBar.setFillColor(sf::Color(0, 0, 0, 160));
    rt.draw(topBar);

    const char* brushName = "?";
    for (int i = 0; i < kBrushCount; ++i) {
        if (kBrushes[i].ch == brush_) { brushName = kBrushes[i].name; break; }
    }

    int zoomPct = static_cast<int>(zoom_ * 100.f + 0.5f);

    std::ostringstream h1;
    h1 << Str::T(Str::EditorHudFile)  << currentFileName()
       << "    " << Str::T(Str::EditorHudBrush) << Str::T(brushName)
       << "    " << Str::T(Str::EditorHudSize)  << width_ << " x " << height_
       << "    " << Str::T(Str::EditorHudZoom)  << zoomPct << "%"
       << "    " << Str::T(Str::EditorHudGrid)  << (showGrid_ ? Str::T(Str::On) : Str::T(Str::Off))
       << "    " << Str::T(Str::EditorHudUndo)  << undoStack_.size()
       << "    " << (showReachability_ ? Str::T(Str::EditorReachOn)
                                       : Str::T(Str::EditorReachOff));
    hudText_.setString(toSf(h1.str()));
    hudText_.setPosition({20.f, 10.f});
    rt.draw(hudText_);

    // 元素统计（第二行）
    std::ostringstream h2;
    h2 << Str::T(Str::BrushPlayer)     << " " << statPlayer
       << " | " << Str::T(Str::BrushEnemy)      << " " << statEnemy
       << " | " << Str::T(Str::BrushCoin)       << " " << statCoin
       << " | " << Str::T(Str::BrushGoal)       << " " << statGoal
       << " | " << Str::T(Str::BrushSpike)      << " " << statSpike
       << " | " << Str::T(Str::BrushJumpPad)    << " " << statJump
       << " | " << Str::T(Str::BrushCheckpoint) << " " << statCheckpoint
       << " | " << Str::T(Str::BrushKey)        << " " << statKey
       << " | " << Str::T(Str::BrushDoor)       << " " << statDoor
       << " | " << Str::T(Str::StatPlatform)    << " " << statPlatform;
    hudText_.setString(toSf(h2.str()));
    hudText_.setPosition({20.f, 46.f});
    rt.draw(hudText_);

    // 底部提示条
    sf::RectangleShape hintBar({winW, 26.f});
    hintBar.setPosition({0.f, winH - kBrushBarHeight - 26.f});
    hintBar.setFillColor(sf::Color(0, 0, 0, 140));
    rt.draw(hintBar);

    hintText_.setString(toSf(Str::T(Str::EditorHint)));
    hintText_.setPosition({20.f, winH - kBrushBarHeight - 22.f});
    rt.draw(hintText_);

    // ⭐ 笔刷条预渲染
    sf::Vector2i mousePos = sf::Mouse::getPosition(window.native());
    int hoverIdx = hitTestBrushBar(mousePos, winSize);

    if (brushBarDirty_
        || hoverIdx != lastBrushBarHover_
        || brush_ != lastBrushBarSelected_
        || static_cast<unsigned>(winW) != lastBrushBarWinW_) {
        unsigned bw = static_cast<unsigned>(winW);
        unsigned bh = static_cast<unsigned>(kBrushBarHeight);
        if (brushBarRT_.resize({bw, bh})) {
            brushBarRT_.clear(sf::Color::Transparent);
            const float barY = winH - kBrushBarHeight;

            sf::RectangleShape bgShape({winW, kBrushBarHeight});
            bgShape.setPosition({0.f, 0.f});
            bgShape.setFillColor(sf::Color(15, 15, 25, 220));
            bgShape.setOutlineThickness(2.f);
            bgShape.setOutlineColor(sf::Color(60, 60, 90));
            brushBarRT_.draw(bgShape);

            for (int i = 0; i < kBrushCount; ++i) {
                const auto& b = kBrushes[i];
                sf::FloatRect r = brushButtonRect(i, winSize);
                sf::FloatRect localR(
                    {r.position.x, r.position.y - barY},
                    {r.size.x, r.size.y});

                const bool selected = (b.ch == brush_);
                const bool hovered  = (i == hoverIdx);

                sf::RectangleShape btn({localR.size.x, localR.size.y});
                btn.setPosition({localR.position.x, localR.position.y});
                sf::Color bg = b.color;
                bg.a = selected ? 255 : (hovered ? 220 : 160);
                btn.setFillColor(bg);
                btn.setOutlineThickness(selected ? 3.f : (hovered ? 2.f : 1.f));
                btn.setOutlineColor(selected ? sf::Color(255, 255, 255)
                                             : sf::Color(0, 0, 0, 150));
                brushBarRT_.draw(btn);

                {
                    float iconSize = localR.size.y * 0.5f;
                    float iconX = localR.position.x + (localR.size.x - iconSize) * 0.5f;
                    float iconY = localR.position.y + 2.f;
                    drawTileIcon(brushBarRT_, b.ch, iconX, iconY, iconSize);
                }

                sf::Text nameText(*font_, toSf(Str::T(b.name)), 14);
                nameText.setFillColor(sf::Color(255, 255, 255));
                nameText.setOutlineThickness(2.f);
                nameText.setOutlineColor(sf::Color(0, 0, 0, 220));
                auto nb = nameText.getLocalBounds();
                nameText.setOrigin({nb.position.x + nb.size.x / 2.f,
                                    nb.position.y + nb.size.y / 2.f});
                nameText.setPosition({localR.position.x + localR.size.x / 2.f,
                                      localR.position.y + localR.size.y - 12.f});
                brushBarRT_.draw(nameText);
            }
            brushBarRT_.display();
            lastBrushBarHover_    = hoverIdx;
            lastBrushBarSelected_ = brush_;
            lastBrushBarWinW_     = static_cast<unsigned>(winW);
            brushBarDirty_ = false;
        }
    }

    {
        sf::Sprite bs(brushBarRT_.getTexture());
        bs.setPosition({0.f, winH - kBrushBarHeight});
        rt.draw(bs);
    }

    // Flash 提示
    if (flashTimer_ > 0.f) {
        flashDraw_.setString(toSf(flashText_));
        auto b = flashDraw_.getLocalBounds();
        flashDraw_.setOrigin({b.position.x + b.size.x / 2.f,
                              b.position.y + b.size.y / 2.f});
        flashDraw_.setPosition({winW / 2.f, winH / 2.f - 100.f});
        rt.draw(flashDraw_);
    }
}