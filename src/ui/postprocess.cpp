#include "postprocess.h"
#include <cmath>
#include <filesystem>
#include <iostream>

namespace {
constexpr float kBloomScale = 0.25f;
}

void PostProcessor::load(const std::string& shaderDir) {
    auto dir = std::filesystem::path(shaderDir);

    {
        auto frag = (dir / "postprocess.frag").string();
        if (std::filesystem::exists(frag)) {
            if (shader_.loadFromFile(frag, sf::Shader::Type::Fragment)) {
                loaded_ = true;
                std::cerr << "[PostProcess] shader 已加载\n";
            } else {
                std::cerr << "[PostProcess] shader 编译失败\n";
            }
        }
    }

    {
        auto bp = (dir / "brightpass.frag").string();
        auto bl = (dir / "blur.frag").string();
        bool ok_bp = std::filesystem::exists(bp)
            && brightpassShader_.loadFromFile(bp, sf::Shader::Type::Fragment);
        bool ok_bl = std::filesystem::exists(bl)
            && blurShader_.loadFromFile(bl, sf::Shader::Type::Fragment);
        if (ok_bp && ok_bl) {
            bloomShadersLoaded_ = true;
            std::cerr << "[PostProcess] bloom shader 已加载\n";
        } else {
            std::cerr << "[PostProcess] bloom shader 加载失败\n";
        }
    }

    // dummy bloom 纹理（1x1 黑），避免 uniform 未绑定时报警告
    if (loaded_) {
        sf::Image img({1, 1}, sf::Color::Black);
        (void)dummyBloomTex_.loadFromImage(img);
        shader_.setUniform("uBloomTex", dummyBloomTex_);
        shader_.setUniform("uBloomStrength", 0.f);
    }
}

bool PostProcessor::bloomActive() const {
    return bloomShadersLoaded_ && bloomStrength_ > 0.001f;
}

bool PostProcessor::isActive() const {
    if (loaded_) {
        if (std::abs(saturation_ - 1.f) > 0.001f) return true;
        if (std::abs(contrast_   - 1.f) > 0.001f) return true;
        if (std::abs(brightness_ - 1.f) > 0.001f) return true;
        if (std::abs(gamma_      - 1.f) > 0.001f) return true;
        if (vignette_ > 0.001f) return true;
        if (chromatic_ > 0.001f) return true;
        if (grain_ > 0.001f) return true;
        if (scanline_ > 0.001f) return true;
        if (dither_ > 0.001f) return true;
    }
    return bloomActive();
}

void PostProcessor::renderBloom(const sf::Texture& source, sf::Vector2u sourceSize) {
    if (!bloomShadersLoaded_) return;
    if (sourceSize.x == 0 || sourceSize.y == 0) return;

    unsigned bw = std::max(1u, static_cast<unsigned>(sourceSize.x * kBloomScale));
    unsigned bh = std::max(1u, static_cast<unsigned>(sourceSize.y * kBloomScale));

    if (bloomSizeCache_ != sf::Vector2u{bw, bh}) {
        if (!bloomRT_A_.resize({bw, bh})) return;
        if (!bloomRT_B_.resize({bw, bh})) return;
        bloomRT_A_.setSmooth(true);
        bloomRT_B_.setSmooth(true);
        bloomSizeCache_ = {bw, bh};
    }

    sf::Vector2f srcSizeF{static_cast<float>(sourceSize.x),
                          static_cast<float>(sourceSize.y)};
    sf::Vector2f outSizeF{static_cast<float>(bw), static_cast<float>(bh)};

    // Pass 1: brightpass
    {
        sf::Sprite s(source);
        s.setScale({outSizeF.x / srcSizeF.x, outSizeF.y / srcSizeF.y});

        brightpassShader_.setUniform("uSource", source);
        brightpassShader_.setUniform("uSourceSize", srcSizeF);
        brightpassShader_.setUniform("uOutputSize", outSizeF);
        brightpassShader_.setUniform("uThreshold", bloomThreshold_);

        bloomRT_A_.clear(sf::Color::Black);
        bloomRT_A_.setView(bloomRT_A_.getDefaultView());
        sf::RenderStates st;
        st.shader = &brightpassShader_;
        bloomRT_A_.draw(s, st);
        bloomRT_A_.display();
    }

    // Pass 2: 水平模糊 A → B
    {
        sf::Sprite s(bloomRT_A_.getTexture());
        blurShader_.setUniform("uSource", bloomRT_A_.getTexture());
        blurShader_.setUniform("uSourceSize", outSizeF);
        blurShader_.setUniform("uOutputSize", outSizeF);
        blurShader_.setUniform("uDirection", sf::Vector2f{1.f, 0.f});

        bloomRT_B_.clear(sf::Color::Black);
        bloomRT_B_.setView(bloomRT_B_.getDefaultView());
        sf::RenderStates st;
        st.shader = &blurShader_;
        bloomRT_B_.draw(s, st);
        bloomRT_B_.display();
    }

    // Pass 3: 垂直模糊 B → A
    {
        sf::Sprite s(bloomRT_B_.getTexture());
        blurShader_.setUniform("uSource", bloomRT_B_.getTexture());
        blurShader_.setUniform("uSourceSize", outSizeF);
        blurShader_.setUniform("uOutputSize", outSizeF);
        blurShader_.setUniform("uDirection", sf::Vector2f{0.f, 1.f});

        bloomRT_A_.clear(sf::Color::Black);
        bloomRT_A_.setView(bloomRT_A_.getDefaultView());
        sf::RenderStates st;
        st.shader = &blurShader_;
        bloomRT_A_.draw(s, st);
        bloomRT_A_.display();
    }
}

void PostProcessor::draw(sf::RenderTarget& target,
                         const sf::Texture& source,
                         sf::Vector2u sourceSize,
                         sf::Vector2u targetSize) {
    if (targetSize.x == 0 || targetSize.y == 0) return;
    if (sourceSize.x == 0 || sourceSize.y == 0) return;

    const bool useBloom = bloomActive();
    if (useBloom) renderBloom(source, sourceSize);

    sf::Sprite sprite(source);
    sprite.setScale({
        static_cast<float>(targetSize.x) / static_cast<float>(sourceSize.x),
        static_cast<float>(targetSize.y) / static_cast<float>(sourceSize.y)
    });

    shader_.setUniform("uSource", source);
    shader_.setUniform("uOutputSize", sf::Vector2f{
        static_cast<float>(targetSize.x), static_cast<float>(targetSize.y)});
    shader_.setUniform("uSaturation", saturation_);
    shader_.setUniform("uContrast",   contrast_);
    shader_.setUniform("uBrightness", brightness_);
    shader_.setUniform("uGamma",      gamma_);
    shader_.setUniform("uVignette",   vignette_);
    shader_.setUniform("uChromatic",  chromatic_);
    shader_.setUniform("uGrain",      grain_);
    shader_.setUniform("uScanline",   scanline_);
    shader_.setUniform("uDither",     dither_);
    shader_.setUniform("uTime",       grainClock_.getElapsedTime().asSeconds());

    if (useBloom) {
        shader_.setUniform("uBloomTex", bloomRT_A_.getTexture());
        shader_.setUniform("uBloomStrength", bloomStrength_);
    } else {
        shader_.setUniform("uBloomTex", dummyBloomTex_);
        shader_.setUniform("uBloomStrength", 0.f);
    }

    sf::RenderStates states;
    states.shader = &shader_;
    target.draw(sprite, states);
}