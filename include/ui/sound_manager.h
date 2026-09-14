#pragma once
#include <SFML/Audio.hpp>
#include <memory>
#include <vector>
#include <cstddef>

class SoundManager {
public:
    static SoundManager& instance();

    void init();

    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const  { return enabled_; }

    void setVolume(float v);
    float volume() const { return volume_; }

    // ===== 音效 =====
    void playJump();
    void playLand();
    void playCoin();
    void playStomp();
    void playHurt();
    void playLevelComplete();
    void playGameOver();
    void playCheckpoint();

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

    sf::SoundBuffer bufJump_;
    sf::SoundBuffer bufLand_;
    sf::SoundBuffer bufCoin_;
    sf::SoundBuffer bufStomp_;
    sf::SoundBuffer bufHurt_;
    sf::SoundBuffer bufComplete_;
    sf::SoundBuffer bufGameOver_;
    sf::SoundBuffer bufCheckpoint_;

    sf::SoundBuffer bufBGM_;
    std::unique_ptr<sf::Sound> bgm_;
    bool  bgmEnabled_ = true;
    bool  bgmPlaying_ = false;

    std::vector<std::unique_ptr<sf::Sound>> pool_;
    std::size_t nextIndex_ = 0;

    bool  enabled_ = true;
    float volume_  = 0.6f;
    bool  initialized_ = false;
};