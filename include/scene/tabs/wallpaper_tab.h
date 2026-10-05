#pragma once
#include "background.h"
#include "button.h"
#include "preferences.h"
#include "wallpaper/wallpaper_library.h"
#include "wallpaper/wallpaper_loader.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

namespace wallpaper_tab_layout {

inline constexpr float kThumbW = 160.f;
inline constexpr float kThumbH = 90.f;
inline constexpr float kRowH = 106.f;
inline constexpr float kGap = 20.f;
inline constexpr float kBtnW = 420.f;
inline constexpr float kBtnH = 46.f;
inline constexpr float kHintH = 34.f;

/// 第 index 行那个按钮的左上角。
///
/// 放在头里是**故意的**：冒烟测试要按同样的坐标模拟鼠标点击，而
/// "点击落在哪个按钮上"这件事只能有一个来源 —— 布局改了而测试还按老坐标点，
/// 断言会变成"点空了所以什么也没发生"，看着像通过、其实什么都没验。
inline sf::Vector2f rowButtonPos(float contentX, float startY, int index) {
    return {contentX + kThumbW + kGap,
            startY + kHintH + static_cast<float>(index) * kRowH + (kRowH - kBtnH) * 0.5f};
}

/// 第 index 行缩略图框的左上角
inline sf::Vector2f rowThumbPos(float contentX, float startY, int index) {
    return {contentX, startY + kHintH + static_cast<float>(index) * kRowH};
}

} // namespace wallpaper_tab_layout

/// 设置里的「壁纸」页：把目录里所有壁纸列出来（缩略图 + 文件名），点一下就切。
///
/// 一行一张：
///     [ 缩略图 160x90 ]  [ Button: 文件名 ]
///
/// **为什么用 Button 当行**：点击、悬停、选中态、以及手柄焦点导航全都白拿 ——
/// `FocusGroup` 收的就是 `std::vector<Button*>`，再按按钮位置做几何导航。
/// 缩略图由本类自己画在按钮左边，不参与焦点。
///
/// ⚠️ 缩略图**必须先缩到 256** 再上传纹理（见 wallpaper_thumbnail.h）。
///    直接给全尺寸纹理套 `setScale` 显示小图的话，5 张壁纸要吃 160MB+ 显存，
///    而那些像素一个都不会真的被显示出来。
class WallpaperTab {
public:
    WallpaperTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                 std::shared_ptr<Background> background,
                 std::shared_ptr<WallpaperLibrary> library,
                 std::shared_ptr<WallpaperLoader> loader);

    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target, float contentX, float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const { return false; } // 本页没有文本输入

private:
    /// 把已经解码好的缩略图上传成小纹理。缩略图是后台线程逐个产出的，
    /// 所以这一步要**反复调**，直到全部就位。
    void ensureThumbnails();

    /// 按当前选中项更新按钮的选中态（每帧调，成本就是几次 setSelected）
    void syncSelection();

    /// 统一算行位置。render 与 registerFocus 都要调 —— 焦点导航是按
    /// 按钮位置做几何计算的，而切 Tab 时的 syncFocus 发生在本页第一次
    /// render **之前**，只靠 render 布局会让首次的手柄导航落在错误位置。
    void layoutRows();

    void applySelection(int index);

    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<Background> background_;
    std::shared_ptr<WallpaperLibrary> library_;
    std::shared_ptr<WallpaperLoader> loader_;

    /// 与 library_ 的下标一一对应
    std::vector<std::unique_ptr<Button>> rowButtons_;

    /// 每张壁纸的缩略图纹理。
    ///
    /// ⚠️ 纹理必须用 `unique_ptr` 持有：sprite 内部存的是纹理**地址**，
    ///    而这个 vector 会随着 push_back 重新分配、把元素搬走 ——
    ///    按值持有的话 sprite 立刻指向已搬走的旧地址（Background 里踩过同样的坑）。
    struct Thumb {
        std::unique_ptr<sf::Texture> tex;
        std::unique_ptr<sf::Sprite> sprite;
    };
    std::vector<Thumb> thumbs_;

    /// 最近一次布局用的原点（render 传入并保存，registerFocus 复用）
    float originX_ = 0.f;
    float originY_ = 0.f;
    bool laidOut_ = false;

    // 提示行（含 "当前/总数"）。sf::Text 没有默认构造函数，必须在初始化
    // 列表里用字体构造 —— 而且不能每帧新建（HarfBuzz 析构那条坑）。
    sf::Text hint_;
};
