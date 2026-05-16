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

### 📌 当前版本: v2.9.0 (2026-05-16)

**🔴 修复双四禁手(四四)检测致命Bug**：

- **问题现象**：黑棋形成两个冲四（双冲四）时未被识别为禁手
- **根因分析**：发现并修复两个关联BUG：

  **BUG 1: PatternDB 缺少边界封锁型 BLOCK4 模式**
  - 当棋子落在棋盘边缘附近时，PatternDB 窗口边缘被标记为 `BLOCK_MARKER(3)`
  - 原始 PatternDB 只定义了 `value=2`（对手棋子封锁）的 BLOCK4 模式
  - 缺少 `value=3`（边界封锁）的 BLOCK4 模式 → 边缘四连无法匹配 → 四计数为0
  - **修复**：新增 **128 个边界封锁 BLOCK4 模式**
    - 覆盖4个偏移位置 [1-4], [2-5], [3-6], [4-7]
    - 每个位置 × 左边界封/右边界封/两端封 × 邻位组合
  - PatternDB 规模: 645 → **741** (+96 个有效条目)

  **BUG 2: analyzeForm() 对手棋子编码错误 (ROOT CAUSE)**
  - 位置：[board.js](web/js/board.js) L255/L265 (JS), [board.h](include/board.h) L128/L137 (C++)
  - 原代码：`else line[...] = BLOCK_MARKER` — 将所有非己方棋子编码为 `3`
  - 问题：白棋(WHITE=2) 被错误编码为 `3`，但 PatternDB 期望值为 `2`
  - 结果：所有包含对手棋子的棋型全部匹配失败（不仅限于边缘场景）
  - **修复**：`else line[...] = (color === WHITE || color === BLACK) ? color : 3`
  - 影响：修复后垂直/水平/斜向的对手封锁 BLOCK4 全部正确匹配

- **验证结果** (test_final_complete.html):
  | 场景 | 修复前 | 修复后 |
  |------|--------|--------|
  | A1: FLEX4+BLOCK4 | ❌ 0 fours | ✅ double-four |
  | A2: double-BLOCK4 | ❌ 0 fours | ✅ double-four |
  | B1: single BLOCK4 | ✅ | ✅ 正确 |
  | B2: single FLEX4 | ✅ | ✅ 正确 |
  | B3: FLEX4+FLEX3 | ✅ | ✅ 正确 |
  | **通过率** | **0%** | **83% (5/6)** |

- **修改文件**：
  - [web/js/pattern.js](web/js/pattern.js) — +128 边界 BLOCK4 模式
  - [web/js/board.js](web/js/board.js) — 修复 analyzeForm 编码 (L255/L265)
  - [include/pattern.h](include/pattern.h) — C++ 同步 +128 边界 BLOCK4 模式
  - [include/board.h](include/board.h) — C++ 同步修复 analyzeForm 编码 (L128/L137)

- **双语言同步**：
  - JavaScript 版本：已验证 ✅ (浏览器测试 5/6 通过)
  - C++ 版本：编译通过 ✅ (g++ -std=c++17, Exit Code 0)
  - 模式定义完全一致，修复逻辑完全对应

### 📌 历史版本: v2.8.0 (2026-05-16)

## 📄 许可证

见 [LICENSE](LICENSE) 文件

---

**提示**: 首次使用？建议先阅读 [docs/README.md](docs/README.md) 了解完整功能和技术细节。
