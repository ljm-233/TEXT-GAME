#!/bin/bash
set -e

cd "$(dirname "$0")/.."

# 配置（如果还没配）
if [ ! -d build ]; then
    mkdir build
    cd build
    cmake .. -DBUILD_TESTS=ON
    cd ..
fi

# 编译
cmake --build build -j

# 跑测试
./build/tests/unit_tests "$@"