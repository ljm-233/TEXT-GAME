#!/usr/bin/env python3
"""
扫描项目里的硬编码中文（C++ 字符串字面量中的中文）。

用法:
    python3 scripts/check_hardcoded.py

退出码:
    0 = 无硬编码中文
    1 = 发现硬编码中文

检测规则:
    - 只看 C++ 字符串字面量里的中文（"..." 里的）
    - 行内注释里的中文忽略
    - 前面 30 字符内有 Str::T( 的视为已翻译，标 OK
    - 跳过 text_strings.h（字符串定义中心）
    - 跳过 logger_->info/warn/error 等开发者日志
"""

import re
import sys
from pathlib import Path

CN_PATTERN = re.compile(r'[\u4e00-\u9fff]')
STR_LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')
INLINE_COMMENT = re.compile(r'//.*$')

SKIP_FILES = {
    'text_strings.h',
    'utf8.cpp',
    'doctest.h',
}

SKIP_LINE_PATTERNS = [
    re.compile(r'^\s*#'),
    re.compile(r'logger_->(info|warn|error|debug|trace|normal)'),
    re.compile(r'^\s*//'),
    re.compile(r'^\s*\*'),
    re.compile(r'^\s*/\*'),
]


def scan_line(line):
    """返回这行里 (is_hardcoded, content) 的列表"""
    results = []

    for pat in SKIP_LINE_PATTERNS:
        if pat.search(line):
            return results

    code = INLINE_COMMENT.sub('', line)

    for m in STR_LITERAL.finditer(code):
        content = m.group(1)
        if not CN_PATTERN.search(content):
            continue

        before = code[max(0, m.start() - 30):m.start()]
        is_translated = 'Str::T(' in before

        results.append((not is_translated, content))

    return results


def main():
    root = Path(__file__).resolve().parent.parent
    found = []
    total_cn = 0

    for base in ('src', 'include'):
        for path in (root / base).rglob('*'):
            if not path.is_file():
                continue
            if path.suffix not in ('.cpp', '.h'):
                continue
            if path.name in SKIP_FILES:
                continue

            rel = path.relative_to(root)
            try:
                with open(path, 'r', encoding='utf-8') as f:
                    for lineno, line in enumerate(f, 1):
                        for is_hard, content in scan_line(line):
                            total_cn += 1
                            if is_hard:
                                found.append((rel, lineno, content, line.strip()))
            except UnicodeDecodeError:
                continue

    print("=== 中文硬编码检测 ===")
    print("扫描目录: src/  include/")
    print(f"中文字符串字面量总数: {total_cn}")
    print(f"疑似硬编码: {len(found)}")
    print()

    if not found:
        print("[OK] 未发现硬编码中文")
        return 0

    print("--- 详情 ---")
    for rel, lineno, content, line in found:
        print(f"{rel}:{lineno}")
        print(f"    字面量: \"{content}\"")
        print(f"    上下文: {line}")
        print()

    print(f"共 {len(found)} 处需要检查")
    print()
    print("提示:")
    print("  1. 如果该字符串已经通过 Str::T(Str::XXX) 使用，忽略")
    print("  2. 如果是纯符号/数字/英文标识，忽略")
    print("  3. 如果是新增的 UI 文字，加入 text_strings.h 并走 Str::T()")
    return 1


if __name__ == '__main__':
    sys.exit(main())