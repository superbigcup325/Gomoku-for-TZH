# Gomoku - 五子棋 AI 对战系统

> 🎮 基于 C++ 实现的高性能五子棋 AI，支持 Minimax 和 MCTS 双引擎

## 📂 项目结构

```
Gomoku/
├── include/          # 🔷 核心头文件（公共接口）
├── src/              # 🔶 源代码实现（各功能入口）
├── bin/              # ⚫  编译输出（.git忽略）
├── docs/             # 📚  完整技术文档
│   └── README.md     # → 详细说明、API文档、版本记录
├── api/              # 🐍  Python Web API 服务
└── README.md         # 📋  本文件（快速导航）
```

## 🚀 快速开始

### 1️⃣ 编译项目

```bash
# Minimax 版本（推荐入门）
g++ -std=c++17 -O2 -I./include -o bin/gomoku src/main_minimax.cpp

# MCTS 版本
g++ -std=c++17 -O2 -I./include -o bin/gomoku_mcts src/main_mcts.cpp

# Minimax vs MCTS 对战
g++ -std=c++17 -O2 -I./include -o bin/gomoku_vs src/main_vs.cpp
```

### 2️⃣ 运行游戏

```bash
# 人机对战（黑棋AI vs 白棋人工输入）
./bin/gomoku

# AI自弈（Minimax vs MCTS）
./bin/gomoku_vs --minimax-depth 4 --mcts-iterations 10000
```

## 📖 文档导航

| 文档 | 说明 |
|------|------|
| [docs/README.md](docs/README.md) | **完整技术文档**（功能特性、算法细节、版本记录） |
| [docs/API_README.md](docs/API_README.md) | Web API 系统架构与部署指南 |
| [docs/pattern_table.md](docs/pattern_table.md) | 棋型评分表参考 |

## ✨ 核心特性

- **🤖 双AI引擎**: Minimax (Alpha-Beta剪枝) + MCTS (蒙特卡洛树搜索)
- **♟️ 完整规则**: 禁手检测、五连判定、平局处理 (38×38棋盘)
- **⚡ 性能优化**: 候选点Top15剪枝、PatternDB哈希查表、阶段权重评估
- **🌐 Web支持**: Python FastAPI + JavaScript前端（可选）

## 🛠️ 技术栈

- **核心语言**: C++17 (AI引擎)
- **Web后端**: Python 3.x + FastAPI
- **Web前端**: JavaScript (HTML5 Canvas)
- **构建工具**: g++ / MinGW / MSVC

## 📝 开发说明

### 目录职责

- **`include/`**: 公共头文件，定义接口和数据结构
- **`src/`**: 各功能的 main() 入口，引用 `../include/` 头文件
- **`bin/`**: 编译产物存放位置（已加入 .gitignore）
- **`docs/`**: 项目文档、课程要求、评分标准

### 修改代码流程

1. 编辑 `include/*.h` (修改接口/实现)
2. 编辑 `src/main_*.cpp` (修改入口逻辑)
3. 重新编译到 `bin/` 目录
4. 更新 `docs/README.md` (记录变更)

### 📌 当前版本: v2.7.0 (2026-05-16)

**🚫 新增：完整禁手检测与提示系统**

- **功能**：网页版黑棋禁手自动检测 + 友好提示 + 视觉反馈
- **支持规则**：长连禁手、四四禁手、三三禁手、五连优先
- **用户体验**：
  - 精确位置坐标显示
  - 详细禁手类型说明（三三/四四/长连）
  - 橙色闪烁警告标记（2秒自动消失）
  - 控制台完整日志记录
- **修改文件**：
  - [web/js/board.js](web/js/pattern.js) (增强禁手算法)
  - [web/js/game.js](web/js/game.js) (集成处理流程)
  - [web/js/ui.js](web/js/ui.js) (视觉反馈系统)
- **测试工具**：[test_forbidden.html](test_forbidden.html) (自动化测试)
- **详情**：见 [docs/README.md](docs/README.md#v270-2026-05-16) 完整版本记录

### 📌 历史版本: v2.6.0 (2026-05-15)

## 📄 许可证

见 [LICENSE](LICENSE) 文件

---

**提示**: 首次使用？建议先阅读 [docs/README.md](docs/README.md) 了解完整功能和技术细节。
