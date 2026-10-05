#pragma once
#include "wallpaper/wallpaper_info.h"

#include <SFML/Graphics/Image.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

/// 后台预解码壁纸为 `sf::Image`。
///
/// 设计理由：壁纸启动时最大的那张解码后是 33MB 的纹理，**同步上传 GPU**
/// 足以让启动体感顿一下。把解码放到 worker thread，主线程只在渲染那一刻
/// 把 Image 上传到 Texture（这一步没法后台做 —— sf::Texture 是 GlResource，
/// 必须主线程 GL 上下文），但解码本身几十毫秒可以并发出去。
///
/// **`sf::Image` 不是 `sf::GlResource`**：默认构造、解码、复制都不需要 GL 上下文，
/// 这正是我们能把它整个扔到 worker 里的原因。`loadFromFile` 走 stb_image，
/// 不碰 OpenGL。
///
/// 线程模型：worker 跑一个 pending-set 上的循环；主线程 `request(index)`
/// 把 index 塞进去。worker 解码完一张 → 放进缓存 → 通知主线程可以拿走。
///
/// **缓存是"取走即清"**：主线程用 `take(index)` 取走一张后这一格就空出来，
/// 不会无限堆积。
///
/// 注意：**调用方负责 index 仍然在 Library 列表里**。这里不做越界检查，
/// 因为 Library 才拥有"哪些 index 合法"的权威信息（Library 一旦
/// `scan()`，旧的 index 就作废了 —— 调用方拿新的库时也应该清掉 loader）。
class WallpaperLoader {
public:
    WallpaperLoader();
    ~WallpaperLoader();

    WallpaperLoader(const WallpaperLoader&) = delete;
    WallpaperLoader& operator=(const WallpaperLoader&) = delete;

    /// 立即在主线程同步解码 + 入缓存。
    ///
    /// 这是"启动不卡"的核心：用户当前选中的那张立即可用，
    /// 构造完 Background 后第一帧就能渲染。
    ///
    /// 返回是否成功 —— 失败时（如文件被删了）缓存里不会有这一项，
    /// 调用方应当回退到 index=0 再 prime。
    bool prime(int index, const WallpaperInfo& info);

    /// 让 worker 异步解码 index。等不到马上就用（缓存未命中）时也能调，
    /// 因为 `take()` 会阻塞到 ready 或者 timeout。
    void request(int index, const WallpaperInfo& info);

    /// 等 index 解码完成（如果已在请求中）然后把 Image 取走。
    ///
    /// **取走后缓存清空**——这是单消费者设计：UI 那一份 Background
    /// 就是唯一消费者，取走即消费。
    ///
    /// timeout 之内没拿到就返回 false：调用方可以决定继续等还是
    /// 显示旧图（切图时的过渡通常会显示 0.5s 旧图，这期间足够解码完）。
    bool take(int index, sf::Image& out,
              std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

    /// worker 退出前最后一次机会把队列里剩下的活儿做完。
    /// 析构里调用。再次调用是无害的（已 stop）。
    void shutdown();

private:
    void workerLoop();

    std::thread worker_;
    std::mutex mu_;
    std::condition_variable cv_;                   ///< pending_ 变化 / 缓存变化 复用一把
    std::unordered_map<int, std::string> pending_; ///< index -> path
    std::unordered_map<int, sf::Image> ready_;     ///< 解码完成、待 take
    std::atomic<bool> stop_{false};
};
