#include "calculator.h"
#include <iostream>
#include <limits>
#include <cmath>
using namespace std;

Calculator::Calculator(shared_ptr<Logger> logger) : logger_(logger) {}

void Calculator::run() {
    double a, b;
    char c;
    double result = 0.0;

    logger_->normal("请输入第一个数字·Enter 1st number:");
    cin >> a;
    if (cin.fail()) {
        logger_->error("输入无效");
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    logger_->normal("请输入第二个数字·Enter 2nd number:");
    cin >> b;
    if (cin.fail()) {
        logger_->error("输入无效");
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    while (true) {
        logger_->normal("你想要什么·What do you want?\n1+ 2- 3x 4/ 5幂函数\n");
        cin >> c;

        switch (c) {
            case '1': result = a + b; break;
            case '2': result = a - b; break;
            case '3': result = a * b; break;
            case '4':
                if (fabs(b) < 1e-12) {
                    logger_->warn("不能除以零·Cannot divide by zero!");
                    return;
                }
                result = a / b;
                break;
            case '5':
                result = pow(a, b);
                if (isinf(result)) {
                    logger_->warn("结果过大，无法显示!");
                    return;
                }
                break;
            default:
                logger_->error(" Fail! :( ");
                logger_->normal("[再试·Retry]");
                continue;
        }

        logger_->normal("结果·End Number: " + to_string(result));
        return;
    }
}