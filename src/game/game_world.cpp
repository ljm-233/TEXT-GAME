#include "game_world.h"
#include "coin.h"
#include "enemy.h"
#include "jump_pad.h"
#include "checkpoint.h"
#include "moving_platform.h"
#include "key.h"
#include "door.h"
#include "spike.h"
#include "player_sprite_factory.h"
#include "game_constants.h"
#include <algorithm>
#include <cmath>

GameWorld::GameWorld(std::unique_ptr<Level> level, int levelIndex)
    : level_(std::move(level)),
      levelIndex_(levelIndex) {

    camera_.setLevelBounds(static_cast<float>(level_->pixelWidth()),
                           static_cast<float>(level_->pixelHeight()));

    level_->setPseudo3D(true);

    spawnPlayer(level_->playerSpawn());
    player_->setKillY(static_cast<float>(level_->pixelHeight() + 64));
    spawnLevelObjects();

    camera_.snapTo(player_->bounds().center());
}

void GameWorld::spawnPlayer(Vec2 spawn) {
    auto p = std::make_unique<Player>(spawn, PlayerSpriteFactory::getSheet());
    player_ = p.get();
    objects_.push_back(std::move(p));
}

void GameWorld::spawnLevelObjects() {
    for (const auto& pos : level_->coinSpawns())
        objects_.push_back(std::make_unique<Coin>(pos, level_->tileSize()));
    totalCoins_ = static_cast<int>(level_->coinSpawns().size());

    for (const auto& pos : level_->enemySpawns())
        objects_.push_back(std::make_unique<Enemy>(pos, level_->tileSize()));

    for (const auto& pos : level_->jumpPadSpawns())
        objects_.push_back(std::make_unique<JumpPad>(pos, level_->tileSize()));

    for (const auto& pos : level_->checkpointSpawns())
        objects_.push_back(std::make_unique<Checkpoint>(pos, level_->tileSize()));

    for (const auto& pos : level_->movingPlatformSpawns())
        objects_.push_back(std::make_unique<MovingPlatform>(
            pos, level_->tileSize(), 4.f * level_->tileSize(), true, 80.f));

    for (const auto& pos : level_->verticalPlatformSpawns())
        objects_.push_back(std::make_unique<MovingPlatform>(
            pos, level_->tileSize(), 3.f * level_->tileSize(), false, 60.f));

    for (const auto& pos : level_->keySpawns())
        objects_.push_back(std::make_unique<Key>(pos, level_->tileSize()));

    doors_.clear();
    for (const auto& pos : level_->doorSpawns()) {
        auto d = std::make_unique<Door>(pos, level_->tileSize());
        int tx = static_cast<int>(pos.x) / level_->tileSize();
        int ty = static_cast<int>(pos.y) / level_->tileSize();

        level_->setDynamicSolid(tx, ty, true);

        doors_.push_back({d.get(), tx, ty});
        objects_.push_back(std::move(d));
    }

    for (const auto& pos : level_->spikeSpawns())
        objects_.push_back(std::make_unique<Spike>(pos, level_->tileSize()));
}

void GameWorld::setViewSize(float w, float h) {
    camera_.setViewSize(w, h);
}

void GameWorld::handleEvent(const sf::Event& event) {
    if (state_ != State::Playing) return;
    if (player_) player_->handleEvent(event);
}

