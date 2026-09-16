#include "calculator.h"
#include "text_strings.h"
#include <cmath>
#include <iostream>
#include <limits>
using namespace std;

Calculator::Calculator(shared_ptr<Logger> logger)
      : logger_(logger) {}

void Calculator::run() {
    double a, b;
    char c;
    double result = 0.0;

    logger_->normal(Str::T(Str::CalcFirstNum));
    cin >> a;
    if (cin.fail()) {
        logger_->error(Str::T(Str::CalcInvalid));
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    logger_->normal(Str::T(Str::CalcSecondNum));
    cin >> b;
    if (cin.fail()) {
        logger_->error(Str::T(Str::CalcInvalid));
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    while (true) {
        logger_->normal(Str::T(Str::CalcWhatWant));
        cin >> c;
        if (cin.fail()) {
            logger_->error(Str::T(Str::CalcInvalid));
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return;
        }

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
                logger_->warn(Str::T(Str::CalcDivZero));
                return;
            }
            result = a / b;
            break;
        case '5':
            result = pow(a, b);
            if (isinf(result)) {
                logger_->warn(Str::T(Str::CalcOverflow));
                return;
            }
            break;
        default:
            logger_->error(Str::T(Str::CalcFail));
            logger_->normal(Str::T(Str::CalcRetry));
            continue;
        }

        logger_->normal(std::string(Str::T(Str::CalcResult)) + to_string(result));
        return;
    }
}