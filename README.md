![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/silver2dream/andromeda-linux-cpp/ubuntu.yml?logo=github)
[![GitHub License](https://img.shields.io/github/license/silver2dream/andromeda-linux-cpp?logo=github)][license]

[license]: LICENSE

## Prerequisite

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/2c1068c628944bc8adf316062f99d8c2)](https://app.codacy.com/gh/silver2dream/andromeda-linux-cpp?utm_source=github.com&utm_medium=referral&utm_content=silver2dream/andromeda-linux-cpp&utm_campaign=Badge_Grade)

* makefile
* gcc
* g++
* protobuf-compiler

## Testing Tools
* valgrind (memeory-leak check tool)
``` cmd
valgrind --tool = memcheck --leak-check=full ./andromeda
```

## Conventional Commits
```markdown
feat：新功能（feature）
fix：修復 bug
docs：文檔改動
style：不影響程式碼意義的改動（空格、格式化、缺分號等）
refactor：重構（即不是新增功能，也不是修改 bug 的程式碼變更）
perf：改善效能的程式碼更改
test：增加缺少的測試或更正現有的測試
chore：對建置過程或輔助工具和函式庫的變更（不影響原始檔、測試）
```