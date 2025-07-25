[English](README.md) | [繁體中文](README-zh-TW.md)
---
# Andromeda Linux C++

![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/silver2dream/andromeda-linux-cpp/ubuntu.yml?logo=github)
[![GitHub License](https://img.shields.io/github/license/silver2dream/andromeda-linux-cpp?logo=github)][license]
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/b0e6a6bf3b59467e9f18379dc87d9ee6)](https://app.codacy.com/gh/silver2dream/andromeda-linux-cpp/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

[license]: LICENSE

A high-performance network server framework written in C++, designed for handling concurrent TCP connections with multi-process and multi-threading architecture.

## 🌟 Features

- **Multi-Process Architecture**: Master-worker process model similar to Nginx
- **High Concurrency**: Epoll-based I/O multiplexing with thread pool
- **Protocol Buffer Support**: Efficient binary serialization using protobuf
- **Security Features**: DDoS protection, flood attack detection, and connection timeout management
- **Memory Management**: Custom memory pool with leak detection using Valgrind
- **Docker Ready**: Containerized deployment support
- **CI/CD Integration**: Automated testing and building with GitHub Actions

## 🏗️ Architecture

```
┌─────────────────┐
│   Master Process │
└─────────┬───────┘
          │
    ┌─────▼─────┐
    │  Worker   │
    │ Processes │
    └─────┬─────┘
          │
    ┌─────▼─────┐
    │  Thread   │
    │   Pool    │
    └───────────┘
```

### Core Components

- **app/**: Main application entry point
- **business/**: Business logic handlers and message processing
- **net/**: Network layer with socket management and epoll handling
- **signal/**: Signal processing for inter-process communication
- **proc/**: Process management utilities
- **misc/**: Utility functions and helper classes
- **_include/**: Header files and public interfaces

## 📁 Project Structure

```
andromeda-linux-cpp/
├── 📁 app/                    # Main application
│   ├── andromeda.cxx          # Entry point and main function
│   └── CMakeLists.txt         # Build configuration
├── 📁 business/               # Business logic layer
│   ├── andro_logic.cxx        # Core business logic and message handlers
│   └── CMakeLists.txt         # Build configuration
├── 📁 net/                    # Network layer
│   ├── andro_socket.cxx       # Main socket management
│   ├── andro_socket_accept.cxx # Connection acceptance
│   ├── andro_socket_conn.cxx  # Connection management
│   ├── andro_socket_inet.cxx  # Network utilities
│   ├── andro_socket_request.cxx # Request processing
│   ├── andro_socket_time.cxx  # Timeout and timer handling
│   ├── ikcp.c                 # KCP protocol implementation
│   └── CMakeLists.txt         # Build configuration
├── 📁 signal/                 # Signal processing
│   └── CMakeLists.txt         # Build configuration
├── 📁 proc/                   # Process management
│   └── CMakeLists.txt         # Build configuration
├── 📁 misc/                   # Utilities and helpers
│   └── CMakeLists.txt         # Build configuration
├── 📁 _include/               # Header files
│   ├── andro_cmd.h            # Command definitions
│   ├── andro_conf.h           # Configuration management
│   ├── andro_crc32.h          # CRC32 checksum utilities
│   ├── andro_func.h           # Common functions
│   ├── andro_global.h         # Global definitions
│   ├── andro_lockmutex.h      # Mutex and locking
│   ├── andro_logic.h          # Business logic interface
│   ├── andro_macro.h          # Macro definitions
│   ├── andro_memory.h         # Memory management
│   ├── andro_packet.h         # Packet structure definitions
│   ├── andro_socket.h         # Socket interface
│   ├── andro_threadpool.h     # Thread pool management
│   └── ikcp.h                 # KCP protocol header
├── 📁 proto/                  # Protocol Buffer definitions
│   └── auth.proto             # Authentication messages
├── 📁 generated/              # Generated protobuf files
│   └── (auto-generated .pb.cc and .pb.h files)
├── 📁 .github/workflows/      # CI/CD pipelines
│   └── ubuntu.yml             # GitHub Actions workflow
├── 📁 logs/                   # Log files directory
├── 📄 andromeda.conf          # Server configuration file
├── 📄 CMakeLists.txt          # Root build configuration
├── 📄 Dockerfile             # Container configuration
├── 📄 .clang-format           # Code formatting rules
├── 📄 .gitignore              # Git ignore patterns
├── 📄 LICENSE                 # AGPL-3.0 license
└── 📄 README.md               # This documentation
```

## 🚀 Quick Start

### Prerequisites

Make sure you have the following dependencies installed:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake g++ protobuf-compiler libprotobuf-dev valgrind

# CentOS/RHEL
sudo yum install -y gcc gcc-c++ cmake protobuf-compiler protobuf-devel valgrind
```

### Building from Source

1. **Clone the repository**
   ```bash
   git clone https://github.com/silver2dream/andromeda-linux-cpp.git
   cd andromeda-linux-cpp
   ```

2. **Generate Protocol Buffer files**
   ```bash
   protoc -I=./proto --cpp_out=./generated ./proto/*.proto
   ```

3. **Build the project**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

4. **Run the server**
   ```bash
   ./andromeda
   ```

### Using Docker

1. **Build Docker image**
   ```bash
   docker build -t andromeda-server .
   ```

2. **Run container**
   ```bash
   docker run -p 9000:9000 andromeda-server
   ```

## ⚙️ Configuration

The server can be configured through the `andromeda.conf` file:

### Process Configuration
```ini
[Process]
Master = andromeda:master process
Worker = andromeda:worker process
WorkerProcesses = 4          # Number of worker processes
WorkerThreadCount = 2        # Number of threads per worker
Daemon = 1                   # Run as daemon (1) or foreground (0)
```

### Network Configuration
```ini
[Net]
PortCount = 1
Port0 = 9000                 # Listening port
WorkerConnections = 1024     # Max connections per worker
SocketRecyWaitTime = 80      # Socket recycling wait time (seconds)
SocketKickTimerEnable = 1    # Enable heartbeat timeout detection
SocketMaxWaitTime = 20       # Heartbeat timeout threshold (seconds)
SocketTimeoutKick = 1        # Kick clients on timeout
```

### Security Configuration
```ini
[NetSecurity]
SecurityFloodAttackDetectionEnable = 1  # Enable flood attack detection
SecurityFloodTimeInterval = 100          # Packet interval threshold (ms)
SecurityFloodKickCounter = 10            # Max packets in interval before kick
```

### Logging Configuration
```ini
[Log]
Log = logs/error.log         # Log file path
LogLevel = 8                 # Log level (0-8, 0=highest priority)
```

## 📡 Protocol

### Message Format

All messages follow a standardized packet format:

```cpp
struct andro_packet_header_s {
    unsigned short pkg_len;   // Total message length (header + body)
    unsigned short msg_code;  // Message type identifier
    int crc32;                // CRC32 checksum for integrity
};
```

### Supported Commands

- **PING (0)**: Heartbeat/keepalive message
- **REGISTER (5)**: User registration
- **LOGIN (6)**: User authentication

### Example Protocol Buffer Definition

```protobuf
syntax = "proto3";
package auth;

message C2SRegister {
   string username = 1;
   string password = 2;
}

message S2CRegister {
    string username = 1;
    string password = 2;   
}
```

## 🧪 Testing

### Memory Leak Detection

Run Valgrind to check for memory leaks:

```bash
make memcheck
```

### Custom Testing Commands

```bash
# Clean build artifacts
make clear

# Kill all running instances
make kill

# Regenerate protocol buffer files
make protoc
```

## 🔧 Development

### Code Style

This project uses clang-format for consistent code formatting. The configuration is defined in `.clang-format`.

### Commit Convention

We follow [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
feat: A new feature
fix: Fixing a bug
docs: Documentation changes
style: Code style changes (formatting, missing semi-colons, etc.)
refactor: Code refactoring (neither adding features nor fixing bugs)
perf: Performance improvements
test: Adding or modifying tests
chore: Build process or auxiliary tool changes
```

### Adding New Message Handlers

1. Define your message in `proto/` directory
2. Add the handler function in `business/andro_logic.cxx`
3. Register the handler in the `status_handler` array
4. Rebuild and test

## 📊 Performance

- **Concurrent Connections**: Supports thousands of concurrent connections
- **Memory Efficient**: Custom memory pool reduces allocation overhead
- **Low Latency**: Epoll-based event handling minimizes context switching
- **Scalable**: Multi-process architecture scales across CPU cores

## 🐳 Docker Deployment

The included Dockerfile creates a secure, non-root container:

- Based on Ubuntu 22.04
- Runs under user `han` (UID 1000)
- Exposes port 9000
- Includes all necessary dependencies

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'feat: add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)** - see the [LICENSE](LICENSE) file for details.

### Key Points of AGPL-3.0:
- ✅ **Freedom to use**: You can use this software for any purpose
- ✅ **Freedom to study and modify**: Source code access and modification rights
- ✅ **Freedom to distribute**: You can distribute copies and modifications
- ⚠️ **Copyleft requirement**: Modified versions must also be AGPL-3.0 licensed
- ⚠️ **Network use disclosure**: If you run a modified version on a server, you must provide the source code to users

## 🔗 Related Projects

- [Protocol Buffers](https://developers.google.com/protocol-buffers) - Serialization library
- [Valgrind](https://valgrind.org/) - Memory debugging tool
- [Docker](https://www.docker.com/) - Containerization platform

## 📧 Support

If you encounter any issues or have questions, please open an issue on GitHub.

---

**Note**: This is a high-performance server framework intended for production use. Please ensure proper testing and security measures when deploying in production environments.