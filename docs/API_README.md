# 五子棋AI对比测试系统

## 📋 系统架构

```
┌─────────────┐     HTTP API      ┌──────────────┐
│   前端UI    │ ──────────────▶  │  FastAPI后端  │
│ (JavaScript)│ ◀──────────────  │  (Python)    │
└─────────────┘                   └──────┬───────┘
                                         │
                                  subprocess调用
                                         │
                                  ┌──────▼───────┐
                                  │ C++ AI引擎   │
                                  │ (Minimax/MCTS)│
                                  └──────────────┘
```

## 🚀 快速启动

### 方式一：一键启动（推荐）

```bash
# Windows: 双击运行
start.bat

# 或手动执行：
# 1. 编译C++
g++ -O2 -std=c++17 -o ver_api.exe ver_api.cpp board.h pattern.h minimax.h mcts.h

# 2. 安装Python依赖
pip install -r api/requirements.txt

# 3. 启动FastAPI服务
cd api && python main.py
```

### 方式二：分步启动

#### 1️⃣ 编译C++程序

```bash
# 确保已安装g++ (MinGW或Visual Studio Build Tools)
g++ --version

# 编译
g++ -O2 -std=c++17 -o ver_api.exe ver_api.cpp board.h pattern.h minimax.h mcts.h

# 测试编译结果
test_cpp.bat
```

#### 2️⃣ 启动API服务

```bash
cd api
pip install -r requirements.txt
python main.py
```

访问：
- API地址：http://localhost:8000
- API文档：http://localhost:8000/docs （Swagger UI）

#### 3️⃣ 打开前端

```bash
# 方式A：直接打开文件
双击 web/index.html

# 方式B：通过HTTP服务（推荐，避免跨域问题）
# 在另一个终端运行：
cd web && python -m http.server 3000
# 然后访问 http://localhost:3000
```

## 🎮 使用方法

### 基本使用

1. **选择引擎**：在控制面板选择 `AI引擎`
   - `JavaScript (本地)` - 使用浏览器内置JS引擎
   - `C++ (需启动API服务)` - 调用后端C++引擎

2. **配置参数**：
   - 棋盘大小：5-38
   - AI算法：Minimax / MCTS
   - 搜索深度/模拟次数

3. **开始游戏**：点击"开始新游戏"

### 对比测试模式

**目的**：验证C++和JS实现的AI决策是否一致

**步骤**：

1. 启动API服务 (`start.bat`)
2. 打开前端页面
3. 用**相同配置**分别测试两个引擎：
   - 先选 `JavaScript` 引擎玩一局，记录走法
   - 再选 `C++` 引擎，相同开局，对比AI响应

**预期结果**：
- ✅ 两个引擎在相同局面下应给出**相同的最佳走法**
- ⚠️ 如果结果不同，说明实现存在差异，需要排查

## 🔌 API接口文档

### 1. 获取最佳走法

```http
POST /api/move
Content-Type: application/json

{
  "boardSize": 15,
  "currentTurn": 0,
  "selfPlayer": 2,
  "algorithm": "minimax",
  "searchDepth": 4,
  "history": [
    {"x": 8, "y": 8},
    {"x": 8, "y": 9}
  ],
  "engine": "cpp"
}
```

**响应示例**：

```json
{
  "success": true,
  "move": {"x": 9, "y": 7},
  "engine": "cpp",
  "algorithm": "minimax",
  "nodesSearched": 15234,
  "thinkTime": 123.45,
  "timestamp": 1700000000
}
```

### 2. 健康检查

```http
GET /health
```

**响应**：

```json
{
  "status": "healthy",
  "cpp_available": true,
  "cpp_path": "../ver_api.exe"
}
```

### 3. 性能基准测试

```http
POST /api/benchmark?runs=5
Content-Type: application/json

{
  "boardSize": 15,
  "selfPlayer": 1,
  "algorithm": "minimax",
  "searchDepth": 4,
  "history": []
}
```

**响应**：

