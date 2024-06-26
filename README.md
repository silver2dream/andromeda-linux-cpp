![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/silver2dream/andromeda-linux-cpp/ubuntu.yml?logo=github)
[![GitHub License](https://img.shields.io/github/license/silver2dream/andromeda-linux-cpp?logo=github)][license]
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/b0e6a6bf3b59467e9f18379dc87d9ee6)](https://app.codacy.com/gh/silver2dream/andromeda-linux-cpp/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

[license]: LICENSE

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
feat: A new feature
fix: Fixing a bug
docs: Documentation changes
style: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc.)
refactor: Refactoring code (neither adding a feature nor fixing a bug)
perf: Changes to improve performance
test: Adding missing tests or correcting existing tests
chore: Changes to the build process or auxiliary tools and libraries (does not affect source files, tests)
```
