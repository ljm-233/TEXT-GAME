#pragma once
#include <memory>
#include "logging.h"

class Calculator {
public:
    explicit Calculator(std::shared_ptr<Logger> logger);
    void run();

private:
    std::shared_ptr<Logger> logger_;
};