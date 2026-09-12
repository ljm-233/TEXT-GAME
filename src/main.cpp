#include "application.h"

int main() {
    // 对应 Python 的：app = Application.instance()
    Application& app = Application::instance();
    app.run();
    return 0;
}