class GomokuUI {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.cellSize = 40;
        this.padding = 30;
        this.pieceRadius = 18;
        this.hoverPos = null;
        this.lastMove = null;
        this.forbiddenPos = null;  // 禁手位置高亮
        this.boardSize = 15;
        this.currentGomoku = null;

        // 渲染性能优化：缓存和状态跟踪
        this.renderCache = null;          // 缓存静态层（背景+网格）
        this.lastRenderState = null;      // 上次渲染的状态哈希
        this.isLargeBoard = false;        // 大规格标记
        this.maxDisplaySize = 700;        // 最大显示尺寸（像素）
        this.scaleFactor = 1.0;           // 缩放因子

        this.setupCanvas();
        this.bindEvents();
    }

    setupCanvas() {
        const size = this.padding * 2 + this.cellSize * (this.boardSize - 1);
        this.canvas.width = size;
        this.canvas.height = size;
    }

    setBoardSize(size) {
        this.boardSize = size;

        // 根据棋盘大小动态调整单元格大小（优化：支持更大规格）
        if (size <= 10) {
            this.cellSize = 50;
            this.pieceRadius = 22;
        } else if (size <= 15) {
            this.cellSize = 40;
            this.pieceRadius = 18;
        } else if (size <= 19) {
            this.cellSize = 34;
            this.pieceRadius = 15;
        } else if (size <= 25) {
            this.cellSize = 28;
            this.pieceRadius = 12;
        } else if (size <= 30) {
            this.cellSize = 24;
            this.pieceRadius = 10;
        } else {
            this.cellSize = 20;
            this.pieceRadius = 8;
        }

        this.padding = Math.max(20, Math.floor(this.cellSize * 0.7));
        this.setupCanvas();

        // 大规格标记和缩放计算
        this.isLargeBoard = size > 19;
        
        // 计算实际Canvas尺寸
        const actualSize = this.padding * 2 + this.cellSize * (this.boardSize - 1);
        
        // 如果超出最大显示尺寸，设置CSS缩放
        if (actualSize > this.maxDisplaySize) {
            this.scaleFactor = this.maxDisplaySize / actualSize;
            this.canvas.style.width = `${this.maxDisplaySize}px`;
            this.canvas.style.height = `${this.maxDisplaySize}px`;
        } else {
            this.scaleFactor = 1.0;
            this.canvas.style.width = `${actualSize}px`;
            this.canvas.style.height = `${actualSize}px`;
        }

        // 清除缓存（规格变化后需要重绘）
        this.renderCache = null;
        this.lastRenderState = null;

        console.log(`📐 棋盘: ${size}×${size}, 单元格: ${this.cellSize}px, Canvas: ${actualSize}px, 缩放: ${(this.scaleFactor * 100).toFixed(0)}%`);
    }

    bindEvents() {
        this.canvas.addEventListener('mousemove', (e) => this.handleMouseMove(e));
        this.canvas.addEventListener('mouseleave', () => this.handleMouseLeave());
        this.canvas.addEventListener('click', (e) => this.handleClick(e));

        // 节流：限制鼠标移动事件频率（大规格棋盘需要更长间隔）
        this.lastMouseMoveTime = 0;
        this.mouseMoveThrottle = 50;  // ms（默认值）
    }

    getMousePos(e) {
        const rect = this.canvas.getBoundingClientRect();
        const scaleX = this.canvas.width / rect.width;
        const scaleY = this.canvas.height / rect.height;

        return {
            x: (e.clientX - rect.left) * scaleX,
            y: (e.clientY - rect.top) * scaleY
        };
    }

    boardPosToPixel(x, y) {
        return {
            px: this.padding + (x - 1) * this.cellSize,
            py: this.padding + (y - 1) * this.cellSize
        };
    }

    pixelToBoardPos(mousePx, mousePy) {
        const x = Math.round((mousePx - this.padding) / this.cellSize) + 1;
        const y = Math.round((mousePy - this.padding) / this.cellSize) + 1;

        if (x < 1 || x > this.boardSize || y < 1 || y > this.boardSize) {
            return null;
        }

        return { x, y };
    }

    handleMouseMove(e) {
        // 动态节流：大规格棋盘使用更长间隔
        const dynamicThrottle = this.isLargeBoard ? 100 : 50;
        
        // 节流：限制渲染频率
        const now = performance.now();
        if (now - this.lastMouseMoveTime < dynamicThrottle) {
            return;
        }
        this.lastMouseMoveTime = now;

        const pos = this.getMousePos(e);
        const boardPos = this.pixelToBoardPos(pos.x, pos.y);

        if (boardPos) {
            const { px, py } = this.boardPosToPixel(boardPos.x, boardPos.y);
            const dist = Math.sqrt((pos.x - px) ** 2 + (pos.y - py) ** 2);

            if (dist < this.cellSize * 0.4) {
                if (!this.hoverPos || this.hoverPos.x !== boardPos.x || this.hoverPos.y !== boardPos.y) {
                    this.hoverPos = boardPos;
                    this.render();
                }
                return;
            }
        }

        if (this.hoverPos) {
            this.hoverPos = null;
            this.render();
        }
    }

    handleMouseLeave() {
        this.hoverPos = null;
        this.render();
    }

    handleClick(e) {
        const pos = this.getMousePos(e);
        const boardPos = this.pixelToBoardPos(pos.x, pos.y);

        if (boardPos && this.hoverPos) {
            const { px, py } = this.boardPosToPixel(boardPos.x, boardPos.y);
            const dist = Math.sqrt((pos.x - px) ** 2 + (pos.y - py) ** 2);

            if (dist < this.cellSize * 0.4) {
                // 触发回调
                if (this.onMoveCallback) {
                    this.onMoveCallback(boardPos.x, boardPos.y);
                }
            }
        }
    }

    setOnMove(callback) {
        this.onMoveCallback = callback;
    }

    drawBoard(gomoku) {
        const renderStart = performance.now();

        // 生成当前状态哈希（用于缓存比对）
        const currentState = this.generateStateHash(gomoku);
        
        // 检查是否需要完全重绘静态层（只在规格变化时重绘）
        if (!this.renderCache || this.needFullRedraw(currentState)) {
            const staticCanvas = this.drawStaticLayer();
            // 合并缓存：保留canvas引用 + 更新状态信息
            this.renderCache = {
                canvas: staticCanvas,
                ...currentState,
                hasStatic: true
            };
        }

        // 清空画布
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

        // 绘制缓存的静态层（背景+网格+星位）
        if (this.renderCache && this.renderCache.canvas) {
            this.ctx.drawImage(this.renderCache.canvas, 0, 0);
        } else {
            // 缓存失败时的降级方案：直接绘制
            console.warn('⚠️ 静态层缓存不可用，使用降级渲染');
            this.drawFallbackBackground();
        }

        // 绘制棋子（优化：只绘制非空位置）
        if (gomoku) {
            const totalCells = this.boardSize * this.boardSize;
            
            // 大规格棋盘：使用简化渲染模式
            if (this.isLargeBoard || totalCells > 400) {
                this.drawPiecesOptimized(gomoku);
            } else {
                this.drawPiecesNormal(gomoku);
            }
        }

        // 绘制最后一步标记
        if (this.lastMove) {
            this.drawLastMoveMarker(this.lastMove.x, this.lastMove.y);
        }

        // 绘制禁手位置高亮警告
        if (this.forbiddenPos) {
            this.drawForbiddenMarker(this.forbiddenPos.x, this.forbiddenPos.y);
        }

        // 绘制悬停提示
        if (this.hoverPos) {
            this.drawHoverPiece(this.hoverPos.x, this.hoverPos.y);
        }

        const renderTime = performance.now() - renderStart;
        if (renderTime > 50) {  // 只在渲染慢时记录
            console.log(`⚠️ 棋盘渲染耗时: ${Math.round(renderTime)}ms (${this.boardSize}×${this.boardSize})`);
        }
    }

    generateStateHash(gomoku) {
        return {
            boardSize: this.boardSize,
            pieceCount: gomoku ? gomoku.getCurrentCount() : 0,
            lastMove: this.lastMove ? `${this.lastMove.x},${this.lastMove.y}` : null,
            hoverPos: this.hoverPos ? `${this.hoverPos.x},${this.hoverPos.y}` : null,
            forbiddenPos: this.forbiddenPos ? `${this.forbiddenPos.x},${this.forbiddenPos.y}` : null
        };
    }

    needFullRedraw(newState) {
        if (!this.lastRenderState) return true;
        
        // 只在棋盘大小变化时重绘静态层
        return this.lastRenderState.boardSize !== newState.boardSize;
    }

    drawStaticLayer() {
        // 创建离屏Canvas用于缓存静态层
        const offscreen = document.createElement('canvas');
        offscreen.width = this.canvas.width;
        offscreen.height = this.canvas.height;
        const ctx = offscreen.getContext('2d');

        // 绘制背景
        ctx.fillStyle = '#DEB887';
        ctx.fillRect(0, 0, offscreen.width, offscreen.height);

        // 绘制网格线（批量绘制）
        ctx.strokeStyle = '#000000';
        ctx.lineWidth = 1;
        ctx.beginPath();

        for (let i = 0; i < this.boardSize; i++) {
            const x = this.padding + i * this.cellSize;
            const y = this.padding + i * this.cellSize;

            // 垂直线
            ctx.moveTo(x, this.padding);
            ctx.lineTo(x, this.padding + (this.boardSize - 1) * this.cellSize);

            // 水平线
            ctx.moveTo(this.padding, y);
            ctx.lineTo(this.padding + (this.boardSize - 1) * this.cellSize, y);
        }
        ctx.stroke();

        // 绘制星位
        this.drawStarPointsToContext(ctx);

        // 返回离屏Canvas（由调用者管理缓存）
        return offscreen;
    }

    drawFallbackBackground() {
        // 降级渲染：当缓存不可用时直接绘制背景
        this.ctx.fillStyle = '#DEB887';
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

        // 绘制网格线
        this.ctx.strokeStyle = '#000000';
        this.ctx.lineWidth = 1;
        this.ctx.beginPath();

        for (let i = 0; i < this.boardSize; i++) {
            const x = this.padding + i * this.cellSize;
            const y = this.padding + i * this.cellSize;

            this.ctx.moveTo(x, this.padding);
            this.ctx.lineTo(x, this.padding + (this.boardSize - 1) * this.cellSize);

            this.ctx.moveTo(this.padding, y);
            this.ctx.lineTo(this.padding + (this.boardSize - 1) * this.cellSize, y);
        }
        this.ctx.stroke();

        // 绘制星位
        this.drawStarPoints();
    }

    drawPiecesNormal(gomoku) {
        for (let x = 1; x <= this.boardSize; x++) {
            for (let y = 1; y <= this.boardSize; y++) {
                const color = gomoku.getColor(x, y);
                if (color !== Player.NONE) {
                    this.drawPiece(x, y, color);
                }
            }
        }
    }

    drawPiecesOptimized(gomoku) {
        // 大规格优化：使用简化的棋子绘制（无阴影、无渐变）
        const size = gomoku.getSize();
        
        for (let x = 1; x <= size; x++) {
            for (let y = 1; y <= size; y++) {
                const color = gomoku.getColor(x, y);
                if (color !== Player.NONE) {
                    this.drawPieceSimple(x, y, color);
                }
            }
        }
    }

    drawPieceSimple(x, y, player) {
        const { px, py } = this.boardPosToPixel(x, y);
        const radius = Math.max(3, this.pieceRadius);  // 最小半径保护

        this.ctx.beginPath();
        this.ctx.arc(px, py, radius, 0, Math.PI * 2);

        // 简化颜色：无渐变，纯色填充
        if (player === Player.BLACK) {
            this.ctx.fillStyle = '#000000';
        } else {
            this.ctx.fillStyle = '#FFFFFF';
            this.ctx.strokeStyle = '#999999';
            this.ctx.lineWidth = 1;
        }

        this.ctx.fill();
        
        // 白子加边框
        if (player === Player.WHITE) {
            this.ctx.stroke();
        }
    }

    drawStarPoints() {
        this.drawStarPointsToContext(this.ctx);
    }

    drawStarPointsToContext(ctx) {
        ctx.fillStyle = '#000000';

        const center = Math.floor((this.boardSize + 1) / 2);
        const points = [];
        
        // 预计算星位偏移量（函数级别，避免作用域错误）
        let offset = 0;
        if (this.boardSize >= 13) {
            offset = Math.floor((this.boardSize - 1) / 4);
        }

        if (this.boardSize >= 9) {
            points.push([center, center]); // 天元
        }

        if (this.boardSize >= 13) {
            points.push(
                [offset + 1, offset + 1],
                [offset + 1, this.boardSize - offset],
                [this.boardSize - offset, offset + 1],
                [this.boardSize - offset, this.boardSize - offset]
            );
        }

        if (this.boardSize >= 19) {
            const offset3 = Math.floor((this.boardSize - 1) / 2);
            points.push(
                [center, offset + 1],
                [center, this.boardSize - offset],
                [offset + 1, center],
                [this.boardSize - offset, center]
            );
        }

        for (const [x, y] of points) {
            const { px, py } = this.boardPosToPixel(x, y);
            ctx.beginPath();
            ctx.arc(px, py, 4, 0, Math.PI * 2);
            ctx.fill();
        }
    }

    drawPiece(x, y, player) {
        const { px, py } = this.boardPosToPixel(x, y);

        // 绘制阴影
        this.ctx.shadowColor = 'rgba(0, 0, 0, 0.5)';
        this.ctx.shadowBlur = 4;
        this.ctx.shadowOffsetX = 2;
        this.ctx.shadowOffsetY = 2;

        // 绘制棋子
        this.ctx.beginPath();
        this.ctx.arc(px, py, this.pieceRadius, 0, Math.PI * 2);

        if (player === Player.BLACK) {
            const gradient = this.ctx.createRadialGradient(
                px - this.pieceRadius * 0.3, py - this.pieceRadius * 0.3,
                0,
                px, py, this.pieceRadius
            );
            gradient.addColorStop(0, '#555555');
            gradient.addColorStop(1, '#000000');
            this.ctx.fillStyle = gradient;
        } else {
            const gradient = this.ctx.createRadialGradient(
                px - this.pieceRadius * 0.3, py - this.pieceRadius * 0.3,
                0,
                px, py, this.pieceRadius
            );
            gradient.addColorStop(0, '#FFFFFF');
            gradient.addColorStop(1, '#CCCCCC');
            this.ctx.fillStyle = gradient;
        }

        this.ctx.fill();

        // 重置阴影
        this.ctx.shadowColor = 'transparent';
        this.ctx.shadowBlur = 0;
        this.ctx.shadowOffsetX = 0;
        this.ctx.shadowOffsetY = 0;
    }

    drawLastMoveMarker(x, y) {
        const { px, py } = this.boardPosToPixel(x, y);

        this.ctx.strokeStyle = '#FF0000';
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();

        const size = 6;
        this.ctx.moveTo(px - size, py);
        this.ctx.lineTo(px + size, py);
        this.ctx.moveTo(px, py - size);
        this.ctx.lineTo(px, py + size);

        this.ctx.stroke();
    }

    drawForbiddenMarker(x, y) {
        const { px, py } = this.boardPosToPixel(x, y);
        const radius = this.cellSize * 0.4;

        this.ctx.save();
        this.ctx.globalAlpha = 0.8 + Math.sin(Date.now() / 150) * 0.2;

        this.ctx.beginPath();
        this.ctx.arc(px, py, radius, 0, Math.PI * 2);
        this.ctx.fillStyle = 'rgba(255, 69, 0, 0.6)';
        this.ctx.fill();
        this.ctx.strokeStyle = '#FF4500';
        this.ctx.lineWidth = 3;
        this.ctx.stroke();

        this.ctx.fillStyle = '#FFFFFF';
        this.ctx.font = `bold ${Math.max(12, this.cellSize * 0.4)}px Arial`;
        this.ctx.textAlign = 'center';
        this.ctx.textBaseline = 'middle';
        this.ctx.fillText('✕', px, py);

        this.ctx.restore();
    }

    drawHoverPiece(x, y) {
        const { px, py } = this.boardPosToPixel(x, y);

        this.ctx.globalAlpha = 0.5;
        this.ctx.beginPath();
        this.ctx.arc(px, py, this.pieceRadius, 0, Math.PI * 2);
        this.ctx.fillStyle = '#00FF00';
        this.ctx.fill();
        this.ctx.globalAlpha = 1.0;
    }

    setLastMove(x, y) {
        this.lastMove = { x, y };
    }

    clearLastMove() {
        this.lastMove = null;
    }

    highlightForbiddenPosition(x, y) {
        this.forbiddenPos = { x, y };
        this.render();
        setTimeout(() => {
            this.clearForbiddenHighlight();
        }, 2000);
    }

    clearForbiddenHighlight() {
        if (this.forbiddenPos) {
            this.forbiddenPos = null;
            this.render();
        }
    }

    render(gomoku = null) {
        if (gomoku) {
            this.currentGomoku = gomoku;
        }
        this.drawBoard(this.currentGomoku);
    }

    showThinking() {
        // 可以在这里添加思考动画
        console.log('AI is thinking...');
    }

    hideThinking() {
        console.log('AI finished thinking');
    }
}
