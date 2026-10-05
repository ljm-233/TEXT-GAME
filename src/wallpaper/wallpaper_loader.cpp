#include "wallpaper/wallpaper_loader.h"

#include <utility>

WallpaperLoader::WallpaperLoader() {
    worker_ = std::thread([this] { workerLoop(); });
}

WallpaperLoader::~WallpaperLoader() {
    shutdown();
    if (worker_.joinable())
        worker_.join();
}

void WallpaperLoader::shutdown() {
    {
        std::lock_guard<std::mutex> lk(mu_);
        stop_.store(true, std::memory_order_release);
    }
    cv_.notify_all();
}

bool WallpaperLoader::prime(int index, const WallpaperInfo& info) {
    if (index < 0 || info.path.empty())
        return false;

    sf::Image img;
    if (!img.loadFromFile(info.path.string()))
        return false;

    std::lock_guard<std::mutex> lk(mu_);
    ready_[index] = std::move(img);
    // 这张已经被取走的诉求"已达成"，从 pending 里清掉（如果之前还排队的话）
    pending_.erase(index);
    cv_.notify_all();
    return true;
}

void WallpaperLoader::request(int index, const WallpaperInfo& info) {
    if (index < 0 || info.path.empty())
        return;
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (stop_.load(std::memory_order_acquire))
            return;
        // 已经 ready / 已经在排队就不再塞
        if (ready_.count(index) || pending_.count(index))
            return;
        pending_.emplace(index, info.path.string());
    }
    cv_.notify_all();
}

bool WallpaperLoader::take(int index, sf::Image& out, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lk(mu_);

    // 已经 ready 就直接拿
    auto it = ready_.find(index);
    if (it != ready_.end()) {
        out = std::move(it->second);
        ready_.erase(it);
        return true;
    }

    // 不在 pending 里的话，调用方想拿的图根本没在解码 —— 立即失败
    //（不要无限等，那样 take 在错误用法下会卡死调用方）
    if (!pending_.count(index))
        return false;

    // 等 worker 解码完这一张（wait_for 的返回值不重要：超时/被 notify 都
    // 落到下面统一检查 ready 与 stop）
    cv_.wait_for(lk, timeout, [this, index] {
        return ready_.count(index) != 0 || stop_.load(std::memory_order_acquire);
    });
    if (stop_.load(std::memory_order_acquire) && !ready_.count(index))
        return false;

    it = ready_.find(index);
    if (it == ready_.end())
        return false;
    out = std::move(it->second);
    ready_.erase(it);
    return true;
}

void WallpaperLoader::workerLoop() {
    while (true) {
        std::pair<int, std::string> job{-1, {}};
        {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [this] {
                return stop_.load(std::memory_order_acquire) || !pending_.empty();
            });
            if (stop_.load(std::memory_order_acquire) && pending_.empty())
                return;
            if (pending_.empty())
                continue;

            // 取第一个（不保证公平：顺序不重要 —— 反正 worker 很快会把 pending
            // 清空，公平性不会影响首屏）。
            auto it = pending_.begin();
            job = *it;
            pending_.erase(it);
        }

        sf::Image img;
        if (img.loadFromFile(job.second)) {
            std::lock_guard<std::mutex> lk(mu_);
            // prime 也许在这期间把它变成 ready 了 —— pending 端已经被清掉，
            // 这里覆盖是无害的（内容相同）
            ready_[job.first] = std::move(img);
        }
        // 解码失败就让 ready 里没有这一项 —— take 会超时返回 false，
        // 调用方会回退到旧图或默认。
        cv_.notify_all();
    }
}
