@echo off
chcp 65001 >nul
echo ========================================
echo   C++ API接口测试
echo ========================================
echo.

cd /d "%~dp0"

if not exist ver_api.exe (
    echo ❌ 请先运行 start.bat 编译C++程序
    pause
    exit /b 1
)

echo 📝 测试用例1: 基本功能测试
echo 输入: 15x15棋盘, Minimax深度4, 黑棋先手
echo.
echo boardSize=15 | ver_api.exe
echo selfPlayer=1 | ver_api.exe
echo algorithm=minimax | ver_api.exe
echo searchDepth=4 | ver_api.exe
echo.

echo ========================================
echo 📝 测试用例2: 带历史走法
echo ---------------------------------------
(
echo # 测试带历史走法的情况
echo boardSize=15
echo selfPlayer=2
echo algorithm=minimax
echo searchDepth=4
echo move=8,8
echo move=8,9
echo move=9,8
) | ver_api.exe

echo.
echo ========================================
echo ✅ 测试完成！
echo.
pause
