#pragma once
#include <SFML/Audio.hpp>
#include <cstddef>
#include <memory>
#include <vector>

class SoundManager {
public:
    static SoundManager& instance();

    void init();

    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const { return enabled_; }

    // ===== 音量通道（三条独立）=====
    // 实际播放音量 = master * (sfx | music)
    void setMasterVolume(float v);    // 0~1
    float masterVolume() const { return masterVolume_; }

    void setSFXVolume(float v);       // 0~1
    float sfxVolume() const { return sfxVolume_; }

    void setMusicVolume(float v);     // 0~1
    float musicVolume() const { return musicVolume_; }

    // 兼容旧名（旧代码里 setVolume 就是 SFX 音量）
    void setVolume(float v) { setSFXVolume(v); }
    float volume() const { return sfxVolume_; }
    void setBGMVolume(float v) { setMusicVolume(v); }
    float bgmVolume() const { return musicVolume_; }

    // ===== 音效 =====
    void playJump();
    void playLand();
    void playCoin();
    void playStomp();
    void playHurt();
    void playLevelComplete();
    void playGameOver();
    void playCheckpoint();
    void playClick();

    // ===== BGM =====
    void playBGM();
    void stopBGM();
    void pauseBGM();
    void resumeBGM();
    void setBGMEnabled(bool e);
    bool isBGMEnabled() const { return bgmEnabled_; }

private:
    SoundManager() = default;

    void play(const sf::SoundBuffer& buf);
    void rebuildBGM();
    void applySFXVolume();
    void applyMusicVolume();

    sf::SoundBuffer bufJump_;
    sf::SoundBuffer bufLand_;
    sf::SoundBuffer bufCoin_;
    sf::SoundBuffer bufStomp_;
    sf::SoundBuffer bufHurt_;
    sf::SoundBuffer bufComplete_;
    sf::SoundBuffer bufGameOver_;
    sf::SoundBuffer bufCheckpoint_;
    sf::SoundBuffer bufClick_;

    // 声部池的占位 buffer，必须比 pool_ 活得久
    sf::SoundBuffer silentBuf_;

    sf::SoundBuffer bufBGM_;
    std::unique_ptr<sf::Sound> bgm_;
    bool bgmEnabled_ = true;
    bool bgmPlaying_ = false;

    // 三条独立音量（0~1）
    float masterVolume_ = 1.0f;
    float sfxVolume_    = 0.6f;
    float musicVolume_  = 0.4f;

    std::vector<std::unique_ptr<sf::Sound>> pool_;
    std::size_t nextIndex_ = 0;

    bool enabled_ = true;
    bool initialized_ = false;
};