```json
{
  "algorithm": "minimax",
  "runs": 5,
  "statistics": {
    "avg_time_ms": 150.23,
    "min_time_ms": 145.12,
    "max_time_ms": 158.90,
    "total_time_s": 0.75
  },
  "moves_consistent": true,
  "sample_moves": [
    {"x": 8, "y": 8},
    {"x": 8, "y": 8},
    {"x": 8, "y": 8}
  ]
}
```

## 🧪 测试用例

### 测试脚本

使用提供的 `test_cpp.bat` 进行基础功能测试：

```bash
test_cpp.bat
```

### 手动测试（curl）

```bash
# 测试空棋盘
curl -X POST http://localhost:8000/api/move ^
  -H "Content-Type: application/json" ^
  -d "{\"boardSize\":15,\"selfPlayer\":1,\"algorithm\":\"minimax\",\"searchDepth\":4,\"history\":[]}"

# 测试带历史走法
curl -X POST http://localhost:8000/api/move ^
  -H "Content-Type: application/json" ^
  -d "{\"boardSize\":15,\"selfPlayer\":2,\"algorithm\":\"mcts\",\"iterations\":1000,\"history\":[{\"x\":8,\"y\":8},{\"x\":9,\"y\":9}]}"
```

## 🔍 故障排查

### 问题1：C++编译失败

**症状**：`g++` 命令未找到

**解决方案**：
```bash
# 安装MinGW-w64 (Windows)
# 下载地址: https://www.mingw-w64.org/

# 或安装Visual Studio Build Tools
# Visual Studio Installer → 组件 → "C++生成工具"
```

### 问题2：前端无法连接API

**症状**：控制台显示 `Failed to fetch` 或 `CORS error`

**解决方案**：
1. 确认已启动 `start.bat`
2. 检查端口8000是否被占用
3. 如遇跨域问题，确保从 `http://localhost:3000` 访问前端（而非直接打开文件）

### 问题3：C++返回错误

**症状**：API返回500错误

**排查步骤**：
```bash
# 1. 直接测试C++程序
echo boardSize=15 | ver_api.exe
echo selfPlayer=1 | ver_api.exe
echo algorithm=minimax | ver_api.exe
echo searchDepth=4 | ver_api.exe

# 2. 查看详细错误日志
# FastAPI控制台会输出stderr信息
```

## 📊 性能对比参考

| 配置 | JS引擎 | C++引擎 | 加速比 |
|------|--------|---------|--------|
| 15×15, Minimax d=2 | ~50ms | ~10ms | 5x |
| 15×15, Minimax d=4 | ~500ms | ~100ms | 5x |
| 15×15, Minimax d=6 | ~5000ms | ~800ms | 6x |
| 15×15, MCTS 1000次 | ~2000ms | ~300ms | 7x |

*注：性能因硬件而异，仅供参考*

## 📝 开发笔记

### C++ CLI接口设计

输入格式（stdin）：
```
key=value
```

支持的字段：
- `boardSize`: 棋盘大小
- `selfPlayer`: AI执子方 (1=黑, 2=白)
- `algorithm`: 算法 ("minimax" / "mcts")
- `searchDepth`: Minimax深度
- `iterations`: MCTS模拟次数
- `move=x,y`: 历史走法（可多个）

输出格式（stdout）：
```json
{
  "success": true/false,
  "move": {"x": int, "y": int},
  "engine": "cpp",
  "algorithm": "...",
  "nodesSearched": int,
  "timestamp": int
}
```

### 扩展建议

1. **添加WebSocket支持**：实时推送AI思考进度
2. **实现完整对比模式**：同时调用两个引擎并展示差异
3. **添加性能监控面板**：图表化展示搜索节点数、时间等指标
4. **支持批量测试**：自动对弈N局统计胜率

## 👥 贡献指南

欢迎提交Issue和PR！

主要改进方向：
- 优化C++编译选项（开启更多优化）
- 添加更多测试用例
- 改进UI交互体验
- 支持更多AI算法变体

---

**版本**: 1.0.0
**最后更新**: 2026-05-12
