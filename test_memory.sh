#!/bin/bash
# 內存安全測試腳本

echo "🛡️ 開始內存安全測試..."

# 1. 使用 Valgrind 檢查內存安全
echo "🔍 運行 Valgrind 內存檢查..."
cd build

# 創建測試用的日誌目錄
mkdir -p logs

# 運行 Valgrind 檢查（限時30秒）
timeout 30s valgrind \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --verbose \
    --log-file=valgrind_test.log \
    ./andromeda &

VALGRIND_PID=$!
sleep 5

# 發送一些測試數據
echo "📡 發送測試數據..."
echo "test" | timeout 2s nc localhost 9000 2>/dev/null || true
echo "longer test string for boundary testing" | timeout 2s nc localhost 9000 2>/dev/null || true

# 等待一會兒讓服務器處理
sleep 3

# 停止服務器
kill -TERM $VALGRIND_PID 2>/dev/null || true
wait $VALGRIND_PID 2>/dev/null || true

# 檢查 Valgrind 結果
echo "📊 分析 Valgrind 結果..."
if [ -f "valgrind_test.log" ]; then
    echo "=== Valgrind 報告摘要 ==="
    
    # 檢查內存洩漏
    LEAKS=$(grep -c "definitely lost:" valgrind_test.log)
    ERRORS=$(grep -c "ERROR SUMMARY:" valgrind_test.log)
    
    if grep -q "definitely lost: 0 bytes in 0 blocks" valgrind_test.log; then
        echo "✅ 無內存洩漏"
    else
        echo "⚠️ 發現潛在內存洩漏："
        grep "definitely lost:" valgrind_test.log
    fi
    
    if grep -q "ERROR SUMMARY: 0 errors" valgrind_test.log; then
        echo "✅ 無內存錯誤"
    else
        echo "⚠️ 發現內存錯誤："
        grep "ERROR SUMMARY:" valgrind_test.log
    fi
    
    echo "📄 完整報告位於: build/valgrind_test.log"
else
    echo "❌ Valgrind 日誌文件未生成"
fi

echo "🎯 內存安全測試完成"
