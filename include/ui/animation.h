#pragma once
#include <SFML/Graphics.hpp>

// 全局动画系统：控制动画开关、速度，提供插值工具
namespace Anim {

// 全局开关：关闭时所有过渡都是瞬间切换
bool  isEnabled();
void  setEnabled(bool e);

// 速度系数：0.5 = 慢，1.0 = 正常，2.0 = 快
float getSpeed();
void  setSpeed(float s);

// 颜色/浮点插值
sf::Color lerp(sf::Color a, sf::Color b, float t);
float     lerpF(float a, float b, float t);

// 指数逼近：从 current 平滑趋近 target
sf::Color approach(sf::Color current, sf::Color target, float dt);
float     approachF(float current, float target, float dt);

} // namespace Anim