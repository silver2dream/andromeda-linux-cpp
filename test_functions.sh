#!/bin/bash
# 功能測試腳本

echo "⚙️ 開始功能測試..."

cd build

# 1. 測試配置文件加載
echo "📝 測試配置文件加載..."
if [ -f "../andromeda.conf" ]; then
    echo "✅ 配置文件存在"
    
    # 檢查配置文件內容
    if grep -q "Port0" ../andromeda.conf; then
        echo "✅ 配置文件格式正確"
    else
        echo "❌ 配置文件格式異常"
    fi
else
    echo "❌ 配置文件不存在"
fi

# 2. 測試進程標題設置功能
echo "🏷️ 測試進程標題設置..."
./andromeda &
SERVER_PID=$!
sleep 2

# 檢查進程標題
PROCESS_TITLE=$(ps -p $SERVER_PID -o comm= 2>/dev/null)
if [ -n "$PROCESS_TITLE" ]; then
    echo "✅ 进程标题设置成功: $PROCESS_TITLE"
else
    echo "⚠️ 無法獲取進程標題"
fi

# 3. 測試網路連接
echo "🌐 測試網路連接..."
sleep 2

# 檢查端口是否開放
if netstat -tuln | grep -q ":9000.*LISTEN"; then
    echo "✅ 服務器在端口 9000 監聽"
    
    # 測試基本連接
    if timeout 3s bash -c "echo > /dev/tcp/localhost/9000" 2>/dev/null; then
        echo "✅ TCP 連接測試成功"
    else
        echo "⚠️ TCP 連接測試失敗"
    fi
else
    echo "❌ 服務器未在端口 9000 監聽"
fi

# 4. 測試日誌功能
echo "📋 測試日誌功能..."
if [ -f "logs/error.log" ]; then
    echo "✅ 日誌文件已創建"
    
    # 檢查日誌內容
    if grep -q "listen() successful" logs/error.log; then
        echo "✅ 日誌內容正常"
    else
        echo "⚠️ 日誌內容可能異常"
    fi
    
    echo "📄 最新日誌內容："
    tail -5 logs/error.log
else
    echo "⚠️ 日誌文件未創建"
fi

# 5. 測試守護進程模式
echo "👻 測試守護進程模式..."
kill -TERM $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null || true

# 修改配置以啟用守護進程模式
cp ../andromeda.conf ../andromeda.conf.bak
sed -i 's/Daemon = 0/Daemon = 1/g' ../andromeda.conf 2>/dev/null || \
sed -i '' 's/Daemon = 0/Daemon = 1/g' ../andromeda.conf 2>/dev/null || true

./andromeda
sleep 3

if pgrep -f andromeda > /dev/null; then
    echo "✅ 守護進程模式工作正常"
    pkill -f andromeda
else
    echo "⚠️ 守護進程模式可能有問題"
fi

# 恢復配置
cp ../andromeda.conf.bak ../andromeda.conf

echo "🎉 功能測試完成"
