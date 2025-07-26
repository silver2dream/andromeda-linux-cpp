#!/bin/bash
# 測試腳本：驗證所有修復是否正確

echo "🔧 開始測試 Andromeda 修復功能..."

# 1. 清理之前的構建
echo "📁 清理構建目錄..."
rm -rf build cmake-build-debug
mkdir build
cd build

# 2. 生成 Protocol Buffer 文件
echo "🔄 生成 Protocol Buffer 文件..."
cd ..
protoc -I=./proto --cpp_out=./generated ./proto/*.proto
if [ $? -ne 0 ]; then
    echo "❌ Protocol Buffer 生成失敗"
    exit 1
fi
echo "✅ Protocol Buffer 文件生成成功"

# 3. CMake 配置
echo "⚙️ 配置 CMake..."
cd build
cmake ..
if [ $? -ne 0 ]; then
    echo "❌ CMake 配置失敗"
    exit 1
fi
echo "✅ CMake 配置成功"

# 4. 編譯項目
echo "🔨 編譯項目..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "❌ 編譯失敗"
    exit 1
fi
echo "✅ 編譯成功"

# 5. 檢查可執行文件
if [ -f "./andromeda" ]; then
    echo "✅ 可執行文件 andromeda 生成成功"
    ls -la ./andromeda
else
    echo "❌ 可執行文件未生成"
    exit 1
fi

echo "🎉 所有編譯測試通過！"
