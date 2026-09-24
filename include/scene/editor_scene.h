#pragma once
#include "scene.h"
#include "background.h"
#include "preferences.h"
#include "logging.h"
#include "level_validator.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

class EditorScene : public Scene {
public:
    EditorScene(std::shared_ptr<Background>  background,
                std::shared_ptr<Preferences> preferences,
                const sf::Font&              font,
                std::shared_ptr<Logger>      logger);

    void onEnter() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }

private:
    void loadFile();
    void saveFile();
    void exportShareCode();
    void importShareCode();

    void pushUndo();
    void undo();

    void resizeLevel(int newW, int newH);
    void refreshDimensions();

    void scanLevelFiles();
    void switchToNextFile();
    std::string currentFileName() const;

    char* tileAt(int tx, int ty);
    const char* tileAt(int tx, int ty) const;

    void paintAt(sf::Vector2i tile);
    void paintCell(int tx, int ty, char newChar);
    void paintLine(sf::Vector2i from, sf::Vector2i to, char newChar);
    void paintRect(sf::Vector2i a, sf::Vector2i b, char newChar);
    void enforceUniquePlayer(int keepX, int keepY);
    void rebuildGeometry();

    sf::View buildWorldView(sf::Vector2u winSize) const;
    sf::Vector2i screenToTile(sf::Vector2i pixel, sf::Vector2u winSize) const;

    sf::FloatRect brushButtonRect(int idx, sf::Vector2u winSize) const;
    int hitTestBrushBar(sf::Vector2i pixel, sf::Vector2u winSize) const;

    void showFlash(const std::string& text, float duration = 1.5f);

    std::shared_ptr<Background>  background_;
    std::shared_ptr<Preferences> preferences_;
    std::shared_ptr<Logger>      logger_;
    const sf::Font*              font_ = nullptr;

    std::vector<std::string> lines_;
    int width_    = 40;
    int height_   = 22;
    int tileSize_ = 32;

    char brush_ = '#';

    sf::Vector2f camera_{0.f, 0.f};

    // 缩放（1.0 = 1x，2.0 = 2 倍大小）
    float zoom_ = 1.0f;
    bool  showGrid_ = true;

    // ⭐ 可达性可视化
    bool showReachability_ = false;
    ValidationReport reachReport_;
    bool reachDirty_ = true;

    bool leftDown_   = false;
    bool rightDown_  = false;
    bool middleDown_ = false;
    sf::Vector2f middleAnchor_{0.f, 0.f};

    // ⭐ 拖拽绘制
    sf::Vector2i dragStartTile_{-1, -1};
    sf::Vector2i dragLastTile_{-1, -1};

    sf::VertexArray tileVA_;
    bool geometryDirty_ = true;

    // ⭐ 性能：特殊元素图标预渲染
    sf::RenderTexture levelIconsRT_;
    bool levelIconsDirty_ = true;
    bool levelIconsRTAvailable_ = true;
    unsigned levelIconsRTW_ = 0;
    unsigned levelIconsRTH_ = 0;

    // ⭐ 性能：笔刷条预渲染
    sf::RenderTexture brushBarRT_;
    bool brushBarDirty_ = true;
    int  lastBrushBarHover_ = -2;
    char lastBrushBarSelected_ = '\0';
    unsigned lastBrushBarWinW_ = 0;

    std::vector<std::vector<std::string>> undoStack_;
    static constexpr std::size_t kMaxUndo = 100;

    std::vector<std::string> levelFiles_;
    int currentFileIdx_ = 0;

    std::string flashText_;
    float flashTimer_ = 0.f;

    sf::Text hudText_;
    sf::Text hintText_;
    sf::Text flashDraw_;
    sf::Text brushNameText_;   // ⭐ 复用，不每帧构造/析构

    std::string savePath_;

    sf::Vector2u lastWinSize_{1280, 720};

    SceneId nextScene_ = SceneId::None;

    static constexpr float kLogicalW = 1280.f;
    static constexpr float kLogicalH = 720.f;
    static constexpr float kBrushBarHeight = 76.f;
};