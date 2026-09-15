#include "animator.h"

void Animator::addClip(const std::string& name, const Clip& clip) {
    clips_[name] = clip;
}

void Animator::play(const std::string& name) {
    if (currentClip_ == name) return;   // 已经在播，不重置
    auto it = clips_.find(name);
    if (it == clips_.end()) return;
    currentClip_ = name;
    currentFrame_ = 0;
    timer_ = 0.f;
}

void Animator::update(float dt) {
    if (currentClip_.empty()) return;
    auto it = clips_.find(currentClip_);
    if (it == clips_.end()) return;
    const auto& clip = it->second;
    if (clip.frames.empty()) return;

    timer_ += dt;
    float frameDur = 1.f / clip.fps;
    while (timer_ >= frameDur) {
        timer_ -= frameDur;
        ++currentFrame_;
        if (currentFrame_ >= static_cast<int>(clip.frames.size())) {
            if (clip.loop) currentFrame_ = 0;
            else currentFrame_ = static_cast<int>(clip.frames.size()) - 1;
        }
    }
}

void Animator::applyTo(sf::Sprite& sprite) const {
    auto it = clips_.find(currentClip_);
    if (it == clips_.end() || it->second.frames.empty()) return;
    sprite.setTextureRect(it->second.frames[currentFrame_]);
}