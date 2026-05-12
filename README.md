# 五子棋游戏 (Gomoku)

一个基于 C++ 实现的命令行五子棋游戏，支持 AI 对战。

## 项目简介

本项目是一个五子棋游戏实现，支持 AI 自动对战。提供两种 AI 引擎：Minimax（Alpha-Beta 剪枝）和 MCTS（蒙特卡洛树搜索），通过棋型分析评估局面。棋盘大小固定为 38×38，支持禁手规则，先连成五子者获胜，棋盘下满判平局。

## 功能特性

- **AI 对战**：Minimax + Alpha-Beta 剪枝，支持进攻/防守策略调节
- **MCTS 引擎**：蒙特卡洛树搜索，带权随机模拟
- **棋型评估**：识别五连、长连、活四、冲四、活三、眠三、活二、眠二、活一、眠一
- **跳型检测**：支持单间隙跳型（如 `X _ X`）的棋型评估
- **候选走法优化**：仅搜索已有棋子周围 2 格内空位，排序后取 Top 15 加速剪枝
- **禁手规则**：黑棋禁止长连、双四、双活三
- **胜负与平局判定**：自动检测五子连珠和棋盘满平局
- **棋盘显示**：带坐标轴和网格的棋盘显示
  - `●` 表示黑子
  - `○` 表示白子
  - `·` 表示空位交叉点

## 文件结构

```
.
├── board.h                 # 棋盘类、游戏规则（落子、判胜、禁手、棋型分析）
├── minimax.h               # Minimax + Alpha-Beta 剪枝 AI 引擎
├── mcts.h                  # MCTS 蒙特卡洛树搜索 AI 引擎
├── ver_minimax.cpp         # Minimax 自弈 / 人机对战入口
├── ver_mcts.cpp            # MCTS 自弈入口
├── ver_minimax_vs_mcts.cpp # Minimax vs MCTS 对战入口（支持命令行参数）
└── README.md               # 项目说明文档
```

## 编译运行

### 环境要求

- C++17 或更高版本编译器
- Windows/Linux/macOS

### 编译命令

```bash
# Minimax 自弈
g++ -std=c++17 -O2 -o gomoku ver_minimax.cpp

# Minimax vs MCTS 对战
g++ -std=c++17 -O2 -o gomoku_vs ver_minimax_vs_mcts.cpp
```

### 运行游戏

```bash
# Minimax 自弈（黑棋与白棋均为 Minimax AI）
./gomoku

# Minimax vs MCTS（默认黑棋 MCTS，白棋 Minimax）
./gomoku_vs

# 自定义参数对战
./gomoku_vs --minimax-depth 6 --mcts-iterations 100000 --minimax-color black
```

## 游戏说明

1. 棋盘大小默认为 38×38 （可修改，会影响ai运算速度）
2. 黑方先行，双方轮流落子
3. 黑棋受禁手规则约束（禁止长连、双四、双活三）
4. 任意一方连成五子即获胜
5. 棋盘填满未分胜负则为平局
6. 可通过取消 `ver_minimax.cpp` 中的注释切换到人工输入模式

## 版本记录

### v2.3.0 (2026-05-06)

- 修复 getStageWeight() 阶段权重计算错误：使用 getCurrentCount() 替代错误的 getGraph().size()
- 修复 analyzeForm() 跳型检测：限制最多一个间隙，支持紧邻中心的跳型 `X _ X`
- 修复 analyzeForm() 中心棋子未计入 count 的问题
- 修复 hasFive() 误将长连等同五连，导致黑棋长连禁手失效
- 新增 getCurrentCount() 方法暴露当前落子数
- 新增平局输出（ver_minimax.cpp）

### v2.2.0 (2026-05-06)

- 修复禁手规则逻辑错误：调整五连与长连判断顺序，五连优先判定
- 将 analyzeForm 声明和实现改为 const，解决 const Gomoku 对象调用问题

### v2.1.0 (2026-04-26)

