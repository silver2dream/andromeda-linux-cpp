# Contributing to Andromeda Linux C++

Thank you for your interest in contributing to Andromeda! 🚀

## 📢 Project Status

**Important**: This project is currently in **showcase mode**. This means:
- ✅ The codebase is functional and demonstrates high-performance networking concepts
- ⚠️ Active maintenance and feature development are limited due to time constraints
- 🤝 Community contributions and discussions are welcome and encouraged

## 🤝 How You Can Contribute

### 1. Learning and Sharing
- **Explore the codebase** and learn from the implementation
- **Share your insights** in discussions or blog posts
- **Reference this project** in your own work or education

### 2. Community Support
- **Help answer questions** in issues
- **Share solutions** and workarounds you've discovered
- **Provide feedback** on bugs and improvements

### 3. Fork and Extend
- **Fork the repository** for your own modifications
- **Implement new features** in your fork
- **Share your improvements** with the community

### 4. Become a Maintainer
If you're passionate about this project and want to take on long-term maintenance:
- Demonstrate consistent involvement in the community
- Show expertise in C++ network programming
- Express interest in becoming a maintainer

## 🐛 Reporting Issues

When reporting issues:
1. Use the appropriate issue template
2. Provide clear reproduction steps
3. Include system information
4. Be patient for responses - community help is encouraged

## 🔧 Code Guidelines

If you're working on your own fork:

### Code Style
- Follow the existing `.clang-format` configuration
- Use consistent naming conventions
- Add comments for complex logic

### Commit Messages
Follow [Conventional Commits](https://www.conventionalcommits.org/):
```
feat: add new feature
fix: resolve bug
docs: update documentation
style: formatting changes
refactor: code restructuring
perf: performance improvements
test: add or update tests
chore: maintenance tasks
```

### Testing
- Test your changes thoroughly
- Use Valgrind for memory leak detection
- Ensure builds pass on supported platforms

## 🚀 Development Environment

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake g++ protobuf-compiler libprotobuf-dev valgrind

# CentOS/RHEL
sudo yum install -y gcc gcc-c++ cmake protobuf-compiler protobuf-devel valgrind
```

### Building
```bash
git clone https://github.com/silver2dream/andromeda-linux-cpp.git
cd andromeda-linux-cpp
protoc -I=./proto --cpp_out=./generated ./proto/*.proto
mkdir build && cd build
cmake ..
make
```

### Testing
```bash
# Memory leak check
make memcheck

# Clean build
make clear

# Kill running instances
make kill
```

## 🌟 Community Guidelines

### Be Respectful
- Treat all community members with respect
- Be constructive in feedback and discussions
- Help create a welcoming environment for newcomers

### Share Knowledge
- Document your solutions and workarounds
- Write clear explanations for technical concepts
- Help others learn from your experience

### Collaborate
- Work together on complex problems
- Share resources and references
- Connect contributors with similar interests

## 📞 Getting Help

- **GitHub Issues**: For bugs, questions, and discussions
- **Code Review**: Community members can review each other's forks
- **Knowledge Sharing**: Share articles, tutorials, or improvements

## 🎯 Focus Areas

If you want to contribute, these areas might be interesting:

### Documentation
- Code comments and examples
- Architecture documentation
- Performance tuning guides
- Deployment best practices

### Testing
- Unit tests
- Integration tests
- Performance benchmarks
- Memory leak detection

### Features (in your fork)
- Additional protocol support
- Monitoring and metrics
- Configuration management
- Security enhancements

### Performance
- Profiling and optimization
- Memory usage improvements
- Scalability enhancements
- Benchmarking tools

## 📄 License

By contributing to this project, you agree that your contributions will be licensed under the AGPL-3.0 license.

---

**Remember**: While direct contributions to this repository may have limited review, your involvement in the community, learning from the code, and sharing improvements in your own forks are all valuable contributions to the broader ecosystem!

Thank you for being part of the Andromeda community! 🌟