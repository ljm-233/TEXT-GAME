#include "sound_manager.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace {

constexpr unsigned kSampleRate = 44100;
constexpr float    kPi = 3.14159265358979323846f;

// 生成"扫频"音频：从 startFreq 平滑变到 endFreq
std::vector<std::int16_t> generateSweep(float startFreq, float endFreq,
                                        float duration, float volume) {
    std::size_t count = static_cast<std::size_t>(kSampleRate * duration);
    std::vector<std::int16_t> samples(count);

    float phase = 0.f;
    for (std::size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / kSampleRate;
        float progress = t / duration;
        float freq = startFreq + (endFreq - startFreq) * progress;
        phase += 2.f * kPi * freq / kSampleRate;

        float env = 1.f;
        if (progress < 0.05f) env = progress / 0.05f;
        env *= (1.f - progress * 0.85f);

        samples[i] = static_cast<std::int16_t>(
            std::sin(phase) * env * volume * 32767.f);
    }
    return samples;
}

// 生成琶音
std::vector<std::int16_t> generateArpeggio(const std::vector<float>& freqs,
                                           float noteDuration, float volume) {
    std::vector<std::int16_t> result;
    for (float f : freqs) {
        auto note = generateSweep(f, f, noteDuration, volume);
        result.insert(result.end(), note.begin(), note.end());
    }
    return result;
}

// SFML 3：loadFromSamples 需要第 5 个参数"声道映射"
bool loadSamples(sf::SoundBuffer& buf,
                 const std::vector<std::int16_t>& samples) {
    return buf.loadFromSamples(
        samples.data(),
        samples.size(),
        1,
        kSampleRate,
        {sf::SoundChannel::Mono});
}

} // namespace

SoundManager& SoundManager::instance() {
    static SoundManager inst;
    return inst;
}

void SoundManager::init() {
    if (initialized_) return;
    initialized_ = true;

    // ===== 生成音效 =====
    loadSamples(bufJump_, generateSweep(200.f, 700.f, 0.12f, 0.5f));
    loadSamples(bufLand_, generateSweep(200.f, 100.f, 0.10f, 0.5f));
    loadSamples(bufCoin_, generateSweep(1000.f, 1400.f, 0.10f, 0.4f));
    loadSamples(bufStomp_, generateSweep(500.f, 100.f, 0.18f, 0.6f));
    loadSamples(bufHurt_, generateSweep(300.f, 200.f, 0.30f, 0.5f));

    loadSamples(bufComplete_,
        generateArpeggio({523.25f, 659.25f, 783.99f, 1046.50f},
                         0.15f, 0.5f));

    loadSamples(bufGameOver_,
        generateArpeggio({392.00f, 311.13f, 261.63f},
                         0.25f, 0.5f));

    // ===== 声部池：用静音缓冲初始化 =====
    std::vector<std::int16_t> silent(64, 0);
    sf::SoundBuffer silentBuf;
    // ⭐ 显式忽略返回值，消 warning
    (void)silentBuf.loadFromSamples(
        silent.data(),
        silent.size(),
        1,
        kSampleRate,
        {sf::SoundChannel::Mono});

    pool_.clear();
    for (int i = 0; i < 16; ++i) {
        pool_.push_back(std::make_unique<sf::Sound>(silentBuf));
    }
}

void SoundManager::setVolume(float v) {
    volume_ = std::clamp(v, 0.f, 1.f);
    for (auto& s : pool_) s->setVolume(volume_ * 100.f);
}

void SoundManager::play(const sf::SoundBuffer& buf) {
    if (!enabled_ || !initialized_ || pool_.empty()) return;

    auto& slot = pool_[nextIndex_];
    nextIndex_ = (nextIndex_ + 1) % pool_.size();

    slot->stop();
    slot->setBuffer(buf);
    slot->setVolume(volume_ * 100.f);
    slot->play();
}

void SoundManager::playJump()          { play(bufJump_); }
void SoundManager::playLand()          { play(bufLand_); }
void SoundManager::playCoin()          { play(bufCoin_); }
void SoundManager::playStomp()         { play(bufStomp_); }
void SoundManager::playHurt()          { play(bufHurt_); }
void SoundManager::playLevelComplete() { play(bufComplete_); }
void SoundManager::playGameOver()      { play(bufGameOver_); }