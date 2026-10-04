#include "log/bootstrap.h"

#include "config/bootstrap_config.h"
#include "config/keys.h"
#include "config/preferences.h"
#include "log/formatters/console_formatter.h"
#include "log/formatters/text_formatter.h"
#include "log/handlers/console_handler.h"
#include "log/handlers/file_handler.h"
#include "log/logger.h"
#include "log/rotation.h"

#include <iostream>
#include <memory>

namespace {

/// 日志级别默认值与旧实现一致：Info。
int minLevelFromPreferences(const std::shared_ptr<Preferences>& prefs) {
    if (!prefs)
        return static_cast<int>(LogLevel::Info);
    return prefs->getInt(ConfigKey::kLogLevel, static_cast<int>(LogLevel::Info));
}

}  // namespace

void registerLog(Container& container) {
    // ---------------- 控制台 ----------------
    container.reg<ConsoleHandler>("log_console", []() {
        auto handler = std::make_shared<ConsoleHandler>(std::cout);
        handler->setFormatter(std::make_shared<ConsoleFormatter>());
        return handler;
    });

    // ---------------- 文件 ----------------
    // 需要 bootstrap_config 才能确定 config 目录；没注册就干脆不提供文件落点，
    // 让"只有控制台"成为合法配置，而不是抛异常。
    container.reg<FileHandler>("log_file", [&container]() {
        auto cfg = container.require<BootstrapConfig>("bootstrap_config");
        auto prefs = container.tryGet<Preferences>("preferences");

        const int rotateIndex = clampLogRotationIndex(
            prefs ? prefs->getInt(ConfigKey::kLogRotate, 0) : 0);
        const int keepIndex = clampLogKeepIndex(
            prefs ? prefs->getInt(ConfigKey::kLogKeep, 1) : 1);

        auto handler = std::make_shared<FileHandler>(
            cfg->configFile("app.log").string(),
            logRotationSizeAt(rotateIndex),
            logRotationKeepAt(keepIndex));
        handler->setFormatter(std::make_shared<TextFormatter>());
        return handler;
    });

    // ---------------- 门面 ----------------
    container.reg<Logger>("logger", [&container]() {
        auto prefs = container.tryGet<Preferences>("preferences");

        auto logger = std::make_shared<Logger>(
            static_cast<LogLevel>(minLevelFromPreferences(prefs)));

        logger->addHandler(container.require<ConsoleHandler>("log_console"));

        if (container.contains("log_file"))
            logger->addHandler(container.require<FileHandler>("log_file"));

        return logger;
    });
}
