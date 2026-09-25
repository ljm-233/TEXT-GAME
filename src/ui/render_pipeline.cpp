#include "render_pipeline.h"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace {
bool needsRT(float scale, bool postActive) {
    if (scale < 0.99f || scale > 1.01f) return true;
    return postActive;
}
} // namespace

void RenderPipeline::init(const std::string& shaderDir) {
    upscaler_.load(shaderDir);
    upscaleLoaded_ = true;
    postProcessor_.load(shaderDir);
}

void RenderPipeline::setRenderScale(float s) {
    s = std::clamp(s, 0.10f, 2.0f);
    if (std::abs(s - renderScale_) < 0.01f) return;
    renderScale_ = s;
    rtNeedsResize_ = true;
}

void RenderPipeline::setUpscaleMode(int mode) {
    upscaler_.setMode(mode);
}

int RenderPipeline::getUpscaleMode() const {
    return upscaler_.getMode();
}

bool RenderPipeline::needsRT() const {
    return ::needsRT(renderScale_, postProcessor_.isActive());
}

sf::RenderTarget& RenderPipeline::target(sf::RenderWindow& window) {
    if (!needsRT()) return window;
    return rt_;
}

void RenderPipeline::beginFrame(sf::RenderWindow& window) {
    if (!needsRT()) return;

    auto winSize = window.getSize();
    if (winSize.x == 0 || winSize.y == 0) return;

    unsigned rw = std::max(1u, static_cast<unsigned>(winSize.x * renderScale_));
    unsigned rh = std::max(1u, static_cast<unsigned>(winSize.y * renderScale_));

    if (rtNeedsResize_ || rt_.getSize() != sf::Vector2u{rw, rh}) {
        if (!rt_.resize({rw, rh})) {
            renderScale_ = 1.0f;
            return;
        }
        rt_.setSmooth(true);
        rtNeedsResize_ = false;
    }

    rt_.setView(sf::View(sf::FloatRect(
        {0.f, 0.f}, {static_cast<float>(winSize.x), static_cast<float>(winSize.y)})));
}

void RenderPipeline::endFrame(sf::RenderWindow& window) {
    if (!needsRT()) {
        upscalePostMs_ = 0.f;
        window.display();
        return;
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    rt_.display();

    auto winSize = window.getSize();
    auto rtSize  = rt_.getSize();
    if (rtSize.x == 0 || rtSize.y == 0 || winSize.x == 0 || winSize.y == 0) {
        auto t1 = std::chrono::high_resolution_clock::now();
        upscalePostMs_ = std::chrono::duration<float, std::milli>(t1 - t0).count();
        window.display();
        return;
    }

    window.setView(window.getDefaultView());

    const bool isSupersample = renderScale_ > 1.01f;
    const bool useUpscaler   = !isSupersample
                             && renderScale_ < 0.99f
                             && upscaleLoaded_ && upscaler_.isLoaded();
    const bool usePost       = postProcessor_.isActive();

    if (usePost && useUpscaler) {
        if (ppInputRT_.getSize() != winSize) {
            if (!ppInputRT_.resize(winSize)) {
                upscaler_.draw(window, rt_.getTexture(), rtSize, winSize);
                window.display();
                return;
            }
            ppInputRT_.setSmooth(true);
        }
        ppInputRT_.clear();
        ppInputRT_.setView(ppInputRT_.getDefaultView());
        upscaler_.draw(ppInputRT_, rt_.getTexture(), rtSize, winSize);
        ppInputRT_.display();
        postProcessor_.draw(window, ppInputRT_.getTexture(), winSize, winSize);
    } else if (usePost) {
        postProcessor_.draw(window, rt_.getTexture(), rtSize, winSize);
    } else if (useUpscaler) {
        upscaler_.draw(window, rt_.getTexture(), rtSize, winSize);
    } else {
        sf::Sprite s(rt_.getTexture());
        s.setScale({
            static_cast<float>(winSize.x) / static_cast<float>(rtSize.x),
            static_cast<float>(winSize.y) / static_cast<float>(rtSize.y)
        });
        window.draw(s);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    upscalePostMs_ = std::chrono::duration<float, std::milli>(t1 - t0).count();

    window.display();
}