#include "sound_manager.h"
#include "game_constants.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

constexpr unsigned kSampleRate = GameConst::kSoundSampleRate;
constexpr float kPi = 3.14159265358979323846f;

std::vector<std::int16_t> generateSweep(float startFreq, float endFreq, float duration,
                                        float volume) {
    std::size_t count = static_cast<std::size_t>(kSampleRate * duration);
    std::vector<std::int16_t> samples(count);

    float phase = 0.f;
    for (std::size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / kSampleRate;
        float progress = t / duration;
        float freq = startFreq + (endFreq - startFreq) * progress;
        phase += 2.f * kPi * freq / kSampleRate;

        float env = 1.f;
        if (progress < 0.05f)
            env = progress / 0.05f;
        env *= (1.f - progress * 0.85f);

        samples[i] = static_cast<std::int16_t>(std::sin(phase) * env * volume * 32767.f);
    }
    return samples;
}

// MC 木按钮风格：三个非谐波频率各自独立衰减（高频衰减更快）
// 音色从"亮"变"暗"，但没有音调变化——不会听成"得"
std::vector<std::int16_t> generateClick(float volume) {
    constexpr float duration = 0.08f;   // 80ms

    // 三个非谐波频率（比例约 1 : 1.36 : 1.82）
    // 非整数比 = 木质共鸣（不是电子音的谐波）
    constexpr float f1 = 280.f;   // 基音
    constexpr float f2 = 380.f;   // 中
    constexpr float f3 = 510.f;   // 亮

    // 衰减率：越高频衰减越快（物理正确）
    constexpr float d1 = 30.f;    // 低频：保持最久
    constexpr float d2 = 55.f;
    constexpr float d3 = 90.f;    // 高频：最先消失

    constexpr float noiseDur = 0.004f;   // 前 4ms 噪声瞬态

    std::size_t count = static_cast<std::size_t>(kSampleRate * duration);
    std::vector<std::int16_t> samples(count);

    // 简易噪声
    std::uint32_t seed = 0x12345678u;
    auto noise = [&]() {
        seed = seed * 1103515245u + 12345u;
        return static_cast<float>((seed >> 16) & 0x7FFF) / 16384.f - 1.f;
    };

    for (std::size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / kSampleRate;

        // 三个频率叠加，各自独立指数衰减
        float body = 0.5f * std::sin(2.f * kPi * f1 * t) * std::exp(-d1 * t)
                   + 0.3f * std::sin(2.f * kPi * f2 * t) * std::exp(-d2 * t)
                   + 0.2f * std::sin(2.f * kPi * f3 * t) * std::exp(-d3 * t);

        // 噪声瞬态：让起音有"咔"的质感
        if (t < noiseDur) {
            float nEnv = 1.f - t / noiseDur;
            body = 0.7f * body + 0.6f * noise() * nEnv;
        }

        samples[i] = static_cast<std::int16_t>(body * volume * 32767.f);
    }
    return samples;
}

std::vector<std::int16_t> generateArpeggio(const std::vector<float>& freqs,
                                           float noteDuration, float volume) {
    std::vector<std::int16_t> result;
    for (float f : freqs) {
        auto note = generateSweep(f, f, noteDuration, volume);
        result.insert(result.end(), note.begin(), note.end());
    }
    return result;
}

// BGM：一段 8 秒的循环小调（C 大调五声音阶）
std::vector<std::int16_t> generateBGM() {
    const float chordRoots[4][3] = {
        {261.63f, 329.63f, 392.00f}, // C
        {220.00f, 261.63f, 329.63f}, // Am
        {174.61f, 220.00f, 261.63f}, // F
        {196.00f, 246.94f, 293.66f}, // G
    };

    constexpr float chordDuration = 2.0f;
    std::size_t chordSamples = static_cast<std::size_t>(kSampleRate * chordDuration);

    std::vector<std::int16_t> result(chordSamples * 4, 0);

    for (int c = 0; c < 4; ++c) {
        for (std::size_t i = 0; i < chordSamples; ++i) {
            float t = static_cast<float>(i) / kSampleRate;
            float progress = t / chordDuration;

            float env = 1.f;
            if (progress < 0.1f)
                env = progress / 0.1f;
            else if (progress > 0.85f)
                env = (1.f - progress) / 0.15f;

            float sample = 0.f;
            for (int n = 0; n < 3; ++n) {
                sample += std::sin(2.f * kPi * chordRoots[c][n] * t);
            }
            sample /= 3.f;

            float bass = std::sin(2.f * kPi * (chordRoots[c][0] * 0.5f) * t);
            sample = sample * 0.6f + bass * 0.4f;

            result[c * chordSamples + i] =
                static_cast<std::int16_t>(sample * env * 0.35f * 32767.f);
        }
    }
    return result;
}

