## Prerequisite
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