void GameWorld::update(float dt) {
    if (state_ != State::Playing) return;

    if (player_) player_->handleGamepad();

    accumulator_ += dt;
    int iterations = 0;
    while (accumulator_ >= GameConst::kFixedTimeStep && iterations < 8) {
        float step = GameConst::kFixedTimeStep;

        // ① 移动平台先更新
        for (auto& obj : objects_) {
            if (obj->type() == GameObject::Type::Platform) {
                obj->update(step, *level_);
            }
        }

        // ② 玩家物理
        if (player_) player_->update(step, *level_);

        // ③ 玩家站台检测
        if (player_) {
            AABB pb = player_->bounds();
            for (auto& obj : objects_) {
                if (obj->type() != GameObject::Type::Platform) continue;
                auto* mp = static_cast<MovingPlatform*>(obj.get());
                AABB plat = mp->bounds();

                bool overlapX = pb.right() > plat.left() + 1.f &&
                                pb.left() < plat.right() - 1.f;
                bool nearTop  = pb.bottom() >= plat.top() - 4.f &&
                                pb.bottom() <= plat.top() + 10.f;

                if (overlapX && nearTop && player_->velocity().y >= -1.f) {
                    player_->landOnPlatform(plat.top());
                    player_->moveBy(mp->lastDelta());
                    break;
                }
            }
        }

        // ④ 其他对象（跳过平台和玩家）
        for (auto& obj : objects_) {
            auto t = obj->type();
            if (t != GameObject::Type::Platform &&
                t != GameObject::Type::Player) {
                obj->update(step, *level_);
            }
        }

        checkCollisionsSafe();
        if (state_ != State::Playing) return;
        accumulator_ -= step;
        ++iterations;
    }

    // 跳跃 / 落地事件
    if (player_) {
        Vec2 pb = player_->bounds().center();
        float footX = pb.x;
        float footY = player_->bounds().bottom();

        if (player_->consumeJustJumped()) {
            bus_.emit(EvJumped{Vec2{footX, footY}});
        }
        if (player_->consumeJustLanded()) {
            float vy = std::abs(player_->velocity().y);
            float intensity = std::clamp(vy / 700.f, 0.5f, 1.5f);
            bus_.emit(EvLanded{Vec2{footX, footY}, intensity});
        }
    }

    // 掉图
    if (player_ && player_->consumeFellOut()) {
        --lives_;
        bus_.emit(EvHurt{player_->bounds().center()});
        if (screenShake_) camera_.shake(8.f, 0.3f);
        if (lives_ <= 0) {
            pendingRestart_ = true;
            respawnDelayTimer_ = 0.5f;
            return;
        }
    }

    // 到达终点
    if (checkGoalReached()) {
        state_ = State::LevelComplete;
        bus_.emit(EvLevelComplete{});
        return;
    }

    // 生命耗尽 → 延迟 0.5 秒后从最近的存档点重生
    if (pendingRestart_) {
        if (respawnDelayTimer_ > 0.f) {
            respawnDelayTimer_ -= dt;
            // 冻结游戏逻辑，但保留粒子和摄像机效果
            if (particlesEnabled_) particles_.update(dt);
            camera_.updateShake(dt);
            if (player_) camera_.follow(player_->bounds().center(),
                                        {0.f, 0.f}, dt);
            return;
        }
        // 延迟结束，触发事件并从存档点重生
        bus_.emit(EvLifeExhausted{});
        respawnAtCheckpoint();
        return;
    }

    objects_.erase(
        std::remove_if(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<GameObject>& o) {
                return o->isRemovable();
            }),
        objects_.end());

    if (player_) camera_.follow(player_->bounds().center(),
                                player_->velocity(), dt);
    camera_.updateShake(dt);
    if (particlesEnabled_) particles_.update(dt);
    else                    particles_.clear();
}

