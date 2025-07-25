[English](README.md) | [繁體中文](README-zh-TW.md)
---
# Andromeda Linux C++

![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/silver2dream/andromeda-linux-cpp/ubuntu.yml?logo=github)
[![GitHub License](https://img.shields.io/github/license/silver2dream/andromeda-linux-cpp?logo=github)][license]
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/b0e6a6bf3b59467e9f18379dc87d9ee6)](https://app.codacy.com/gh/silver2dream/andromeda-linux-cpp/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

[license]: LICENSE

一個使用 C++ 編寫的高性能網路伺服器框架，採用多進程和多線程架構設計，專為處理高並發 TCP 連接而開發。

## 🌟 特色功能

- **多進程架構**：採用類似 Nginx 的 Master-Worker 進程模型
- **高並發處理**：基於 Epoll 的 I/O 多路復用配合線程池
- **Protocol Buffer 支援**：使用 protobuf 進行高效的二進制序列化
- **安全特性**：DDoS 防護、洪水攻擊檢測和連接超時管理
- **記憶體管理**：自定義記憶體池，配合 Valgrind 進行洩漏檢測
- **Docker 就緒**：支援容器化部署
- **CI/CD 整合**：透過 GitHub Actions 自動化測試和構建

## 🏗️ 系統架構

```
┌─────────────────┐
│   Master 進程    │
└─────────┬───────┘
          │
    ┌─────▼─────┐
    │  Worker   │
    │   進程群   │
    └─────┬─────┘
          │
    ┌─────▼─────┐
    │   線程池   │
    └───────────┘
```

### 核心組件

- **app/**: 主應用程式入口點
- **business/**: 業務邏輯處理器和訊息處理
- **net/**: 網路層，包含 socket 管理和 epoll 處理
- **signal/**: 信號處理，用於進程間通信
- **proc/**: 進程管理工具
- **misc/**: 工具函數和輔助類別
- **_include/**: 頭文件和公共介面

## 📁 專案結構

```
andromeda-linux-cpp/
├── 📁 app/                    # 主應用程式
│   ├── andromeda.cxx          # 入口點和主函數
│   └── CMakeLists.txt         # 構建配置
├── 📁 business/               # 業務邏輯層
│   ├── andro_logic.cxx        # 核心業務邏輯和訊息處理器
│   └── CMakeLists.txt         # 構建配置
├── 📁 net/                    # 網路層
│   ├── andro_socket.cxx       # 主要 socket 管理
│   ├── andro_socket_accept.cxx # 連接接受
│   ├── andro_socket_conn.cxx  # 連接管理
│   ├── andro_socket_inet.cxx  # 網路工具
│   ├── andro_socket_request.cxx # 請求處理
│   ├── andro_socket_time.cxx  # 超時和計時器處理
│   ├── ikcp.c                 # KCP 協定實現
│   └── CMakeLists.txt         # 構建配置
├── 📁 signal/                 # 信號處理
│   └── CMakeLists.txt         # 構建配置
├── 📁 proc/                   # 進程管理
│   └── CMakeLists.txt         # 構建配置
├── 📁 misc/                   # 工具和輔助
│   └── CMakeLists.txt         # 構建配置
├── 📁 _include/               # 頭文件
│   ├── andro_cmd.h            # 指令定義
│   ├── andro_conf.h           # 配置管理
│   ├── andro_crc32.h          # CRC32 校驗工具
│   ├── andro_func.h           # 通用函數
│   ├── andro_global.h         # 全域定義
│   ├── andro_lockmutex.h      # 互斥鎖和鎖定
│   ├── andro_logic.h          # 業務邏輯介面
│   ├── andro_macro.h          # 巨集定義
│   ├── andro_memory.h         # 記憶體管理
│   ├── andro_packet.h         # 封包結構定義
│   ├── andro_socket.h         # Socket 介面
│   ├── andro_threadpool.h     # 線程池管理
│   └── ikcp.h                 # KCP 協定頭文件
├── 📁 proto/                  # Protocol Buffer 定義
│   └── auth.proto             # 認證訊息
├── 📁 generated/              # 生成的 protobuf 文件
│   └── (自動生成的 .pb.cc 和 .pb.h 文件)
├── 📁 .github/workflows/      # CI/CD 管道
│   └── ubuntu.yml             # GitHub Actions 工作流程
├── 📁 logs/                   # 日誌文件目錄
├── 📄 andromeda.conf          # 伺服器配置文件
├── 📄 CMakeLists.txt          # 根構建配置
├── 📄 Dockerfile             # 容器配置
├── 📄 .clang-format           # 程式碼格式化規則
├── 📄 .gitignore              # Git 忽略模式
├── 📄 LICENSE                 # AGPL-3.0 授權條款
└── 📄 README.md               # 此文檔
```

## 🚀 快速開始

### 系統需求

確保您的系統已安裝以下依賴項：

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake g++ protobuf-compiler libprotobuf-dev valgrind

# CentOS/RHEL
sudo yum install -y gcc gcc-c++ cmake protobuf-compiler protobuf-devel valgrind
```

### 從原始碼構建

1. **複製儲存庫**
   ```bash
   git clone https://github.com/silver2dream/andromeda-linux-cpp.git
   cd andromeda-linux-cpp
   ```

2. **生成 Protocol Buffer 文件**
   ```bash
   protoc -I=./proto --cpp_out=./generated ./proto/*.proto
   ```

3. **構建專案**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

4. **執行伺服器**
   ```bash
   ./andromeda
   ```

### 使用 Docker

1. **構建 Docker 映像**
   ```bash
   docker build -t andromeda-server .
   ```

2. **執行容器**
   ```bash
   docker run -p 9000:9000 andromeda-server
   ```

## ⚙️ 配置設定

伺服器可透過 `andromeda.conf` 文件進行配置：

### 進程配置
```ini
[Process]
Master = andromeda:master process
Worker = andromeda:worker process
WorkerProcesses = 4          # Worker 進程數量
WorkerThreadCount = 2        # 每個 Worker 的線程數
Daemon = 1                   # 以守護進程運行 (1) 或前台運行 (0)
```

### 網路配置
```ini
[Net]
PortCount = 1
Port0 = 9000                 # 監聽埠號
WorkerConnections = 1024     # 每個 Worker 的最大連接數
SocketRecyWaitTime = 80      # Socket 回收等待時間（秒）
SocketKickTimerEnable = 1    # 啟用心跳超時檢測
SocketMaxWaitTime = 20       # 心跳超時閾值（秒）
SocketTimeoutKick = 1        # 超時時踢出客戶端
```

### 安全配置
```ini
[NetSecurity]
SecurityFloodAttackDetectionEnable = 1  # 啟用洪水攻擊檢測
SecurityFloodTimeInterval = 100          # 封包間隔閾值（毫秒）
SecurityFloodKickCounter = 10            # 間隔內最大封包數，超過即踢出
```

### 日誌配置
```ini
[Log]
Log = logs/error.log         # 日誌文件路徑
LogLevel = 8                 # 日誌級別 (0-8，0=最高優先級)
```

## 📡 通信協定

### 訊息格式

所有訊息都遵循標準化的封包格式：

```cpp
struct andro_packet_header_s {
    unsigned short pkg_len;   // 總訊息長度（頭部 + 主體）
    unsigned short msg_code;  // 訊息類型識別碼
    int crc32;                // CRC32 校驗和，用於完整性檢查
};
```

### 支援的指令

- **PING (0)**: 心跳/保活訊息
- **REGISTER (5)**: 使用者註冊
- **LOGIN (6)**: 使用者認證

### Protocol Buffer 定義範例

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

## 🧪 測試

### 記憶體洩漏檢測

執行 Valgrind 檢查記憶體洩漏：

```bash
make memcheck
```

### 自定義測試指令

```bash
# 清理構建產物
make clear

# 終止所有運行實例
make kill

# 重新生成 Protocol Buffer 文件
make protoc
```

## 🔧 開發指南

### 程式碼風格

本專案使用 clang-format 保持一致的程式碼格式。配置定義在 `.clang-format` 文件中。

### 提交規範

我們遵循 [Conventional Commits](https://www.conventionalcommits.org/) 規範：

```
feat: 新功能
fix: 修復錯誤
docs: 文檔變更
style: 程式碼風格變更（格式化、缺少分號等）
refactor: 程式碼重構（既不添加功能也不修復錯誤）
perf: 性能改進
test: 添加或修改測試
chore: 構建流程或輔助工具變更
```

### 添加新的訊息處理器

1. 在 `proto/` 目錄中定義您的訊息
2. 在 `business/andro_logic.cxx` 中添加處理函數
3. 在 `status_handler` 陣列中註冊處理器
4. 重新構建和測試

## 📊 性能表現

- **並發連接**：支援數千個並發連接
- **記憶體效率**：自定義記憶體池減少分配開銷
- **低延遲**：基於 Epoll 的事件處理最小化上下文切換
- **可擴展性**：多進程架構可跨 CPU 核心擴展

## 🐳 Docker 部署

包含的 Dockerfile 創建了一個安全的非 root 容器：

- 基於 Ubuntu 22.04
- 以使用者 `han`（UID 1000）身份運行
- 暴露 9000 埠
- 包含所有必要的依賴項

## 🤝 貢獻指南

1. Fork 此儲存庫
2. 創建功能分支（`git checkout -b feature/amazing-feature`）
3. 提交您的變更（`git commit -m 'feat: add amazing feature'`）
4. 推送到分支（`git push origin feature/amazing-feature`）
5. 開啟 Pull Request

## 📝 授權條款

本專案依據 **GNU Affero General Public License v3.0 (AGPL-3.0)** 進行授權 - 詳情請參閱 [LICENSE](LICENSE) 文件。

### AGPL-3.0 的重要特點：
- ✅ **使用自由**：您可以將此軟體用於任何目的
- ✅ **研究和修改自由**：擁有原始碼存取和修改權限
- ✅ **散布自由**：您可以散布副本和修改版本
- ⚠️ **Copyleft 要求**：修改版本也必須採用 AGPL-3.0 授權
- ⚠️ **網路使用揭露**：如果您在伺服器上運行修改版本，必須向使用者提供原始碼

## 🔗 相關專案

- [Protocol Buffers](https://developers.google.com/protocol-buffers) - 序列化函式庫
- [Valgrind](https://valgrind.org/) - 記憶體調試工具
- [Docker](https://www.docker.com/) - 容器化平台

## 📧 技術支援

如果您遇到任何問題或有疑問，請在 GitHub 上開啟 issue。

---

**注意**：這是一個面向生產環境的高性能伺服器框架。在生產環境中部署時，請確保進行適當的測試和安全措施。