#pragma once
#include "background.h"
#include "button.h"
#include "save_manager.h"
#include "scene.h"
#include "stats.h"
#include <memory>
#include <vector>

//
// 成绩 / 统计页。
//
// 数据全部来自 Stats::summarize()（纯函数，有单测）—— 这里只负责排版，
// 一个字都不自己算。每关一行（关卡 / 星级 / 最佳时间 / 最佳金币）+ 总计 + 进度条。
//
// ⚠️ 关卡数不写死：一屏放不下就按可用高度分栏（栏数在 render 里按窗口高度算，
// 不存状态）。每行的 sf::Text 是**成员**，按关数只增不减地扩容 ——
// 绝不在 render 里构造 sf::Text（SFML 3.1 析构会死锁）。
//
class StatsScene : public Scene {
public:
    StatsScene(std::shared_ptr<Background> background,
               std::shared_ptr<SaveManager> saveManager, const sf::Font& font,
               std::shared_ptr<Logger> logger);

    void onEnter() override;
    void onResume() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(Window& window) override;

    SceneId nextScene() const override { return nextScene_; }
    std::string windowTitleHint() const override;

private:
    void reload();          // 读存档列表 + 选中项 + 统计 + 文字
    void refreshLabels();   // 只重设文字（语言切换 / 切换存档）
    void ensureRowTexts();  // 按关数补足每行的 Text（只增不减）
    void selectSave(int i); // 上一个 / 下一个存档（环绕）
    void syncFocus();

    std::shared_ptr<Background> background_;
    std::shared_ptr<SaveManager> saveManager_;
    std::shared_ptr<Logger> logger_;
    const sf::Font& font_;

    std::vector<SaveInfo> saves_; // listSaves() 已按 lastPlayed 降序
    int index_ = -1;              // saves_ 下标；-1 = 一个存档都没有
    Stats::Summary summary_;

    std::unique_ptr<Button> prevSaveButton_;
    std::unique_ptr<Button> nextSaveButton_;
    std::unique_ptr<Button> backButton_;

    sf::Text titleText_;
    sf::Text saveText_;     // "当前存档: 名字  (1/3)"
    sf::Text emptyText_;    // 一个存档都没有
    sf::Text noRecordText_; // 全新存档：还没有记录
    sf::Text summaryText_;  // 多行总计
    sf::Text progressText_; // "进度：33%"

    // 表头
    sf::Text colLevelText_;
    sf::Text colStarsText_;
    sf::Text colTimeText_;
    sf::Text colCoinsText_;

    // 每关一组的四列文字（按需扩容，常驻成员）
    std::vector<std::unique_ptr<sf::Text>> levelTexts_;
    std::vector<std::unique_ptr<sf::Text>> starTexts_;
    std::vector<std::unique_ptr<sf::Text>> timeTexts_;
    std::vector<std::unique_ptr<sf::Text>> coinTexts_;

    sf::RectangleShape panel_;
    sf::RectangleShape progressBg_;
    sf::RectangleShape progressFill_;

    SceneId nextScene_ = SceneId::None;
    int lastLangVersion_ = -1;
};
