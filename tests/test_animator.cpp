#include "doctest.h"
#include "animator.h"

// 造一个简单的 Animator，不需要 sf::Sprite
TEST_CASE("Animator - 添加和播放") {
    Animator anim;

    Animator::Clip clip;
    clip.frames = {
        sf::IntRect({0, 0}, {32, 32}),
        sf::IntRect({32, 0}, {32, 32}),
        sf::IntRect({64, 0}, {32, 32}),
        sf::IntRect({96, 0}, {32, 32}),
    };
    clip.fps = 10.f;
    clip.loop = true;

    anim.addClip("walk", clip);
    anim.play("walk");

    CHECK(anim.currentClip() == "walk");
}

TEST_CASE("Animator - 切换剪辑") {
    Animator anim;

    Animator::Clip idle;
    idle.frames = {sf::IntRect({0, 0}, {32, 32})};
    idle.fps = 4.f;
    idle.loop = true;

    Animator::Clip walk;
    walk.frames = {
        sf::IntRect({0, 0}, {32, 32}),
        sf::IntRect({32, 0}, {32, 32}),
    };
    walk.fps = 10.f;
    walk.loop = true;

    anim.addClip("idle", idle);
    anim.addClip("walk", walk);

    anim.play("idle");
    CHECK(anim.currentClip() == "idle");

    anim.play("walk");
    CHECK(anim.currentClip() == "walk");

    // 重复播放同一个剪辑，不应该重置
    anim.play("walk");
    CHECK(anim.currentClip() == "walk");
}

TEST_CASE("Animator - 播放不存在的剪辑不崩") {
    Animator anim;
    anim.play("nonexistent");   // 不应该崩溃
    CHECK(anim.currentClip() == "");
}

TEST_CASE("Animator - update 推进时间") {
    Animator anim;
    Animator::Clip clip;
    clip.frames = {
        sf::IntRect({0, 0}, {32, 32}),
        sf::IntRect({32, 0}, {32, 32}),
    };
    clip.fps = 10.f;   // 每帧 0.1 秒
    clip.loop = true;
    anim.addClip("test", clip);
    anim.play("test");

    // 调用 update 不应该崩溃
    anim.update(0.05f);
    anim.update(0.05f);
    anim.update(0.05f);
    CHECK(anim.currentClip() == "test");
}