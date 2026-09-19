#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <string>

class PostProcessor {
public:
    void load(const std::string& shaderDir);
    bool isLoaded() const { return loaded_; }
    bool isActive() const;
    bool bloomActive() const;

    void setSaturation(float v) { saturation_ = v; }
    void setContrast(float v)   { contrast_   = v; }
    void setBrightness(float v) { brightness_ = v; }
    void setGamma(float v)      { gamma_      = v; }
    void setVignette(float v)   { vignette_   = v; }
    void setBloomStrength(float v)  { bloomStrength_ = v; }
    void setBloomThreshold(float v) { bloomThreshold_ = v; }
    void setChromatic(float v)      { chromatic_ = v; }
    void setGrain(float v)          { grain_     = v; }
    void setScanline(float v)       { scanline_  = v; }
    void setDither(float v)         { dither_    = v; }

    float saturation() const { return saturation_; }
    float contrast() const   { return contrast_; }
    float brightness() const { return brightness_; }
    float gamma() const      { return gamma_; }
    float vignette() const   { return vignette_; }
    float bloomStrength() const  { return bloomStrength_; }
    float bloomThreshold() const { return bloomThreshold_; }
    float chromatic() const      { return chromatic_; }
    float grain() const          { return grain_; }
    float scanline() const       { return scanline_; }
    float dither() const         { return dither_; }

    // 统一入口：sourceSize 是源纹理像素尺寸，targetSize 是最终窗口尺寸
    void draw(sf::RenderTarget& target,
              const sf::Texture& source,
              sf::Vector2u sourceSize,
              sf::Vector2u targetSize);

private:
    void renderBloom(const sf::Texture& source, sf::Vector2u sourceSize);

    sf::Shader shader_;
    sf::Shader brightpassShader_;
    sf::Shader blurShader_;
    bool loaded_ = false;
    bool bloomShadersLoaded_ = false;

    sf::RenderTexture bloomRT_A_;
    sf::RenderTexture bloomRT_B_;
    sf::Vector2u bloomSizeCache_{0, 0};
    sf::Texture dummyBloomTex_;

    float saturation_ = 1.f;
    float contrast_   = 1.f;
    float brightness_ = 1.f;
    float gamma_      = 1.f;
    float vignette_   = 0.f;

    float bloomStrength_  = 0.f;
    float bloomThreshold_ = 0.7f;

    float chromatic_ = 0.f;
    float grain_     = 0.f;
    float scanline_  = 0.f;
    float dither_    = 0.f;

    sf::Clock grainClock_;
};