class GomokuUI {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.cellSize = 40;
        this.padding = 30;
        this.pieceRadius = 18;
        this.hoverPos = null;
        this.lastMove = null;
        this.boardSize = 15;
        this.currentGomoku = null; // 保存当前棋盘状态

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

        // 根据棋盘大小动态调整单元格大小
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
        } else {
            this.cellSize = 22;
            this.pieceRadius = 9;
        }

        this.padding = Math.max(25, this.cellSize);
        this.setupCanvas();
    }

    bindEvents() {
        this.canvas.addEventListener('mousemove', (e) => this.handleMouseMove(e));
        this.canvas.addEventListener('mouseleave', () => this.handleMouseLeave());
        this.canvas.addEventListener('click', (e) => this.handleClick(e));

        // 节流：限制鼠标移动事件频率（每50ms最多触发一次）
        this.lastMouseMoveTime = 0;
        this.mouseMoveThrottle = 50;  // ms
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
        // 节流：限制渲染频率
        const now = performance.now();
        if (now - this.lastMouseMoveTime < this.mouseMoveThrottle) {
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

        // 清空画布
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

        // 绘制棋盘背景
        this.ctx.fillStyle = '#DEB887';
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

        // 绘制网格线（优化：批量绘制）
        this.ctx.strokeStyle = '#000000';
        this.ctx.lineWidth = 1;
        this.ctx.beginPath();

        for (let i = 0; i < this.boardSize; i++) {
            const x = this.padding + i * this.cellSize;
            const y = this.padding + i * this.cellSize;

            // 垂直线
            this.ctx.moveTo(x, this.padding);
            this.ctx.lineTo(x, this.padding + (this.boardSize - 1) * this.cellSize);

            // 水平线
            this.ctx.moveTo(this.padding, y);
            this.ctx.lineTo(this.padding + (this.boardSize - 1) * this.cellSize, y);
        }
        this.ctx.stroke();

        // 绘制星位（天元和星）
        this.drawStarPoints();

        // 绘制棋子
        if (gomoku) {
            for (let x = 1; x <= this.boardSize; x++) {
                for (let y = 1; y <= this.boardSize; y++) {
                    const color = gomoku.getColor(x, y);
                    if (color !== Player.NONE) {
                        this.drawPiece(x, y, color);
                    }
                }
            }
        }

        // 绘制最后一步标记
        if (this.lastMove) {
            this.drawLastMoveMarker(this.lastMove.x, this.lastMove.y);
        }

        // 绘制悬停提示
        if (this.hoverPos) {
            this.drawHoverPiece(this.hoverPos.x, this.hoverPos.y);
        }

        const renderTime = performance.now() - renderStart;
        if (renderTime > 50) {  // 只在渲染慢时记录
            console.log(`⚠️ 棋盘渲染耗时: ${Math.round(renderTime)}ms`);
        }
    }

    drawStarPoints() {
        this.ctx.fillStyle = '#000000';

        const center = Math.floor((this.boardSize + 1) / 2);
        const points = [];

        if (this.boardSize >= 9) {
            points.push([center, center]); // 天元
        }

        if (this.boardSize >= 13) {
            const offset = Math.floor((this.boardSize - 1) / 4);
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
            this.ctx.beginPath();
            this.ctx.arc(px, py, 4, 0, Math.PI * 2);
            this.ctx.fill();
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
