@echo off
chcp 65001 >nul
echo ========================================
echo   五子棋AI系统 - 编译和启动脚本
echo ========================================
echo.

cd /d "%~dp0"

echo [1/4] 检查C++编译器...
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo ❌ 未找到g++，请安装MinGW或Visual Studio Build Tools
    pause
    exit /b 1
)
echo ✅ 找到C++编译器

echo.
echo [2/4] 编译C++ API接口...
g++ -O2 -std=c++17 -o ver_api.exe ver_api.cpp board.h pattern.h minimax.h mcts.h
if %errorlevel% neq 0 (
    echo ❌ C++编译失败！
    pause
    exit /b 1
)
echo ✅ C++编译成功: ver_api.exe

echo.
echo [3/4] 安装Python依赖...
pip install -r api\requirements.txt --quiet
if %errorlevel% neq 0 (
    echo ⚠️ Python依赖安装可能失败，请手动执行: pip install -r api\requirements.txt
)

echo.
echo [4/4] 启动FastAPI服务...
echo.
echo 🚀 API服务地址: http://localhost:8000
echo 📖 API文档地址: http://localhost:8000/docs
echo 🎮 前端地址: web\index.html (需要修改为调用API)
echo.
echo 按 Ctrl+C 停止服务
echo ========================================

cd api
python main.py

pause