void GameWorld::checkCollisionsSafe() {
    if (!player_) return;
    AABB pb = player_->bounds();

    for (auto& obj : objects_) {
        if (obj.get() == player_) continue;
        if (obj->type() == GameObject::Type::Platform) continue;
        if (!obj->bounds().intersects(pb)) continue;

        switch (obj->type()) {
            case GameObject::Type::Coin: {
                auto* c = static_cast<Coin*>(obj.get());
                if (!c->collected()) {
                    c->collect();
                    ++coins_;
                    bus_.emit(EvCoined{c->bounds().center()});
                }
                break;
            }
            case GameObject::Type::Enemy: {
                auto* e = static_cast<Enemy*>(obj.get());
                if (e->killed()) break;

                bool falling = player_->velocity().y > 0.f;
                float overlap = pb.bottom() - e->bounds().top();
                bool fromAbove = overlap < GameConst::kStompTolerance;

                if (falling && fromAbove) {
                    e->kill();
                    player_->bounce();
                    bus_.emit(EvStomped{e->bounds().center()});
                    if (screenShake_) camera_.shake(4.f, 0.15f);
                } else if (!player_->isInvincible()) {
                    player_->takeDamage();
                    --lives_;
                    bus_.emit(EvHurt{pb.center()});
                    if (screenShake_) camera_.shake(8.f, 0.3f);
                    if (lives_ <= 0) {
                        pendingRestart_ = true;
                        respawnDelayTimer_ = 0.5f;
                        return;
                    }
                }
                break;
            }
            case GameObject::Type::JumpPad: {
                auto* jp = static_cast<JumpPad*>(obj.get());
                if (jp->canTrigger()) {
                    player_->setVelocityY(JumpPad::kLaunchSpeed);
                    jp->trigger();
                    bus_.emit(EvJumpPad{pb.center()});
                }
                break;
            }
            case GameObject::Type::Checkpoint: {
                auto* cp = static_cast<Checkpoint*>(obj.get());
                if (!cp->isActive()) {
                    // ⭐ 取消上一个激活的存档点
                    if (activeCheckpoint_ && activeCheckpoint_ != cp) {
                        activeCheckpoint_->deactivate();
                    }
                    cp->activate();
                    activeCheckpoint_ = cp;
                    player_->setSpawn(cp->respawnPos());
                    bus_.emit(EvCheckpoint{cp->bounds().center()});
                }
                break;
            }
            case GameObject::Type::Key: {
                auto* k = static_cast<Key*>(obj.get());
                if (!k->collected()) {
                    k->collect();
                    player_->addKey();
                    bus_.emit(EvCoined{k->bounds().center()});

                    for (auto& entry : doors_) {
                        entry.door->unlock();
                        level_->setDynamicSolid(entry.tx, entry.ty, false);
                    }
                }
                break;
            }
            case GameObject::Type::Door:
                // 门是动态瓦片（Level::dynamicSolid_），
                // Player 的瓦片碰撞会自动挡住，这里不需要处理
                break;

            case GameObject::Type::Spike: {
                if (!player_->isInvincible()) {
                    player_->takeDamage();
                    --lives_;
                    bus_.emit(EvHurt{pb.center()});
                    if (screenShake_) camera_.shake(8.f, 0.3f);
                    if (lives_ <= 0) {
                        pendingRestart_ = true;
                        respawnDelayTimer_ = 0.5f;
                        return;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

bool GameWorld::checkGoalReached() const {
    if (!level_->hasGoal() || !player_) return false;
    AABB goal{level_->goalPos().x, level_->goalPos().y,
              static_cast<float>(level_->tileSize()),
              static_cast<float>(level_->tileSize())};
    return player_->bounds().intersects(goal);
}

void GameWorld::renderShadow(sf::RenderTarget& target,
                             Vec2 worldPos,
                             float width, float height) const {
    // ⭐ 一次性生成径向渐变阴影纹理
    if (!shadowTexReady_) {
        constexpr unsigned kSize = 64;
        sf::Image img({kSize, kSize}, sf::Color::Transparent);
        const float half = kSize * 0.5f;
        for (unsigned y = 0; y < kSize; ++y) {
            for (unsigned x = 0; x < kSize; ++x) {
                float dx = (x + 0.5f) - half;
                float dy = (y + 0.5f) - half;
                float r = std::sqrt(dx * dx + dy * dy) / half;
                if (r >= 1.f) continue;
                // ⭐ 内 40% 完全不透明，外部到边缘平滑衰减
                float t = std::clamp((r - 0.4f) / 0.6f, 0.f, 1.f);
                float a = 1.f - t;
                a = a * a;   // 边缘再柔一点
                auto a8 = static_cast<std::uint8_t>(a * 255.f);
                img.setPixel({x, y}, sf::Color(0, 0, 0, a8));
            }
        }
        (void)shadowTex_.loadFromImage(img);
        shadowTex_.setSmooth(true);
        shadowTexReady_ = true;
    }

    int ts = level_->tileSize();
    int tx = static_cast<int>(worldPos.x / ts);

    float shadowY = worldPos.y + height;

    for (int ty = static_cast<int>((worldPos.y + height) / ts);
         ty < level_->height(); ++ty) {
        if (level_->isSolid(tx, ty)) {
            shadowY = static_cast<float>(ty * ts);
            break;
        }
    }

    float distance = shadowY - (worldPos.y + height);

    float alphaFactor = std::clamp(1.f - distance / 300.f, 0.2f, 1.f);
    float scaleFactor = std::clamp(1.f - distance / 500.f, 0.5f, 1.f);

    float rx = (width * 0.70f) * scaleFactor;
    float ry = rx * 0.35f;

    // ⭐ 用 sprite 绘制渐变阴影
    sf::Sprite s(shadowTex_);
    s.setOrigin({32.f, 32.f});   // 纹理中心
    s.setPosition({worldPos.x + width * 0.5f, shadowY - 2.f});
    s.setScale({rx / 32.f, ry / 32.f});
    s.setColor(sf::Color(255, 255, 255,
        static_cast<std::uint8_t>(200 * alphaFactor)));
    target.draw(s);
}

void GameWorld::renderDebugColliders(sf::RenderTarget& target) {
    sf::RectangleShape rect;
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(1.f);

    for (const auto& obj : objects_) {
        AABB b = obj->bounds();
        rect.setSize({b.w, b.h});
        rect.setPosition({b.x, b.y});

        sf::Color color;
        switch (obj->type()) {
            case GameObject::Type::Player:     color = sf::Color(0, 255, 0);   break;
            case GameObject::Type::Enemy:      color = sf::Color(255, 0, 0);   break;
            case GameObject::Type::Coin:       color = sf::Color(255, 255, 0); break;
            case GameObject::Type::JumpPad:    color = sf::Color(0, 200, 255); break;
            case GameObject::Type::Checkpoint: color = sf::Color(255, 128, 0); break;
            case GameObject::Type::Platform:   color = sf::Color(160, 120, 80); break;
            case GameObject::Type::Key:        color = sf::Color(255, 210, 60); break;
            case GameObject::Type::Door:       color = sf::Color(140, 90, 50);  break;
            case GameObject::Type::Spike:      color = sf::Color(200, 200, 210); break;
            default:                            color = sf::Color(200, 200, 200); break;
        }
        rect.setOutlineColor(color);
        target.draw(rect);
    }
}

void GameWorld::render(sf::RenderTarget& target) {
    Vec2 camTL = camera_.effectivePosition();

    level_->render(target,
                   camTL.x, camTL.y,
                   camera_.viewWidth(), camera_.viewHeight());

    if (pseudo3D_) {
        for (const auto& obj : objects_) {
            switch (obj->type()) {
                case GameObject::Type::Player: {
                    AABB b = obj->bounds();
                    renderShadow(target, {b.x, b.y}, b.w, b.h);
                    break;
                }
                case GameObject::Type::Enemy: {
                    AABB b = obj->bounds();
                    renderShadow(target, {b.x, b.y}, b.w, b.h);
                    break;
                }
                default: break;
            }
        }
    }

    for (const auto& obj : objects_) {
        obj->render(target);
    }

    if (particlesEnabled_) particles_.render(target);
    if (showColliders_) renderDebugColliders(target);
}

void GameWorld::respawnAtCheckpoint() {
    // ⭐ 从最近的存档点重生：
    //   - 生命重置为初始值
    //   - 金币 / 敌人 / 已激活的存档点 保持不变
    //   - 玩家位置回到 spawn_（激活 checkpoint 时已设为 checkpoint 位置）
    lives_ = initialLives_;
    state_ = State::Playing;
    pendingRestart_ = false;
    respawnDelayTimer_ = 0.f;
    accumulator_ = 0.f;
    particles_.clear();

    if (player_) {
        player_->respawn(player_->spawn());
        camera_.snapTo(player_->bounds().center());
    }
}

void GameWorld::reset() {
    lives_ = initialLives_;
    coins_ = 0;
    state_ = State::Playing;
    accumulator_ = 0.f;
    particles_.clear();

    // ⭐ 对象即将销毁，先清空指针避免悬垂
    activeCheckpoint_ = nullptr;
    objects_.clear();
    doors_.clear();
    if (player_) player_->resetKeys();

    for (int ty = 0; ty < level_->height(); ++ty)
        for (int tx = 0; tx < level_->width(); ++tx)
            level_->setDynamicSolid(tx, ty, false);

    spawnPlayer(level_->playerSpawn());
    player_->setKillY(static_cast<float>(level_->pixelHeight() + 64));
    spawnLevelObjects();

    camera_.snapTo(player_->bounds().center());
}