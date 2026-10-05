#pragma once
#include "config.h"

class Preferences : public Config {
public:
    explicit Preferences(const Paths& p, const ResourceManager& r)
          : Config(p, r, "preferences.conf") {}
};