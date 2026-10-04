#pragma once
//
// 编辑器里的格子几何。
//
// 抽出来的理由：拖拽绘制最容易出的错就是差一格（往左上拖和往右下拖
// 结果不一致、连线补格漏掉端点）。这些是纯几何、与 sf::Font / 渲染无关，
// 所以做成纯函数就能测 —— 编辑器场景本身构造需要字体，测不了。
//
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace EditorTools {

/// 两个角点围出的全部格子（含两端），已按行列归一化。
///
/// 拖拽方向任意：从左下拖到右上，和从右上拖到左下，结果必须一致。
std::vector<sf::Vector2i> rectCells(sf::Vector2i a, sf::Vector2i b);

/// 两点之间的一条线（Bresenham），含两端。
///
/// 用途：鼠标事件是离散采样的，快速拖动时相邻两点可能隔了好几格，
/// 只画端点会留下断断续续的虚线。
std::vector<sf::Vector2i> lineCells(sf::Vector2i from, sf::Vector2i to);

} // namespace EditorTools
