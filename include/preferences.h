#pragma once
#include "config.h"

class Preferences : public Config {
public:
    explicit Preferences(const Paths& p) : Config(p, "preferences.conf") {}
};