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
├── include/                 # 核心头文件
│   ├── board.h              # 棋盘类、游戏规则（落子、判胜、禁手、棋型分析）
│   ├── pattern.h            # 棋型数据库（PatternDB 哈希表）
│   ├── minimax.h            # Minimax + Alpha-Beta 剪枝 AI 引擎
│   └── mcts.h               # MCTS 蒙特卡洛树搜索 AI 引擎
├── src/                     # 源代码实现
│   ├── main_minimax.cpp     # Minimax 自弈 / 人机对战入口
│   ├── main_mcts.cpp        # MCTS 自弈入口
│   ├── main_vs.cpp          # Minimax vs MCTS 对战入口（支持命令行参数）
│   └── main_api.cpp         # API 服务入口（供 Python 后端调用）
├── bin/                     # 编译输出目录
├── docs/                    # 项目文档
│   └── README.md            # 本文档
└── api/                     # Python Web API 服务
```

## 编译运行

### 环境要求

- C++17 或更高版本编译器
- Windows/Linux/macOS

### 编译命令

```bash
# Minimax 自弈（输出到 bin/ 目录）
g++ -std=c++17 -O2 -I./include -o bin/gomoku src/main_minimax.cpp

# MCTS 自弈
g++ -std=c++17 -O2 -I./include -o bin/gomoku_mcts src/main_mcts.cpp

# Minimax vs MCTS 对战
g++ -std=c++17 -O2 -I./include -o bin/gomoku_vs src/main_vs.cpp

# API 版本（供 Python 后端调用）
g++ -std=c++17 -O2 -I./include -o bin/ver_api src/main_api.cpp
```

### 运行游戏

```bash
# Minimax 自弈（黑棋与白棋均为 Minimax AI）
./bin/gomoku

# MCTS 自弈
./bin/gomoku_mcts

# Minimax vs MCTS（默认黑棋 MCTS，白棋 Minimax）
./bin/gomoku_vs

# 自定义参数对战
./bin/gomoku_vs --minimax-depth 6 --mcts-iterations 100000 --minimax-color black
```

## 游戏说明

1. 棋盘大小默认为 38×38 （可修改，会影响ai运算速度）
2. 黑方先行，双方轮流落子
3. 黑棋受禁手规则约束（禁止长连、双四、双活三）
4. 任意一方连成五子即获胜
5. 棋盘填满未分胜负则为平局
6. 可通过取消 `ver_minimax.cpp` 中的注释切换到人工输入模式

## 版本记录

### v2.5.0 (2026-05-15)

- **移除无效的阶段权重系统**：删除 `getStageWeight()` 及相关接口
  - 原实现将 `stageWeight` 作为全局公因子，未实现真正的动态防守权重调整
  - 删除 `openingWeight`、`middleWeight`、`endgameWeight` 属性
  - 简化评估函数，移除所有 `stageWeight` 调用
  - `evaluatePiece()` 中位置评分改为始终计算（不再仅限开局）
  - 同步更新 JavaScript (minimax.js) 和 C++ (minimax.h) 双版本
- **修复执白模式（AI先行）功能**：
  - 问题：选择执白后游戏卡住，AI不自动走第一步
  - 原因：`startNewGame()` 未在初始化后触发AI先行逻辑
  - 修复：添加自动检测机制，当玩家执白时自动调用 `makeAIMove()`
  - 影响：PvE模式下执白/执黑均可正常工作（无需后端服务，纯JS引擎即可）
- **紧急修复棋盘渲染失败问题**：
  - **Bug 1: `offset is not defined` 错误**（致命错误）
    - 位置：`drawStarPointsToContext()` 函数第356-359行
    - 原因：JavaScript `const` 块级作用域导致变量访问失败
      - `offset` 在 `if (boardSize >= 13)` 块内定义
      - 但在 `if (boardSize >= 19)` 块内引用时已超出作用域
    - 修复：将 `offset` 提升为函数级别变量（`let offset = 0`）
    - 影响：≥19格棋盘完全无法渲染（ReferenceError崩溃）
  
  - **Bug 2: 静态层缓存机制失效**（功能缺陷）
    - 位置：`drawBoard()` 函数第186-197行
    - 原因：缓存对象被覆盖，`canvas` 属性丢失
      - `drawStaticLayer()` 设置 `renderCache = { canvas: offscreen }`
      - 紧接着被 `renderCache = { ...currentState }` 覆盖
      - 导致 `renderCache.canvas` 为 `undefined`
    - 修复：重构为返回值模式 + 对象合并
      - `drawStaticLayer()` 改为返回 offscreen Canvas
      - `drawBoard()` 统一管理缓存状态
    - 新增降级方案：`drawFallbackBackground()` 缓存失败时的备用渲染
  
  - **影响范围**：所有规格棋盘均受影响（5-38格）
  - **严重程度**：Critical（导致页面白屏/报错）
- **大规格棋盘性能优化（支持5-38格流畅运行）**：
  - **前端渲染优化 (ui.js)**：
    - 新增静态层缓存机制：背景+网格+星位只绘制一次，后续复用
    - 大规格棋盘使用简化渲染模式：无渐变、无阴影的纯色棋子
    - 动态Canvas缩放：超出700px时自动缩放显示
    - 自适应鼠标节流：大规格下100ms间隔（正常50ms）
    - 扩展规格支持：新增30格和38格档位的单元格尺寸配置
  - **后端计算优化 (game.js + minimax.js)**：
    - 新增 `getAdaptiveAIParams()` 方法：根据规格自动调整AI参数上限
    - 规格映射表：10格(深度6/5000次) → 15格(4/2000) → 19格(3/1000) → 25格(2/500) → 30格(2/300) → 38格(2/200)
    - Minimax候选点动态优化：大规格缩小搜索半径(1格)、减少候选点数量(15个)
    - 保证响应时间：< 3秒（普通浏览器）
  - **用户体验改进 (index.html)**：
    - 界面提示更新：标注大规格自动优化功能

### v2.4.0 (2026-05-12)

- **同步 JavaScript 版本到 C++ 版本**：确保 Web 端 AI 引擎与 C++ 完全一致
- 修复 minimax.js basePatternScore() 评分表：
  - FLEX4 分值从 1,000,000 调整为 **5,000,000**（与 C++ 一致）
  - BLOCK4 分值从 500,000 调整为 **3,000,000**（与 C++ 一致）
  - 移除跳型特殊评分（isJumpBlock4/isJumpFlex3/isJumpFlex2），C++ 未实现
- 移除 minimax.js evaluateCombination() 函数及调用（C++ 无此逻辑）
- 修复 minimax.js getCandidateMoves() 必防点检测逻辑：
  - 改为两轮检测：先检测五连威胁，再检测活四威胁（与 C++ 一致）
  - 原逻辑错误地同时检测活四和冲四，且遗漏五连检测
- 改进 mcts.js 随机数生成器：
  - 从简单 LCG 算法升级为 **xorshift128+** 高质量算法
  - 随机数质量接近 C++ std::mt19937，提升 MCTS 模拟准确性
- 修复 mcts.js getBestMove() 循环控制流错误：
  - 移除错误的 continue 语句，确保每次迭代都执行 simulate 和 backpropagate
  - 原逻辑导致部分迭代跳过回传，影响搜索结果正确性

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