bool loadSamples(sf::SoundBuffer& buf, const std::vector<std::int16_t>& samples) {
    return buf.loadFromSamples(samples.data(), samples.size(), 1, kSampleRate,
                               {sf::SoundChannel::Mono});
}

} // namespace

SoundManager& SoundManager::instance() {
    static SoundManager inst;
    return inst;
}

void SoundManager::init() {
    if (initialized_)
        return;
    initialized_ = true;

    loadSamples(bufJump_, generateSweep(200.f, 700.f, 0.12f, 0.5f));
    loadSamples(bufLand_, generateSweep(200.f, 100.f, 0.10f, 0.5f));
    loadSamples(bufCoin_, generateSweep(1000.f, 1400.f, 0.10f, 0.4f));
    loadSamples(bufStomp_, generateSweep(500.f, 100.f, 0.18f, 0.6f));
    loadSamples(bufHurt_, generateSweep(300.f, 200.f, 0.30f, 0.5f));

    loadSamples(bufComplete_,
                generateArpeggio({523.25f, 659.25f, 783.99f, 1046.50f}, 0.15f, 0.5f));
    loadSamples(bufGameOver_, generateArpeggio({392.00f, 311.13f, 261.63f}, 0.25f, 0.5f));
    loadSamples(bufCheckpoint_, generateArpeggio({880.f, 1174.66f}, 0.08f, 0.5f));
    loadSamples(bufClick_, generateClick(0.9f));

    loadSamples(bufBGM_, generateBGM());
    bgm_ = std::make_unique<sf::Sound>(bufBGM_);
    bgm_->setLooping(true);

    std::vector<std::int16_t> silent(64, 0);
    (void)silentBuf_.loadFromSamples(silent.data(), silent.size(), 1, kSampleRate,
                                     {sf::SoundChannel::Mono});

    pool_.clear();
    for (int i = 0; i < 16; ++i)
        pool_.push_back(std::make_unique<sf::Sound>(silentBuf_));

    // 应用初始音量
    applySFXVolume();
    applyMusicVolume();
}

void SoundManager::setMasterVolume(float v) {
    masterVolume_ = std::clamp(v, 0.f, 1.f);
    applySFXVolume();
    applyMusicVolume();
}

void SoundManager::setSFXVolume(float v) {
    sfxVolume_ = std::clamp(v, 0.f, 1.f);
    applySFXVolume();
}

void SoundManager::setMusicVolume(float v) {
    musicVolume_ = std::clamp(v, 0.f, 1.f);
    applyMusicVolume();
}

void SoundManager::applySFXVolume() {
    float v = masterVolume_ * sfxVolume_ * 100.f;
    for (auto& s : pool_)
        s->setVolume(v);
}

void SoundManager::applyMusicVolume() {
    if (bgm_)
        bgm_->setVolume(masterVolume_ * musicVolume_ * 100.f);
}

void SoundManager::play(const sf::SoundBuffer& buf) {
    if (!enabled_ || !initialized_ || pool_.empty())
        return;
    auto& slot = pool_[nextIndex_];
    nextIndex_ = (nextIndex_ + 1) % pool_.size();
    slot->stop();
    slot->setBuffer(buf);
    slot->setVolume(masterVolume_ * sfxVolume_ * 100.f);
    slot->play();
}

void SoundManager::playJump() {
    play(bufJump_);
}
void SoundManager::playLand() {
    play(bufLand_);
}
void SoundManager::playCoin() {
    play(bufCoin_);
}
void SoundManager::playStomp() {
    play(bufStomp_);
}
void SoundManager::playHurt() {
    play(bufHurt_);
}
void SoundManager::playLevelComplete() {
    play(bufComplete_);
}
void SoundManager::playGameOver() {
    play(bufGameOver_);
}
void SoundManager::playCheckpoint() {
    play(bufCheckpoint_);
}
void SoundManager::playClick() {
    play(bufClick_);
}

// ===== BGM =====

void SoundManager::setBGMEnabled(bool e) {
    bgmEnabled_ = e;
    if (!bgmEnabled_) {
        stopBGM();
    }
}

void SoundManager::playBGM() {
    if (!enabled_ || !bgmEnabled_ || !bgm_)
        return;
    if (bgm_->getStatus() == sf::Sound::Status::Playing)
        return;
    bgm_->play();
    bgmPlaying_ = true;
}

void SoundManager::stopBGM() {
    if (!bgm_)
        return;
    bgm_->stop();
    bgmPlaying_ = false;
}

void SoundManager::pauseBGM() {
    if (!bgm_)
        return;
    if (bgm_->getStatus() == sf::Sound::Status::Playing) {
        bgm_->pause();
    }
}

void SoundManager::resumeBGM() {
    if (!enabled_ || !bgmEnabled_ || !bgm_)
        return;
    if (bgmPlaying_ && bgm_->getStatus() == sf::Sound::Status::Paused) {
        bgm_->play();
    }
}

void SoundManager::rebuildBGM() {
    // 占位：如果以后要换 BGM，可以重新生成
}