#pragma once
#include <memory>
#include "logging.h"
#include "window.h"

class Game {
public:
    Game(std::shared_ptr<Window> window, std::shared_ptr<Logger> logger);
    void run();

private:
    std::shared_ptr<Window> window_;
    std::shared_ptr<Logger> logger_;
};