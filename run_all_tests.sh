#!/bin/bash
# 完整測試套件 - 運行所有測試

echo "🚀 開始 Andromeda 完整測試套件..."
echo "======================================"

# 檢查必要工具
echo "🔧 檢查測試工具..."
MISSING_TOOLS=""

if ! command -v valgrind &> /dev/null; then
    MISSING_TOOLS="$MISSING_TOOLS valgrind"
fi

if ! command -v nc &> /dev/null && ! command -v netcat &> /dev/null; then
    MISSING_TOOLS="$MISSING_TOOLS netcat"
fi

if ! command -v protoc &> /dev/null; then
    MISSING_TOOLS="$MISSING_TOOLS protobuf-compiler"
fi

if [ -n "$MISSING_TOOLS" ]; then
    echo "⚠️ 缺少測試工具: $MISSING_TOOLS"
    echo "請安裝: sudo apt-get install$MISSING_TOOLS"
    echo "繼續進行可用的測試..."
fi

# 設置測試環境
TEST_RESULTS=""
TOTAL_TESTS=0
PASSED_TESTS=0

# 記錄測試結果的函數
record_test() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if [ $1 -eq 0 ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
        TEST_RESULTS="$TEST_RESULTS\n✅ $2"
        echo "✅ $2 - 通過"
    else
        TEST_RESULTS="$TEST_RESULTS\n❌ $2"
        echo "❌ $2 - 失敗"
    fi
}

echo ""
echo "======================================"
echo "🔨 1. 編譯測試"
echo "======================================"
bash test_build.sh
record_test $? "編譯測試"

echo ""
echo "======================================"
echo "⚙️ 2. 功能測試"
echo "======================================"
bash test_functions.sh
record_test $? "功能測試"

echo ""
echo "======================================"
echo "🔤 3. 字符串安全測試"
echo "======================================"
bash test_strings.sh
record_test $? "字符串安全測試"

echo ""
if command -v valgrind &> /dev/null then
    echo "======================================"
    echo "🛡️ 4. 內存安全測試"
    echo "======================================"
    bash test_memory.sh
    record_test $? "內存安全測試"
else
    echo "⏭️ 跳過內存安全測試 (valgrind 未安裝)"
fi

# 額外的快速檢查
echo ""
echo "======================================"
echo "🔍 5. 快速健康檢查"
echo "======================================"

# 檢查編譯輸出
if [ -f "build/andromeda" ]; then
    echo "✅ 可執行文件存在"
    
    # 檢查文件大小 (應該大於 100KB)
    FILE_SIZE=$(stat -c%s "build/andromeda" 2>/dev/null || stat -f%z "build/andromeda" 2>/dev/null)
    if [ $FILE_SIZE -gt 100000 ]; then
        echo "✅ 可執行文件大小正常 ($FILE_SIZE 字節)"
        record_test 0 "可執行文件檢查"
    else
        echo "⚠️ 可執行文件大小異常 ($FILE_SIZE 字節)"
        record_test 1 "可執行文件檢查"
    fi
else
    echo "❌ 可執行文件不存在"
    record_test 1 "可執行文件檢查"
fi

# 檢查關鍵源文件是否存在修復
echo "🔍 檢查安全修復..."
SECURITY_CHECKS=0
SECURITY_PASSED=0

# 檢查是否使用了安全的字符串函數
if grep -q "strnlen" app/andro_log.cxx app/andro_conf.cxx app/andro_setproctitle.cxx; then
    echo "✅ 安全字符串函數 (strnlen) 已使用"
    SECURITY_PASSED=$((SECURITY_PASSED + 1))
else
    echo "❌ 未發現安全字符串函數使用"
fi
SECURITY_CHECKS=$((SECURITY_CHECKS + 1))

# 檢查是否使用了 nanosleep 替代 usleep
if grep -q "nanosleep" misc/andro_threadpool.cxx net/andro_socket_time.cxx net/andro_socket_conn.cxx; then
    echo "✅ 現代系統調用 (nanosleep) 已使用"
    SECURITY_PASSED=$((SECURITY_PASSED + 1))
else
    echo "❌ 未發現現代系統調用使用"
fi
SECURITY_CHECKS=$((SECURITY_CHECKS + 1))

# 檢查 umask 設置
if grep -q "umask(077)" proc/andro_daemon.cxx; then
    echo "✅ 安全文件權限 (umask 077) 已設置"
    SECURITY_PASSED=$((SECURITY_PASSED + 1))
else
    echo "❌ 未發現安全文件權限設置"
fi
SECURITY_CHECKS=$((SECURITY_CHECKS + 1))

if [ $SECURITY_PASSED -eq $SECURITY_CHECKS ]; then
    record_test 0 "安全修復檢查"
else
    record_test 1 "安全修復檢查"
fi

# 生成測試報告
echo ""
echo "======================================"
echo "📊 測試報告"
echo "======================================"
echo -e "$TEST_RESULTS"
echo ""
echo "總測試數: $TOTAL_TESTS"
echo "通過測試: $PASSED_TESTS"
echo "失敗測試: $((TOTAL_TESTS - PASSED_TESTS))"
echo "成功率: $(( PASSED_TESTS * 100 / TOTAL_TESTS ))%"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo ""
    echo "🎉 所有測試通過！您的修復工作正確！"
    exit 0
else
    echo ""
    echo "⚠️ 部分測試失敗，請檢查上述輸出以了解詳情"
    exit 1
fi
