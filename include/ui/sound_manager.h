#pragma once
#include <SFML/Audio.hpp>
#include <cstddef>
#include <memory>
#include <vector>

// 全局音效管理器：音效用代码生成，不依赖外部文件
class SoundManager {
public:
    static SoundManager& instance();

    void init();

    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const { return enabled_; }

    void setVolume(float v);
    float volume() const { return volume_; }

    void playJump();
    void playLand();
    void playCoin();
    void playStomp();
    void playHurt();
    void playLevelComplete();
    void playGameOver();

private:
    SoundManager() = default;

    void play(const sf::SoundBuffer& buf);

    sf::SoundBuffer bufJump_;
    sf::SoundBuffer bufLand_;
    sf::SoundBuffer bufCoin_;
    sf::SoundBuffer bufStomp_;
    sf::SoundBuffer bufHurt_;
    sf::SoundBuffer bufComplete_;
    sf::SoundBuffer bufGameOver_;

    std::vector<std::unique_ptr<sf::Sound>> pool_;
    std::size_t nextIndex_ = 0;

    bool enabled_ = true;
    float volume_ = 0.6f;
    bool initialized_ = false;
};