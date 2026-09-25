#include "application.h"
#include <cstdlib>
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

    int code = 0;
    try {
        Application& app = Application::instance();
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "程序异常退出: " << e.what() << std::endl;
        code = 1;
    }

    // ⭐ 直接终止进程，跳过所有静态/单例析构
    //    规避 SFML 3.1.0 HarfBuzz 死锁
    std::_Exit(code);
}