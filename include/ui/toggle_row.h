#pragma once

#include "button.h"
#include "text_strings.h"
#include "ui_scale.h"
#include "utf8.h"

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>

// ⭐ 统一管理一个 ON/OFF 类设置项
// SettingsScene 和 PauseMenu 共用
struct ToggleRow {
    std::string labelKey;
    sf::Text label;
    std::unique_ptr<Button> onButton;
    std::unique_ptr<Button> offButton;
    bool currentValue = false;
    std::function<void(bool)> onChanged;

    ToggleRow(const sf::Font& font,
              const std::string& key,
              std::function<void(bool)> cb)
        : labelKey(key),
          label(font, toSf(Str::T(key.c_str())), scaledFontSize(20)),
          onChanged(std::move(cb)) {
        label.setFillColor(sf::Color(230, 230, 230));
    }
};