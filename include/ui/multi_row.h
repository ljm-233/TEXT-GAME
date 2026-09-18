#pragma once

#include "button.h"
#include "text_strings.h"
#include "ui_scale.h"
#include "utf8.h"

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// ⭐ 统一管理"一组按钮里选一个"的设置项
// 只负责：存储按钮、当前选中索引、回调；不负责布局和创建
struct MultiRow {
    std::string labelKey;
    sf::Text label;
    std::vector<std::unique_ptr<Button>> buttons;
    int currentIndex = 0;
    std::function<void(int)> onSelected;

    // ⭐ 布局参数
    int   columns = 0;      // 0 = 单行；> 0 = 每行按钮数
    float stepX   = 96.f;   // 水平步进（按钮原点间距）
    float stepY   = 50.f;   // 单行 = y 步进；多行 = 每行垂直步进

    MultiRow(const sf::Font& font,
             const std::string& key,
             std::function<void(int)> cb)
        : labelKey(key),
          label(font, toSf(Str::T(key.c_str())), scaledFontSize(20)),
          onSelected(std::move(cb)) {
        label.setFillColor(sf::Color(230, 230, 230));
    }

    void addButton(std::unique_ptr<Button> btn) {
        buttons.push_back(std::move(btn));
    }

    void setSelected(int idx) {
        currentIndex = idx;
        for (size_t i = 0; i < buttons.size(); ++i) {
            buttons[i]->setSelected(static_cast<int>(i) == idx);
        }
    }

    void refreshLabel() {
        label.setString(toSf(Str::T(labelKey.c_str())));
    }
};