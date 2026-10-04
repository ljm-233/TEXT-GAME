#include "core/container.h"

#include <cstdlib>
#include <sstream>

#if defined(__has_include)
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define TEXTGAME_HAS_CXXABI 1
#endif
#endif

namespace {

/// 把 typeid().name() 还原成人类可读的名字。
/// 没有 cxxabi（MSVC 等）时退回原名，不为了调试输出引入平台依赖。
std::string readableTypeName(const std::type_index& info) {
#ifdef TEXTGAME_HAS_CXXABI
    int status = 0;
    char* demangled = abi::__cxa_demangle(info.name(), nullptr, nullptr, &status);
    if (status == 0 && demangled != nullptr) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
#endif
    return info.name();
}

}  // namespace

bool Container::contains(const std::string& name) const {
    return entries_.count(name) != 0;
}

bool Container::hasInstance(const std::string& name) const {
    auto it = entries_.find(name);
    return it != entries_.end() && static_cast<bool>(it->second.instance);
}

std::vector<std::string> Container::names() const {
    return order_;
}

void Container::touch(const std::string& name) {
    auto it = entries_.find(name);
    if (it == entries_.end())
        throw ContainerError(name, "未注册");

    Entry& entry = it->second;
    if (entry.singleton && !entry.instance)
        entry.instance = entry.factory();
}

std::string Container::str() const {
    size_t constructed = 0;
    for (const auto& [name, entry] : entries_) {
        (void)name;
        if (entry.instance)
            ++constructed;
    }

    std::ostringstream out;
    out << "Container(" << entries_.size() << " registered)\n";

    out << "  entries:\n";
    for (const std::string& name : order_) {
        const Entry& entry = entries_.at(name);
        out << "    " << name
            << (entry.singleton ? "  [singleton]" : "  [transient]")
            << "  -> " << readableTypeName(entry.type);

        if (entry.instance)
            out << "  (已构造)";
        out << '\n';
    }

    out << "  instances: " << constructed << "/" << entries_.size();
    return out.str();
}
