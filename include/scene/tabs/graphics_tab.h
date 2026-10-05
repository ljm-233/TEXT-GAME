#pragma once
#include "preferences.h"
#include "window.h"
#include "slider.h"
#include "toggle_row.h"
#include "multi_row.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class GraphicsTab {
public:
    GraphicsTab(const sf::Font& font, std::shared_ptr<Preferences> prefs,
                std::shared_ptr<Window> window);

    void loadFromPrefs();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target, float contentX, float ctrlX, float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const;

    // 一键重置所有后处理
    void resetPost();

    /// 「恢复本页默认」用：重读配置，并把需要立即生效的东西重新应用一次。
    /// 与 loadFromPrefs() 的区别是它**会**去改全局状态（主题、音量、后处理器…）。
    void reapply();

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<Window> window_;

    // ===== 状态 =====
    bool pseudo3D_ = true;
    bool parallaxEnabled_ = true;
    bool playerAnimEnabled_ = true;
    bool particlesEnabled_ = true;
    bool screenShake_ = true;
    float renderScale_ = 1.0f;
    int upscaleMode_ = 1;
    int particleDensity_ = 2;
    float shakeIntensity_ = 1.0f;

    // ===== 后处理状态 =====
    float postSaturation_ = 1.0f;
    float postContrast_ = 1.0f;
    float postBrightness_ = 1.0f;
    float postGamma_ = 1.0f;
    float postVignette_ = 0.0f;
    float postBloomStrength_ = 0.0f;
    float postBloomThreshold_ = 0.7f;
    float postChromatic_ = 0.0f;
    float postGrain_ = 0.0f;
    float postScanline_ = 0.0f;
    float postDither_ = 0.0f;

    // ===== 控件 =====
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>> multiRows_;
    std::unique_ptr<MultiRow> presetRow_;
    std::unique_ptr<Slider> shakeIntensitySlider_;

    // ⭐ 具名行指针。addToggle / addMultiRow 返回的是行对象在**堆上**的地址：
    // 行本身由 toggles_ / multiRows_ 里的 unique_ptr 持有，vector 扩容搬的是
    // unique_ptr 这个指针本身，指向的对象不会动，所以在这里缓存裸指针是安全的。
    ToggleRow* rowPseudo3D_ = nullptr;
    ToggleRow* rowParallax_ = nullptr;
    ToggleRow* rowPlayerAnim_ = nullptr;
    ToggleRow* rowParticles_ = nullptr;
    ToggleRow* rowScreenShake_ = nullptr;

    MultiRow* rowRenderScale_ = nullptr;
    MultiRow* rowUpscaleMode_ = nullptr;
    MultiRow* rowParticleDensity_ = nullptr;

    // 后处理 label
    sf::Text labelPostSaturation_;
    sf::Text labelPostContrast_;
    sf::Text labelPostBrightness_;
    sf::Text labelPostGamma_;
    sf::Text labelPostVignette_;
    sf::Text labelPostBloomStrength_;
    sf::Text labelPostBloomThreshold_;
    sf::Text labelPostChromatic_;
    sf::Text labelPostGrain_;
    sf::Text labelPostScanline_;
    sf::Text labelPostDither_;
    sf::Text labelShakeIntensity_;

    // 后处理 slider
    std::unique_ptr<Slider> saturationSlider_;
    std::unique_ptr<Slider> contrastSlider_;
    std::unique_ptr<Slider> brightnessSlider_;
    std::unique_ptr<Slider> gammaSlider_;
    std::unique_ptr<Slider> vignetteSlider_;
    std::unique_ptr<Slider> bloomStrengthSlider_;
    std::unique_ptr<Slider> bloomThresholdSlider_;
    std::unique_ptr<Slider> chromaticSlider_;
    std::unique_ptr<Slider> grainSlider_;
    std::unique_ptr<Slider> scanlineSlider_;
    std::unique_ptr<Slider> ditherSlider_;

    // ===== 应用 =====
    void applyPseudo3D();
    void applyParallax();
    void applyPlayerAnimation();
    void applyParticles();
    void applyScreenShake();
    void applyPreset(int idx);
};
