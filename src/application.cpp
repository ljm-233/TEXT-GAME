#include "application.h"
#include "logging.h"
#include "time_utils.h"

#include <iostream>
#include <limits>
#include <cstdlib>
#include <cmath>

using namespace std;

// 出错提示
static void fail(shared_ptr<Logger> logger) {
    logger->error(" Fail! :( ");
}

// 输入检查
static void check(shared_ptr<Logger> logger) {
    if (cin.fail()) {
        fail(logger);
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        exit(0);
    }
}

// 计算器主逻辑
static void runCalculator(shared_ptr<Logger> logger) {
    double a, b;
    char c;
    double result = 0.0;

    /*getime();*/   // 打印当前时间（可选）

    logger->normal("请输入第一个数字·Enter 1st number:");
    cin >> a;
    check(logger);
    logger->info("输入成功");

    logger->normal("请输入第二个数字·Enter 2nd number:");
    cin >> b;
    check(logger);
    logger->info("输入成功");

    while (true) {
        logger->normal("你想要什么·What do you want?\n1+ 2- 3x 4/ 5幂函数\n");
        cin >> c;
        logger->info("输入成功");

        switch (c) {
            case '1':
                result = a + b;
                break;
            case '2':
                result = a - b;
                break;
            case '3':
                result = a * b;
                break;
            case '4':
                if (fabs(b) < 1e-12) {
                    logger->warn("不能除以零·Cannot divide by zero!");
                    return;
                }
                result = a / b;
                break;
            case '5':
                result = pow(a, b);
                if (isinf(result)) {
                    logger->warn("结果过大，无法显示!");
                    return;
                }
                break;
            default:
                fail(logger);
                logger->normal("[再试·Retry]");
                continue;
        }

        logger->normal("结果·End Number: " + to_string(result));
        return;
    }
}

// ---------- Application ----------

Application::Application() {
    // 注册 Logger：全局单例
    container_.registerType<Logger>([]() {
        return make_shared<Logger>("app.log");
    });

    // 以后有新类，继续在这里注册
    // container_.registerType<Game>([this]() {
    //     auto logger = container_.resolve<Logger>();
    //     return make_shared<Game>(logger);
    // });
}

void Application::run() {
    auto logger = container_.resolve<Logger>();

    logger->info("程序启动");
    logger->debug("x = " + to_string(42));
    logger->warn("磁盘空间不足");
    logger->error("打开文件失败");
    logger->trace("TEXT");

    runCalculator(logger);

    logger->info("程序结束");
}