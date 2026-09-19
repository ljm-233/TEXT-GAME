#include "upscaler.h"
#include <filesystem>
#include <iostream>

void Upscaler::load(const std::string& shaderDir) {
    std::filesystem::path dir(shaderDir);

    // ⭐ 加载双三次 shader
    {
        auto frag = (dir / "upscale.frag").string();
        if (std::filesystem::exists(frag)) {
            if (shaderBicubic_.loadFromFile(frag, sf::Shader::Type::Fragment)) {
                bicubicLoaded_ = true;
                std::cerr << "[Upscaler] 双三次 shader 已加载\n";
            } else {
                std::cerr << "[Upscaler] 双三次 shader 编译失败\n";
            }
        }
    }

    // ⭐ 加载 FSR1 shader
    {
        auto frag = (dir / "fsr1.frag").string();
        if (std::filesystem::exists(frag)) {
            if (shaderFsr1_.loadFromFile(frag, sf::Shader::Type::Fragment)) {
                fsr1Loaded_ = true;
                std::cerr << "[Upscaler] FSR1 shader 已加载\n";
            } else {
                std::cerr << "[Upscaler] FSR1 shader 编译失败\n";
            }
        }
    }
}

void Upscaler::draw(sf::RenderTarget& target,
                    const sf::Texture& source,
                    sf::Vector2u sourceSize,
                    sf::Vector2u targetSize) {
    if (sourceSize.x == 0 || sourceSize.y == 0 ||
        targetSize.x == 0 || targetSize.y == 0) {
        return;
    }

    sf::Sprite sprite(source);
    sprite.setScale({
        static_cast<float>(targetSize.x) / static_cast<float>(sourceSize.x),
        static_cast<float>(targetSize.y) / static_cast<float>(sourceSize.y)
    });

    // ⭐ 选择 shader
    sf::Shader* chosen = nullptr;
    if (mode_ == 1 && bicubicLoaded_)      chosen = &shaderBicubic_;
    else if (mode_ == 2 && fsr1Loaded_)    chosen = &shaderFsr1_;

    if (chosen) {
        chosen->setUniform("uSource", source);
        chosen->setUniform("uSourceSize", sf::Vector2f{
            static_cast<float>(sourceSize.x),
            static_cast<float>(sourceSize.y)
        });
        chosen->setUniform("uOutputSize", sf::Vector2f{
            static_cast<float>(targetSize.x),
            static_cast<float>(targetSize.y)
        });

        sf::RenderStates states;
        states.shader = chosen;
        target.draw(sprite, states);
    } else {
        // 关 / shader 加载失败 → 双线性
        target.draw(sprite);
    }
}