- 重构评估函数，改为遍历已有棋子消除重复计算
- 新增 evaluatePiece() 从棋子出发评估，仅起始点计分
- 新增 evaluateEmpty() 评估空位战略价值，用于候选排序
- 修正棋型评分表，区分死四/活三、死三/活二等同级棋型
- 防守权重从 1.2 调整为 1.12

### v2.0.1 (2026-04-26)

- 优化 show() 棋盘显示，添加坐标轴和网格
- 空位显示使用 · 中点，呈现网格效果
- show() 添加 const 修饰

### v2.0.0 (2026-04-26)

- 新增 AI 对战功能（Minimax + Alpha-Beta 剪枝）
- 实现棋型分析（五连、活四、死四、活三、死三等）
- 实现全局评估函数，支持防守权重调节
- 实现候选走法生成与排序优化

### v1.0.0 (2026-04-26)

- 初始版本发布
- 实现基础五子棋功能
- 实现胜负判定逻辑
- 实现棋盘显示功能

## 技术细节

### 类设计

- `Gomoku`: 棋盘核心类，管理棋盘状态和游戏规则
  - `set(x, y, player)` / `undo(x, y)`: 落子与撤销
  - `validPosition(x, y, player)`: 坐标合法性（越界/重复/禁手）
  - `Win(x, y, player)`: 五连胜利判定
  - `GameOver()`: 棋盘满平局判定
  - `getColor(x, y)`: 获取指定位置棋子颜色
  - `getCurrentCount()`: 获取当前已落子数
  - `analyzeForm(x, y, dx, dy, player)`: 单方向棋型分析（含跳型检测）
  - `isForbidden(x, y, player)`: 黑棋禁手判定
  - `show()`: 显示棋盘

- `Minimax`: 基于 Minimax + Alpha-Beta 剪枝的 AI 引擎
  - `getBestMove(g)`: 获取当前局面最优走法
  - `alphaBeta(g, depth, alpha, beta, isMaximizing)`: Alpha-Beta 搜索
  - `evaluate(g)`: 全局局面评估（攻防分离 + 阶段权重）
  - `evaluatePiece(g, x, y, p)`: 单棋子棋型评估（含组合加成）
  - `evaluateEmpty(g, x, y)`: 空位战略价值评估（用于候选排序）
  - `getCandidateMoves(g)`: 候选走法生成（范围2格，Top 15）

- `MCTS`: 蒙特卡洛树搜索 AI 引擎
  - `getBestMove(g)`: UCB 选择 + 带权随机模拟
  - `quickEval(g, x, y, player)`: 快速攻防评分
  - 根节点展开 60 候选，每层展开 20 子节点

### AI 算法

**Minimax:**
- 搜索深度：默认 4 层（可配置）
- 攻防系数：黑棋 attackCoeff=1.2 defenseCoeff=1.0，白棋 attackCoeff=1.0 defenseCoeff=1.3
- 阶段权重：开局 0.8 / 中局 1.0 / 残局 1.2
- 防守权重：默认 1.12

**MCTS:**
- 迭代次数：默认 1000（Minimax vs MCTS 默认 50000）
- 探索参数：UCB exploration=1.414
- 模拟步数上限：80 步，带权随机选择

### 棋型评分表（Minimax）

| 棋型 | 分值 |
|------|------|
| 五连 / 长连 | 10,000,000 |
| 活四 | 1,000,000 |
| 冲四 | 500,000 |
| 活三 | 50,000 |
| 眠三 | 1,000 |
| 活二 | 200 |
| 眠二 | 50 |
| 活一 | 10 |
| 眠一 | 2 |

### 棋型组合加成

| 组合 | 加分 |
|------|------|
| 白棋双四（活四+活四/冲四） | 2,000,000 |
| 活四 + 活三 | 1,000,000 |
| 双活三 | 300,000 |
| 冲四 + 活三 | 200,000 |
| 双冲四 | 150,000 |

### 游戏规则

- 棋盘大小：38×38
- 胜利条件：任意方向（横、竖、斜）连成五子
- 黑棋禁手：长连（6+子）、双四、双活三
- 平局条件：棋盘 1444 格全部填满无人五连
- 坐标系统：1-based 索引

## 作者

本项目为学习用途开发。
