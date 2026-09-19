#pragma once
#include <SFML/Graphics.hpp>
#include <string>

// ⭐ 超分辨率放大器
//   mode = 0: 关（双线性）
//   mode = 1: 双三次 + 轻度锐化（简化版）
//   mode = 2: FSR 1.0（EASU + RCAS）
class Upscaler {
public:
    void load(const std::string& shaderDir);
    bool isLoaded() const { return bicubicLoaded_ || fsr1Loaded_; }

    void setMode(int mode) { mode_ = mode; }
    int  getMode() const { return mode_; }

    void draw(sf::RenderTarget& target,
              const sf::Texture& source,
              sf::Vector2u sourceSize,
              sf::Vector2u targetSize);

private:
    sf::Shader shaderBicubic_;
    sf::Shader shaderFsr1_;
    bool       bicubicLoaded_ = false;
    bool       fsr1Loaded_    = false;
    int        mode_          = 1;
};