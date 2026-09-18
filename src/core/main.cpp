#include "application.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    // 控制台 UTF-8（解决中文日志乱码）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try {
        Application& app = Application::instance();
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "程序异常退出: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}