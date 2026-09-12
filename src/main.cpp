#include "application.h"
#include <iostream>

int main() {
    try {
        // 对应 Python 的：app = Application.instance()
        Application& app = Application::instance();
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "程序异常退出: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}