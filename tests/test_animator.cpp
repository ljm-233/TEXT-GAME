#include "doctest.h"
#include "animator.h"

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
    CHECK(anim.currentFrame() == 0);
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
    CHECK(anim.currentFrame() == 0);
}

TEST_CASE("Animator - 播放不存在的剪辑不崩") {
    Animator anim;
    anim.play("nonexistent");
    CHECK(anim.currentClip() == "");
}

TEST_CASE("Animator - 帧推进") {
    Animator anim;
    Animator::Clip clip;
    clip.frames = {
        sf::IntRect({0, 0}, {32, 32}),
        sf::IntRect({32, 0}, {32, 32}),
        sf::IntRect({64, 0}, {32, 32}),
    };
    clip.fps = 10.f;   // 每帧 0.1 秒
    clip.loop = true;
    anim.addClip("test", clip);
    anim.play("test");

    CHECK(anim.currentFrame() == 0);

    anim.update(0.1f);
    CHECK(anim.currentFrame() == 1);

    anim.update(0.1f);
    CHECK(anim.currentFrame() == 2);

    // loop 回绕
    anim.update(0.1f);
    CHECK(anim.currentFrame() == 0);
}

TEST_CASE("Animator - 非 loop 停在最后一帧") {
    Animator anim;
    Animator::Clip clip;
    clip.frames = {
        sf::IntRect({0, 0}, {32, 32}),
        sf::IntRect({32, 0}, {32, 32}),
    };
    clip.fps = 10.f;
    clip.loop = false;
    anim.addClip("once", clip);
    anim.play("once");

    CHECK(anim.currentFrame() == 0);

    anim.update(0.1f);
    CHECK(anim.currentFrame() == 1);

    // 再推进很多时间，应该停在最后一帧
    anim.update(1.0f);
    CHECK(anim.currentFrame() == 1);
}

TEST_CASE("Animator - 单帧剪辑") {
    Animator anim;
    Animator::Clip clip;
    clip.frames = {sf::IntRect({0, 0}, {32, 32})};
    clip.fps = 10.f;
    clip.loop = true;
    anim.addClip("single", clip);
    anim.play("single");

    CHECK(anim.currentFrame() == 0);
    anim.update(1.0f);
    CHECK(anim.currentFrame() == 0);
}

TEST_CASE("Animator - 大 dt 跨多帧") {
    Animator anim;
    Animator::Clip clip;
    clip.frames = {
        sf::IntRect({0, 0}, {32, 32}),
        sf::IntRect({32, 0}, {32, 32}),
        sf::IntRect({64, 0}, {32, 32}),
        sf::IntRect({96, 0}, {32, 32}),
    };
    clip.fps = 10.f;   // 每帧 0.1 秒
    clip.loop = true;
    anim.addClip("test", clip);
    anim.play("test");

    // 0.35 秒 = 3.5 帧 → 应该到第 3 帧
    anim.update(0.35f);
    CHECK(anim.currentFrame() == 3);
}