#pragma once
#include "config.h"

// 引导配置：程序启动前读取，改了需要重启程序
// 存放日志级别、首次运行标记等
class BootstrapConfig : public Config {
public:
    explicit BootstrapConfig(const Paths& p) : Config(p, "bootstrap.conf") {}
};