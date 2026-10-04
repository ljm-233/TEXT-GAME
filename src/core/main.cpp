#include "core/application.h"
#include "core/bootstrap.h"
#include "config/bootstrap.h"
#include "log/bootstrap.h"
#include "scene/bootstrap.h"

#include <cstdlib>
#include <exception>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

// 入口只做三件事：装配、跑生命周期、把退出码交出去。
//
// 装配清单写在这里而不是藏在 Application 里，是为了让"这个程序由哪些层
// 组成"一眼可见；每层自己提供 registerXxx/wireXxx，入口只负责排序。
int main() {
#ifdef _WIN32
    // 控制台 UTF-8（解决中文日志乱码）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    int code = 0;
    try {
        Application& app = Application::instance();

        app.boot([](Application& a) {
            // 装配顺序 = 分层顺序，底层在前。
            // 这一步只往容器里放工厂、不构造实例，所以这里排的是
            // "读起来谁在下"，而不是真正的构造顺序（构造由解析时机决定）。
            registerConfig(a.container());  // 配置层：bootstrap / preferences / runtime
            registerLog(a.container());     // 日志层：console + file 两个落点
            registerCore(a.container());    // 核心层：路径 / 窗口 / 字体 / 存档 / 游戏本体
            registerScenes(a.container());  // 场景层：登记 8 个场景工厂
            wireCore(a);                    // 挂核心层生命周期钩子（此处只注册，不执行）
        });

        code = app.exec();
    } catch (const std::exception& e) {
        std::cerr << "程序异常退出: " << e.what() << std::endl;
        code = 1;
    }

    // ⭐ 直接终止进程，跳过所有静态/单例析构
    //    规避 SFML 3.1.0 HarfBuzz 死锁
    std::_Exit(code);
}
