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

### v2.8.0 (2026-05-16)

- **📜 新增完整历史步数记录系统**：
  - **功能**：为网页版添加详细的历史步数显示、统计和导航功能
  
  - **历史面板**：
    - 📋 实时显示每一步的落子记录（最新20步）
    - 🎯 棋盘坐标标记（如 H8、D12）+ 原始坐标 (x, y)
    - ⚫⚪ 颜色标识区分黑白棋
    - 📊 统计信息（黑棋/白棋各走多少步）
  
  - **交互功能**：
    - 👆 点击历史步骤 → 高亮显示该步在棋盘上的位置
    - ↩️ 悔棋按钮 → 撤销上一步（PvE模式撤销2步）
    - 🗑️ 清空按钮 → 重新开始游戏
    - 🔄 自动更新（每次落子后刷新）
  
  - **技术实现**：
    - [web/index.html](../web/index.html) - 新增历史面板UI结构
      - 历史控制按钮（悔棋、清空）
      - 可滚动的历史列表容器
      - 统计信息显示区域
    
    - [web/js/game.js](../web/js/game.js) - 核心逻辑增强
      - `updateMoveHistory()` 方法：同步更新历史UI
      - 增强 `makeMove()`：落子后自动调用历史更新
      - 增强 `undo()`：悔棋后自动刷新历史显示
      - 统计计算（黑棋/白棋步数统计）
    
    - [web/js/ui.js](../web/js/ui.js) - 渲染引擎扩展
      - `renderMoveHistory()` 方法：渲染历史列表
      - 坐标转换（数字坐标 → 棋盘标记如 H8）
      - 点击事件绑定（高亮对应棋位）
      - 视觉反馈（active状态样式）
    
    - [web/js/main.js](../web/js/main.js) - 事件绑定
      - 清空历史按钮事件处理
      - 确认对话框防止误操作
    
    - [web/style.css](../web/style.css) - UI美化
      - 历史列表滚动条自定义样式
      - 步骤项悬停动画效果
      - 黑白棋颜色区分渐变背景
      - 活跃状态高亮样式
      - 统计区域卡片式设计

  - **用户体验优化**：
    - 📍 精确坐标显示（棋盘标记 + 数字坐标双格式）
    - 🎨 视觉层次分明（黑棋深色边框，白棋浅色边框）
    - ✨ 平滑交互动画（hover滑动效果）
    - 📱 响应式设计（最大高度280px，超出可滚动）
    - 🔍 信息提示（tooltip显示详细信息）

  - **性能考虑**：
    - 只渲染最近20步（避免长对局时DOM过大）
    - 超出部分显示"还有N步记录"提示
    - 事件委托优化（减少内存占用）

### v2.7.0 (2026-05-16)

- **🚫 新增完整禁手检测功能（用户体验优化）**：
  - **功能**：为网页版人机/人人对战添加黑棋禁手检测与提示系统
  - **支持规则**：
    - ✅ **长连禁手**：黑棋超过5连判定为禁手
    - ✅ **四四禁手**：黑棋同时形成2个或以上活四/冲四
    - ✅ **三三禁手**：黑棋同时形成2个或以上活三
    - ✅ **五连优先**：形成五连时不受禁手限制
  
  - **技术实现**：
    - [web/js/board.js](../web/js/board.js) - 增强 `checkForbidden()` 方法
      - 新增详细返回对象（包含类型、描述、详细信息）
      - 新增 `validPosition()` 返回结构化验证结果
      - 保持向后兼容（`isForbidden()` 仍返回布尔值）
    
    - [web/js/game.js](../web/js/game.js) - 集成禁手处理流程
      - 新增 `handleInvalidMove()` 统一错误处理
      - 新增 `showForbiddenWarning()` 详细提示显示
      - 区分不同错误类型（越界/占用/禁手）并给出具体信息
    
    - [web/js/ui.js](../web/js/ui.js) - 视觉反馈系统
      - 新增 `highlightForbiddenPosition()` 禁位高亮
      - 新增 `drawForbiddenMarker()` 动态警告标记
      - 橙色闪烁圆圈 + ✕ 符号，2秒自动消失
      - 添加 `forbiddenPos` 状态跟踪

  - **用户体验提升**：
    - 📍 **精确定位**：显示禁手位置坐标 (x, y)
    - 📝 **详细说明**：明确告知是哪种禁手（三三/四四/长连）
    - 🔢 **数据支撑**：显示检测到的活三/冲四数量
    - 👁️ **视觉反馈**：橙色闪烁标记 + 控制台详细日志
    - ⏱️ **自动恢复**：2秒后自动清除警告标记

  - **测试工具**：
    - 创建 [test_forbidden.html](../test_forbidden.html) 自动化测试页面
    - 包含6组测试用例（正常位置、白棋、三三、四四、五连优先、集成测试）
    - 提供游戏界面实际操作测试入口

