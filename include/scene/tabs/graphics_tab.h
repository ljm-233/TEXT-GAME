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
    GraphicsTab(const sf::Font& font,
                std::shared_ptr<Preferences> prefs,
                std::shared_ptr<Window> window);

    void loadFromPrefs();
    void handleEvent(const sf::Event& ev);
    void update();

    float render(sf::RenderTarget& target,
                 float contentX, float ctrlX,
                 float startY);

    void refreshLabels();
    void refreshSelection();
    void registerFocus(std::vector<Button*>& out);
    bool anyEditing() const;

    // 一键重置所有后处理
    void resetPost();

private:
    const sf::Font& font_;
    std::shared_ptr<Preferences> prefs_;
    std::shared_ptr<Window>      window_;

    // ===== 状态 =====
    bool  animationEnabled_      = true;
    int   animationSpeedIndex_   = 1;
    bool  pseudo3D_              = true;
    bool  parallaxEnabled_       = true;
    bool  playerAnimEnabled_     = true;
    bool  levelIntroEnabled_     = true;
    bool  particlesEnabled_      = true;
    bool  screenShake_           = true;
    bool  notificationEnabled_   = true;
    bool  showColliders_         = false;
    int   initialLives_          = 1;
    int   notificationPosition_  = 1;
    float buttonCorner_          = 6.f;
    float buttonOutline_         = 2.f;

    // ===== 后处理状态 =====
    float postSaturation_      = 1.0f;
    float postContrast_        = 1.0f;
    float postBrightness_      = 1.0f;
    float postGamma_           = 1.0f;
    float postVignette_        = 0.0f;
    float postBloomStrength_   = 0.0f;
    float postBloomThreshold_  = 0.7f;
    float postChromatic_       = 0.0f;
    float postGrain_           = 0.0f;
    float postScanline_        = 0.0f;
    float postDither_          = 0.0f;

    // ===== 控件 =====
    std::vector<std::unique_ptr<ToggleRow>> toggles_;
    std::vector<std::unique_ptr<MultiRow>>  multiRows_;
    std::unique_ptr<MultiRow>               presetRow_;

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
    void applyAnimation();
    void applyPseudo3D();
    void applyParallax();
    void applyPlayerAnimation();
    void applyLevelIntro();
    void applyParticles();
    void applyScreenShake();
    void applyNotification();
    void applyShowColliders();
    void applyButtonStyle();
    void applyPreset(int idx);
};