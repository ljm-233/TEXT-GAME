#pragma once
#include "config.h"

// 运行时配置：程序自己读写，记录上次状态
// 存放上次打开的存档、上次窗口位置等
class RuntimeConfig : public Config {
public:
    explicit RuntimeConfig(const Paths& p) : Config(p, "runtime.conf") {}
};