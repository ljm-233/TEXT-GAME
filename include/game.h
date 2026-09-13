#pragma once
#include <memory>
#include "logging.h"
#include "window.h"
#include "background.h"

class Game {
public:
    Game(std::shared_ptr<Window>     window,
         std::shared_ptr<Logger>     logger,
         std::shared_ptr<Background> background);

    void run();

private:
    std::shared_ptr<Window>     window_;
    std::shared_ptr<Logger>     logger_;
    std::shared_ptr<Background> background_;
};