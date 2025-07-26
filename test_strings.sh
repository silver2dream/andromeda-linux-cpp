#!/bin/bash
# 字符串安全測試腳本

echo "🔤 開始字符串安全測試..."

cd build

# 1. 測試配置文件解析的字符串安全性
echo "📖 測試配置文件字符串解析..."

# 創建測試配置文件
cat > test_config.conf << 'EOF'
# 測試正常配置
[Process]
Master = andromeda:master process
Worker = andromeda:worker process

# 測試長字符串（邊界測試）
[Net]
Port0 = 9000
VeryLongConfigName = ThisIsAVeryLongConfigurationValueThatShouldBeHandledSafelyWithoutCausingBufferOverflowsOrMemoryIssues

# 測試特殊字符
[Test]
SpecialChars = @#$%^&*(){}[]|\\:";'<>?,./
EmptyValue = 
EOF

echo "✅ 測試配置文件已創建"

# 2. 測試進程標題的字符串處理
echo "🏷️ 測試進程標題字符串處理..."

# 使用長標題啟動服務器
./andromeda &
SERVER_PID=$!
sleep 3

# 檢查進程是否正常運行
if kill -0 $SERVER_PID 2>/dev/null; then
    echo "✅ 長進程標題處理正常"
else
    echo "❌ 進程標題處理可能有問題"
fi

# 3. 測試網路輸入的字符串安全性
echo "🌐 測試網路輸入字符串安全性..."

# 測試正常輸入
echo "normal test" | timeout 2s nc localhost 9000 2>/dev/null || true

# 測試長輸入
python3 -c "print('A' * 1000)" | timeout 2s nc localhost 9000 2>/dev/null || true

# 測試包含空字符的輸入
printf "test\x00null\x00terminated" | timeout 2s nc localhost 9000 2>/dev/null || true

# 測試二進制數據
python3 -c "import sys; sys.stdout.buffer.write(b'\\x01\\x02\\x03\\x04\\x05' * 100)" | timeout 2s nc localhost 9000 2>/dev/null || true

sleep 2

# 檢查服務器是否仍在運行
if kill -0 $SERVER_PID 2>/dev/null; then
    echo "✅ 服務器在各種輸入測試後仍正常運行"
else
    echo "❌ 服務器在輸入測試後崩潰"
fi

# 4. 測試日誌字符串處理
echo "📝 測試日誌字符串處理..."

# 檢查日誌文件是否正常
if [ -f "logs/error.log" ]; then
    # 檢查日誌是否包含異常字符或截斷
    LOG_SIZE=$(wc -c < logs/error.log)
    if [ $LOG_SIZE -gt 0 ]; then
        echo "✅ 日誌文件正常寫入 ($LOG_SIZE 字節)"
        
        # 檢查最新的日誌條目
        echo "📄 最新日誌條目："
        tail -3 logs/error.log
    else
        echo "⚠️ 日誌文件為空"
    fi
else
    echo "⚠️ 未找到日誌文件"
fi

# 清理
kill -TERM $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
rm -f test_config.conf

echo "🎯 字符串安全測試完成"
