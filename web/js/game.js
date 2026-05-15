class GomokuGame {
    constructor() {
        this.gomoku = null;
        this.ui = null;
        this.ai = null;
        this.gameMode = 'pve'; // pve 或 pvp
        this.playerColor = Player.BLACK;
        this.currentTurn = 0; // 0=黑棋, 1=白棋
        this.gameOver = false;
        this.moveHistory = [];
        this.isAIThinking = false;

        // 配置参数
        this.boardSize = 15;
        this.aiAlgorithm = 'minimax';
        this.searchDepth = 4;
        this.iterations = 1000;

        // 引擎选择: "js" (本地JS) 或 "cpp" (通过API调用C++)
        this.engine = 'js';
        this.apiBaseUrl = 'http://localhost:8000'; // FastAPI地址
    }

    init(ui) {
        this.ui = ui;
        this.ui.setOnMove((x, y) => this.handlePlayerMove(x, y));
        this.startNewGame();
    }

    startNewGame() {
        const startTime = performance.now();

        try {
            // 显示初始化提示
            this.showMessage('正在初始化新游戏...', 'info');

            // 获取配置
            this.boardSize = parseInt(document.getElementById('boardSize').value) || 15;
            this.gameMode = document.getElementById('gameMode').value;
            this.aiAlgorithm = document.getElementById('aiAlgorithm').value;
            this.searchDepth = parseInt(document.getElementById('searchDepth').value) || 4;
            this.iterations = parseInt(document.getElementById('iterations').value) || 1000;
            this.playerColor = document.getElementById('playerColor').value === 'black' ? Player.BLACK : Player.WHITE;

            // 获取引擎选择
            const engineSelect = document.getElementById('aiEngine');
            if (engineSelect) {
                this.engine = engineSelect.value;
            }

            // 验证参数范围
            this.boardSize = Math.max(5, Math.min(38, this.boardSize));
            
            // 根据棋盘规格自动调整AI参数（性能优化）
            const adaptiveParams = this.getAdaptiveAIParams(this.boardSize);
            this.searchDepth = Math.min(this.searchDepth, adaptiveParams.maxDepth);
            this.iterations = Math.min(this.iterations, adaptiveParams.maxIterations);
            
            console.log(`🎯 自适应参数: 深度=${this.searchDepth}, 迭代=${this.iterations} (规格${this.boardSize}×${this.boardSize})`);

            // 初始化棋盘
            this.gomoku = new Gomoku(this.boardSize);
            this.ui.setBoardSize(this.boardSize);

            // 初始化AI（仅JS引擎需要）
            if (this.gameMode === 'pve' && this.engine === 'js') {
                const aiPlayer = (this.playerColor === Player.BLACK) ? Player.WHITE : Player.BLACK;
                if (this.aiAlgorithm === 'minimax') {
                    this.ai = new Minimax(aiPlayer, this.searchDepth);
                } else {
                    this.ai = new MCTS(aiPlayer, this.iterations);
                }
            } else {
                this.ai = null;
            }

            // 重置游戏状态
            this.currentTurn = 0;
            this.gameOver = false;
            this.moveHistory = [];
            this.isAIThinking = false;

            // 更新UI
            this.ui.clearLastMove();
            this.ui.render(this.gomoku);
            this.updateStatus();
            this.updateButtons();

            const endTime = performance.now();
            const initTime = Math.round(endTime - startTime);

            // 显示成功消息
            let engineInfo = '';
            if (this.gameMode === 'pve') {
                engineInfo = ` (${this.engine.toUpperCase()} + ${this.aiAlgorithm})`;
            }
            this.showMessage(`✅ 游戏就绪！${this.boardSize}×${this.boardSize} ${engineInfo} [${initTime}ms]`, 'success');

            console.log(`🎮 新游戏: ${this.boardSize}x${this.boardSize}, 模式:${this.gameMode}, AI:${this.aiAlgorithm || '无'}, 初始化耗时:${initTime}ms`);

            // PvE模式下，如果AI先行（玩家执白），自动触发AI第一步
            if (this.gameMode === 'pve' && !this.gameOver) {
                const currentPlayer = this.getCurrentPlayer();
                if (currentPlayer !== this.playerColor) {
                    console.log(`🤖 AI (${currentPlayer === Player.BLACK ? '黑棋' : '白棋'}) 先行...`);
                    setTimeout(() => this.makeAIMove(), 100);
                }
            }

        } catch (error) {
            console.error('❌ 游戏启动失败:', error);
            this.showMessage('游戏启动失败：' + error.message, 'error');
        }
    }

    handlePlayerMove(x, y) {
        if (this.gameOver || this.isAIThinking) return;

        const currentPlayer = this.getCurrentPlayer();

        // 在PvE模式下，检查是否是玩家的回合
        if (this.gameMode === 'pve' && currentPlayer !== this.playerColor) {
            this.showMessage('请等待AI完成操作！', 'warning');
            return;
        }

        // 尝试落子
        if (!this.makeMove(x, y, currentPlayer)) {
            return;
        }

        // 检查游戏是否结束
        if (this.checkGameEnd(x, y, currentPlayer)) {
            return;
        }

        // PvE模式下，让AI走棋
        if (this.gameMode === 'pve' && !this.gameOver) {
            this.makeAIMove();
        }
    }

    makeMove(x, y, player) {
        const validation = this.gomoku.validPosition(x, y, player);

        if (!validation.valid) {
            this.handleInvalidMove(x, y, player, validation);
            return false;
        }

        this.gomoku.set(x, y, player);
        this.moveHistory.push({ x, y, player });
        this.ui.setLastMove(x, y);
        this.ui.render(this.gomoku);
        this.ui.clearForbiddenHighlight();

        this.currentTurn++;
        this.updateStatus();

        return true;
    }

    handleInvalidMove(x, y, player, validation) {
        switch (validation.reason) {
            case 'out_of_range':
                this.showMessage(`⚠️ 位置 (${x}, ${y}) 超出棋盘范围！`, 'error');
                break;

            case 'occupied':
                this.showMessage(`⚠️ 位置 (${x}, ${y}) 已有棋子！`, 'error');
                break;

            case 'forbidden':
                this.showForbiddenWarning(x, y, player, validation.forbiddenInfo);
                break;

            default:
                this.showMessage('⚠️ 无效位置！', 'error');
        }

        this.ui.highlightForbiddenPosition(x, y);
    }

    showForbiddenWarning(x, y, player, forbiddenInfo) {
        const posText = `(${x}, ${y})`;
        let message = `🚫 ${posText} 禁手：${forbiddenInfo.description}`;

        if (forbiddenInfo.type === 'double-four') {
            const d = forbiddenInfo.details;
            message += `\n   检测到：活四×${d.flex4} + 冲四×${d.block4}`;
        } else if (forbiddenInfo.type === 'double-three') {
            message += `\n   检测到：活三×${forbiddenInfo.details.flex3}`;
        }

        this.showMessage(message, 'warning');

        console.warn(`禁手检测 [${posText}]:`, {
            type: forbiddenInfo.type,
            description: forbiddenInfo.description,
            details: forbiddenInfo.details
        });
    }

    async makeAIMove() {
        if (this.gameOver) return;

        // 检查是否需要AI（PvE模式下）
        if (this.gameMode !== 'pve') return;

        this.isAIThinking = true;
        this.updateButtons();
        this.ui.showThinking();
        this.showMessage('AI思考中...', 'info');

        // 使用setTimeout让UI有机会更新
        await new Promise(resolve => setTimeout(resolve, 50));

        try {
            const startTime = performance.now();

            if (this.engine === 'cpp') {
                // 调用C++引擎（通过FastAPI）
                await this.makeCPPMove();
            } else {
                // 调用本地JS引擎
                await this.makeJSMove(startTime);
            }

        } catch (error) {
            console.error('AI计算出错:', error);
            this.showMessage('AI计算出错：' + error.message, 'error');
        } finally {
            this.isAIThinking = false;
            this.ui.hideThinking();
            this.updateButtons();
        }
    }

    async makeJSMove(startTime) {
        if (!this.ai) {
            this.showMessage('JS AI未初始化！', 'error');
            return;
        }

        const [x, y] = this.ai.getBestMove(this.gomoku);
        const endTime = performance.now();
        const thinkTime = Math.round(endTime - startTime);

        // 更新统计信息
        document.getElementById('nodeCount').textContent = this.ai.getNodesSearched().toLocaleString();
        document.getElementById('thinkTime').textContent = `${thinkTime}ms`;

        if (x === -1 || y === -1) {
            this.showMessage('AI无法找到有效位置！', 'error');
            return;
        }

        const aiPlayer = this.ai.getSelf();

        // AI落子
        if (!this.makeMove(x, y, aiPlayer)) {
            this.showMessage('AI落子失败！', 'error');
            return;
        }

        // 检查游戏是否结束
        this.checkGameEnd(x, y, aiPlayer);
    }

    async makeCPPMove() {
        const aiPlayer = (this.playerColor === Player.BLACK) ? Player.WHITE : Player.BLACK;

        // 构建请求数据
        const requestData = {
            boardSize: this.boardSize,
            currentTurn: this.currentTurn,
            selfPlayer: aiPlayer,
            algorithm: this.aiAlgorithm,
            searchDepth: this.searchDepth,
            iterations: this.iterations,
            history: this.moveHistory.map(m => ({ x: m.x, y: m.y })),
            engine: 'cpp'
        };

        try {
            const response = await fetch(`${this.apiBaseUrl}/api/move`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(requestData)
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }

            const result = await response.json();
            const endTime = performance.now();

            if (!result.success) {
                throw new Error(result.error || 'C++返回错误');
            }

            const { x, y } = result.move;

            // 更新统计信息
            document.getElementById('nodeCount').textContent =
                (result.nodesSearched || 0).toLocaleString();
            document.getElementById('thinkTime').textContent =
                `${result.thinkTime || Math.round(endTime - startTime)}ms`;
            document.getElementById('currentTurn').textContent +=
                ` (C++ Engine)`;

            if (x === -1 || y === -1) {
                this.showMessage('C++ AI无法找到有效位置！', 'error');
                return;
            }

            // AI落子
            if (!this.makeMove(x, y, aiPlayer)) {
                this.showMessage('C++ AI落子失败！', 'error');
                return;
            }

            // 检查游戏是否结束
            this.checkGameEnd(x, y, aiPlayer);

        } catch (error) {
            if (error.name === 'TypeError' && error.message.includes('fetch')) {
                throw new Error(`无法连接到API服务器 (${this.apiBaseUrl})，请确认已启动 start.bat`);
            }
            throw error;
        }
    }

    checkGameEnd(x, y, player) {
        // 检查胜利
        if (this.gomoku.Win(x, y, player)) {
            this.gameOver = true;
            const winnerName = player === Player.BLACK ? '⚫ 黑棋' : '⚪ 白棋';

            if (this.gameMode === 'pve') {
                if (player === this.playerColor) {
                    this.showMessage(`🎉 恭喜！你赢了！（${winnerName}胜）`, 'success');
                } else {
                    this.showMessage(`😔 很遗憾，AI赢了！（${winnerName}胜）`, 'warning');
                }
            } else {
                this.showMessage(`🎉 游戏结束！${winnerName}获胜！`, 'success');
            }

            this.updateButtons();
            return true;
        }

        // 检查平局
        if (this.gomoku.GameOver()) {
            this.gameOver = true;
            this.showMessage('🤝 平局！棋盘已满', 'info');
            this.updateButtons();
            return true;
        }

        return false;
    }

    undo() {
        if (this.moveHistory.length === 0) {
            this.showMessage('没有可悔的棋步！', 'warning');
            return;
        }

        if (this.isAIThinking) {
            this.showMessage('AI思考中，无法悔棋！', 'warning');
            return;
        }

        // 在PvE模式下，需要撤销两步（玩家+AI）
        let stepsToUndo = 1;
        if (this.gameMode === 'pve') {
            stepsToUndo = 2;
        }

        stepsToUndo = Math.min(stepsToUndo, this.moveHistory.length);

        for (let i = 0; i < stepsToUndo; i++) {
            const move = this.moveHistory.pop();
            if (move) {
                this.gomoku.undo(move.x, move.y);
                this.currentTurn--;
            }
        }

        // 更新最后一步标记
        if (this.moveHistory.length > 0) {
            const lastMove = this.moveHistory[this.moveHistory.length - 1];
            this.ui.setLastMove(lastMove.x, lastMove.y);
        } else {
            this.ui.clearLastMove();
        }

        this.gameOver = false;
        this.ui.render(this.gomoku);
        this.updateStatus();
        this.showMessage('已悔棋', 'info');

        console.log(`悔棋: 撤销了 ${stepsToUndo} 步`);
    }

    getCurrentPlayer() {
        return (this.currentTurn & 1) ? Player.WHITE : Player.BLACK;
    }

    updateStatus() {
        const currentP = this.getCurrentPlayer();
        const turnText = currentP === Player.BLACK ? '⚫ 黑棋' : '⚪ 白棋';
        document.getElementById('currentTurn').textContent = turnText;
        document.getElementById('moveCount').textContent = this.moveHistory.length.toString();
    }

    updateButtons() {
        const undoBtn = document.getElementById('undoBtn');
        undoBtn.disabled = this.moveHistory.length === 0 || this.isAIThinking || this.gameOver;
    }

    showMessage(message, type = 'info') {
        const msgEl = document.getElementById('gameMessage');
        msgEl.textContent = message;
        msgEl.className = `game-message ${type}`;
    }

    getStats() {
        return {
            boardSize: this.boardSize,
            gameMode: this.gameMode,
            moveCount: this.moveHistory.length,
            currentTurn: this.currentTurn,
            isGameOver: this.gameOver,
            aiAlgorithm: this.aiAlgorithm,
            nodesSearched: this.ai ? this.ai.getNodesSearched() : 0
        };
    }

    getAdaptiveAIParams(boardSize) {
        // 根据棋盘规格返回推荐的AI参数上限
        // 目标：保证响应时间 < 3秒（普通浏览器）
        const params = [
            { maxSize: 10, maxDepth: 6, maxIterations: 5000 },
            { maxSize: 15, maxDepth: 4, maxIterations: 2000 },
            { maxSize: 19, maxDepth: 3, maxIterations: 1000 },
            { maxSize: 25, maxDepth: 2, maxIterations: 500 },
            { maxSize: 30, maxDepth: 2, maxIterations: 300 },
            { maxSize: 38, maxDepth: 2, maxIterations: 200 }
        ];

        for (const p of params) {
            if (boardSize <= p.maxSize) {
                return { maxDepth: p.maxDepth, maxIterations: p.maxIterations };
            }
        }

        // 超大规格：最保守参数
        return { maxDepth: 2, maxIterations: 100 };
    }
}
