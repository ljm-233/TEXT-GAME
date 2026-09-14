#pragma once
#include "logging.h"
#include <memory>

class Calculator {
public:
    explicit Calculator(std::shared_ptr<Logger> logger);
    void run();

private:
    std::shared_ptr<Logger> logger_;
};