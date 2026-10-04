#include "config/bootstrap.h"

#include "config/bootstrap_config.h"
#include "config/preferences.h"
#include "config/runtime_config.h"
#include "core/paths.h"

#include <memory>

void registerConfig(Container& container) {
    container.reg<BootstrapConfig>("bootstrap_config", [&container]() {
        return std::make_shared<BootstrapConfig>(*container.require<Paths>("paths"));
    });

    container.reg<Preferences>("preferences", [&container]() {
        return std::make_shared<Preferences>(*container.require<Paths>("paths"));
    });

    container.reg<RuntimeConfig>("runtime_config", [&container]() {
        return std::make_shared<RuntimeConfig>(*container.require<Paths>("paths"));
    });
}
