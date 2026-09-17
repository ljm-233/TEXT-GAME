#pragma once
#include "vec2.h"
#include <functional>
#include <variant>
#include <vector>

// ============================================================
// 游戏事件
// ============================================================

struct EvJumped     { Vec2 pos; };
struct EvLanded     { Vec2 pos; float intensity = 1.f; };
struct EvCoined     { Vec2 pos; };
struct EvStomped    { Vec2 pos; };
struct EvHurt       { Vec2 pos; };
struct EvCheckpoint { Vec2 pos; };
struct EvJumpPad    { Vec2 pos; };
struct EvLevelComplete  {};
struct EvLifeExhausted  {};   // 生命耗尽，自动重生

using GameEvent = std::variant<
    EvJumped, EvLanded, EvCoined, EvStomped, EvHurt,
    EvCheckpoint, EvJumpPad, EvLevelComplete, EvLifeExhausted>;

// ============================================================
// 事件总线
// ============================================================

class EventBus {
public:
    using Handler = std::function<void(const GameEvent&)>;

    void subscribe(Handler h) { handlers_.push_back(std::move(h)); }

    void emit(const GameEvent& e) const {
        for (const auto& h : handlers_) {
            if (h) h(e);
        }
    }

    void clear() { handlers_.clear(); }

private:
    std::vector<Handler> handlers_;
};