### v2.6.0 (2026-05-15)

- **🔴 紧急修复 PatternDB 棋型识别缺陷（致命Bug）**：
  - **问题**：PatternDB 缺少"跳型棋子间隙被填"后的防守结果模式
    - 导致 AI 无法正确评估"堵住跳活三/跳活二间隙位"的防守价值
    - 评估函数返回 0 分（DEFAULT），AI 认为该位置"无威胁"
    - 结果：AI 放弃最佳防守位置，选择次优解 → 对手形成活四 → **输棋**
  
  - **P0 致命修复：FLEX3 跳活三间隙防守 (16个模式)**：
    - 缺失类型：`X_XX` 和 `XX_X` 的间隙被对手落子填充后
    - 原始形态：跳活三 `_X_XX_` 或 `_XX_X_`
    - 防守操作：在间隙位落子 → `_XOXX_` 或 `_XXOX_`
    - 修复前：PatternDB 返回 DEFAULT (0分) ❌
    - 修复后：正确识别为 BLOCK3 (1000分) ✅
    - 影响范围：所有包含跳活三的对局场景
    - 严重程度：**Critical（直接导致输棋）**
  
  - **P1 重要修复：FLEX2 跳活二间隙防守 (~16个模式)**：
    - 缺失类型：`X_X` 和 `X__X` 的间隙被填充后的 BLOCK2 模式
    - 影响范围：中期战术布局，影响活三形成路径的阻断
    - 严重程度：High（影响中长期策略）
  
  - **P2/P3 补全修复：BLOCK3/BLOCK2 分散模式 (~16个模式)**：
    - 补全被分割后的低威胁眠三/眠二组合
    - 提升评估精度，完善边界情况处理
  
  - **技术细节**：
    - 修改文件：[web/js/pattern.js](../web/js/pattern.js)
    - 新增模式统计：
      - `addBlock3Patterns()`: +24个模式（P0: 16个 + P2: 8个）
      - `addBlock2Patterns()`: +20个模式（P1: 16个 + P3: 4个）
      - 总计新增：**~44个 PatternDB 条目**
    - 代码行数：约90行新增代码
    - 测试验证：已创建 [test_pattern_fix.js](../test_pattern_fix.js) 自动化测试脚本

- **根因分析**：
  - PatternDB 设计时的思维盲区：只考虑了"端点封堵"式防守
  - 遗漏了"间隙填充"式防守（仅对跳型棋子有效）
  - 连型棋子（XXX）无此问题，但跳型（X_XX）需要分别定义两种防守结果

- **验证方法**：
  - 浏览器控制台运行 `runPatternFixTests()`
  - 预期结果：所有测试通过（100% success rate）

- **🔄 双语言同步完成 (C++)**：
  - ✅ 同步修改文件：[include/pattern.h](../include/pattern.h)
  - ✅ C++ 编译测试：**通过** (g++ -std=c++17, Exit Code 0)
  - ✅ 新增模式统计（与JS版本完全一致）：
    - `addBlock3Patterns()`: +40个模式
      - 跳活三 1011 (X_XX): 16个基本+边界变体
      - 跳活三 1101 (XX_X): 16个基本+边界变体
      - 偏移形态: 8个
    - `addBlock2Patterns()`: +28个模式
      - 跳活二 101 (X_X): 16个基本+边界变体
      - 跳活二 1001 (X__X): 8个
      - 偏移形态: 4个
  - ✅ 总计新增：**~68个 PatternDB 条目**
  - ✅ 代码行数：约120行新增代码（C++版本）
  - ✅ 语法检查：通过 g++ 编译器严格语法验证

- **双语言一致性保证**：
  - JavaScript 版本：[web/js/pattern.js](../web/js/pattern.js) (已验证 ✅)
  - C++ 版本：[include/pattern.h](../include/pattern.h) (已编译 ✅)
  - 模式定义完全一致，参数格式完全对应
  - 两套引擎现在具有相同的防守识别能力

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
