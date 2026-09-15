#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>

class Animator {
public:
    struct Clip {
        std::vector<sf::IntRect> frames;
        float fps = 10.f;
        bool loop = true;
    };

    void addClip(const std::string& name, const Clip& clip);
    void play(const std::string& name);

    void update(float dt);
    void applyTo(sf::Sprite& sprite) const;

    const std::string& currentClip() const { return currentClip_; }

private:
    std::map<std::string, Clip> clips_;
    std::string currentClip_;
    int currentFrame_ = 0;
    float timer_ = 0.f;
};