#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "upscaler.h"
#include "postprocess.h"

// ⭐ 渲染管线：渲染缩放 + Upscaler + PostProcessor
//    Window 只管窗口本身，所有"渲染"逻辑在这里
class RenderPipeline {
public:
    // 加载 shader（shaderDir 是 *.frag 所在目录）
    void init(const std::string& shaderDir);
    bool isLoaded() const { return upscaleLoaded_; }

    // 渲染缩放
    //   < 1.0：低分辨率渲染 + upscaler 上采样
    //   = 1.0：直接渲染到窗口
    //   > 1.0：超采样渲染 + 降采样（抗锯齿）
    void setRenderScale(float s);
    float getRenderScale() const { return renderScale_; }

    void setUpscaleMode(int mode);
    int  getUpscaleMode() const;

    PostProcessor& postProcess() { return postProcessor_; }
    const PostProcessor& postProcess() const { return postProcessor_; }

    // 每帧：beginFrame → 场景用 target() 画 → endFrame
    sf::RenderTarget& target(sf::RenderWindow& window);
    void beginFrame(sf::RenderWindow& window);
    void endFrame(sf::RenderWindow& window);

    // 性能：上一帧 endFrame 中 upscaler + postprocess 绘制耗时（毫秒）
    float upscalePostMs() const { return upscalePostMs_; }

    // 窗口 recreate() 后调用，让 RT 重新分配
    void invalidate() { rtNeedsResize_ = true; }

private:
    bool needsRT() const;

    // 中间渲染纹理
    sf::RenderTexture rt_;
    float renderScale_ = 1.0f;
    bool  rtNeedsResize_ = true;

    // 超分
    Upscaler upscaler_;
    bool     upscaleLoaded_ = false;

    // 后处理
    PostProcessor postProcessor_;
    sf::RenderTexture ppInputRT_;

    // 性能
    float upscalePostMs_ = 0.f;
};