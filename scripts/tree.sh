#!/usr/bin/env bash
# 显示项目结构，忽略构建产物和缓存
tree -a -I 'build|.cache|.git|node_modules|target|*.o|*.out' "${@:-.}"
