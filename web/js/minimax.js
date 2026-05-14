class Minimax {
    constructor(selfValue, maxDepthValue = 4, defendWeightValue = 1.1) {
        this.self = selfValue;
        this.opponent = (selfValue === Player.BLACK) ? Player.WHITE : Player.BLACK;
        this.maxDepth = maxDepthValue;
        this.nodeCount = 0;
        this.defendWeight = defendWeightValue;
        this.openingWeight = 0.8;
        this.middleWeight = 1.0;
        this.endgameWeight = 1.2;

        if (this.self === Player.BLACK) {
            this.attackCoeff = 1.2;
            this.defenseCoeff = 1.0;
        } else {
            this.attackCoeff = 1.0;
            this.defenseCoeff = 1.3;
        }
    }

    basePatternScore(pattern) {
        switch (pattern.form()) {
            case PatternType.FIVE:
            case PatternType.OVERLINE:
                return 10000000;
            case PatternType.FLEX4:
                return 5000000;
            case PatternType.BLOCK4:
                return 3000000;
            case PatternType.FLEX3:
                return 50000;
            case PatternType.BLOCK3:
                return 1000;
            case PatternType.FLEX2:
                return 200;
            case PatternType.BLOCK2:
                return 50;
            case PatternType.FLEX1:
                return 10;
            case PatternType.BLOCK1:
                return 2;
            default:
                return 0;
        }
    }

    positionScore(g, x, y) {
        const size = g.getSize();
        const center = Math.floor((size + 1) / 2);
        const distX = Math.abs(x - center);
        const distY = Math.abs(y - center);
        const dist = Math.max(distX, distY);
        const maxDist = Math.floor(size / 2);

        if (dist >= maxDist) return 1;
        return (maxDist - dist + 1) * 5;
    }

    getStageWeight(g) {
        const totalCells = g.getSize() * g.getSize();
        const placed = g.getCurrentCount();

        if (placed < 10) return this.openingWeight;
        if (placed < totalCells * 0.6) return this.middleWeight;
        return this.endgameWeight;
    }

    evaluatePiece(g, x, y, player) {
        if (g.getColor(x, y) !== player) return 0;

        let score = 0;
        for (let [dx, dy] of Config.directions) {
            const pattern = g.analyzeForm(x, y, dx, dy, player);
            score += this.basePatternScore(pattern);
        }

        const stageWeight = this.getStageWeight(g);
        if (stageWeight < 1.0) score += this.positionScore(g, x, y);

        return score;
    }

    evaluateEmpty(g, x, y) {
        if (g.getColor(x, y) !== Player.NONE) return 0;

        let score = 0;
        for (let [dx, dy] of Config.directions) {
            const patternSelf = g.analyzeForm(x, y, dx, dy, this.self);
            const patternOpponent = g.analyzeForm(x, y, dx, dy, this.opponent);
            
            score += this.basePatternScore(patternSelf);
            score += this.basePatternScore(patternOpponent);
        }
        
        score += this.positionScore(g, x, y);

        return score;
    }

    evaluate(g) {
        let selfScore = 0;
        let opponentScore = 0;
        const size = g.getSize();
        const evaluated = new Array(size * size).fill(false);
        const stageWeight = this.getStageWeight(g);

        for (let x = 1; x <= size; x++) {
            for (let y = 1; y <= size; y++) {
                const p = g.getColor(x, y);
                if (p === Player.NONE) continue;

                const idx = (x - 1) * size + (y - 1);
                if (evaluated[idx]) continue;

                if (p === this.self) {
                    const pieceScore = this.evaluatePiece(g, x, y, this.self);
                    selfScore += Math.floor(pieceScore * this.attackCoeff * stageWeight);
                } else if (p === this.opponent) {
                    const pieceScore = this.evaluatePiece(g, x, y, this.opponent);
                    opponentScore += Math.floor(pieceScore * this.defenseCoeff * stageWeight);
                }

                evaluated[idx] = true;

                for (let [dx, dy] of Config.directions) {
                    let nx = x + dx, ny = y + dy;
                    while (!g.outOfRange(nx, ny) && g.getColor(nx, ny) === p) {
                        evaluated[(nx - 1) * size + (ny - 1)] = true;
                        nx += dx;
                        ny += dy;
                    }
                    nx = x - dx;
                    ny = y - dy;
                    while (!g.outOfRange(nx, ny) && g.getColor(nx, ny) === p) {
                        evaluated[(nx - 1) * size + (ny - 1)] = true;
                        nx -= dx;
                        ny -= dy;
                    }
                }
            }
        }

        const val = selfScore - opponentScore * this.defendWeight;
        return Math.floor(val);
    }

    getCandidateMoves(g) {
        const size = g.getSize();
        const visited = new Array(size * size).fill(false);
        const candidates = [];

        for (let x = 1; x <= size; x++) {
            for (let y = 1; y <= size; y++) {
                if (g.getColor(x, y) !== Player.NONE) {
                    for (let dx = -2; dx <= 2; dx++) {
                        for (let dy = -2; dy <= 2; dy++) {
                            const nx = x + dx, ny = y + dy;
                            if (!g.outOfRange(nx, ny) && g.getColor(nx, ny) === Player.NONE) {
                                const idx = (nx - 1) * size + (ny - 1);
                                if (!visited[idx]) {
                                    candidates.push([nx, ny]);
                                    visited[idx] = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (candidates.length === 0) {
            candidates.push([Math.floor((size + 1) / 2), Math.floor((size + 1) / 2)]);
            return candidates;
        }

        // 必防点检测：对手在此落子是否立即获胜（五连）
        const mustDefendPoints = [];
        for (const [x, y] of candidates) {
            if (g.getColor(x, y) === Player.NONE) {
                // 检查4个方向：如果对手在此落子是否形成五连
                for (const [dx, dy] of Config.directions) {
                    let count = 1;
                    let nx = x + dx, ny = y + dy;
                    while (!g.outOfRange(nx, ny) && g.getColor(nx, ny) === this.opponent) { count++; nx += dx; ny += dy; }
                    nx = x - dx; ny = y - dy;
                    while (!g.outOfRange(nx, ny) && g.getColor(nx, ny) === this.opponent) { count++; nx -= dx; ny -= dy; }
                    if (count >= 5) {
                        mustDefendPoints.push([x, y]);
                        break;
                    }
                }
            }
        }

        // 同时检测活四威胁：对手在此落子形成活四（下一步必胜）
        for (const [x, y] of candidates) {
            let alreadyMustDefend = false;
            for (const pt of mustDefendPoints) { if (pt[0] === x && pt[1] === y) { alreadyMustDefend = true; break; } }
            if (alreadyMustDefend) continue;

            for (const [dx, dy] of Config.directions) {
                const patternOpponent = g.analyzeForm(x, y, dx, dy, this.opponent);
                if (patternOpponent.isFlex4()) {
                    mustDefendPoints.push([x, y]);
                    break;
                }
            }
        }

        candidates.sort((a, b) => {
            return this.evaluateEmpty(g, b[0], b[1]) - this.evaluateEmpty(g, a[0], a[1]);
        });

        // 强制包含必防点，然后补充其他高分位置
        const result = [];
        const added = new Set();
        
        // 先添加所有必防点
        for (const pt of mustDefendPoints) {
            const key = `${pt[0]},${pt[1]}`;
            if (!added.has(key)) {
                result.push(pt);
                added.add(key);
            }
        }
        
        // 再添加其他候选点（最多20个）
        for (const pt of candidates) {
            const key = `${pt[0]},${pt[1]}`;
            if (!added.has(key)) {
                result.push(pt);
                added.add(key);
            }
            if (result.length >= 20) break;
        }
        
        return result;
    }

    alphaBeta(g, depth, alpha, beta, isMaximizing) {
        this.nodeCount++;

        if (depth === 0 || g.GameOver()) return this.evaluate(g);

        const moves = this.getCandidateMoves(g);
        if (moves.length === 0) return 0;

        if (isMaximizing) {
            let maxValue = Number.MIN_SAFE_INTEGER;
            for (const [x, y] of moves) {
                if (!g.validPosition(x, y, this.self)) continue;

                g.set(x, y, this.self);
                if (g.Win(x, y, this.self)) {
                    g.undo(x, y);
                    return 10000000 + depth;
                }

                const value = this.alphaBeta(g, depth - 1, alpha, beta, false);
                g.undo(x, y);

                maxValue = Math.max(maxValue, value);
                alpha = Math.max(alpha, value);
                if (beta <= alpha) break;
            }
            return maxValue;
        } else {
            let minValue = Number.MAX_SAFE_INTEGER;
            for (const [x, y] of moves) {
                if (!g.validPosition(x, y, this.opponent)) continue;

                g.set(x, y, this.opponent);
                if (g.Win(x, y, this.opponent)) {
                    g.undo(x, y);
                    return -(10000000 + depth);
                }

                const value = this.alphaBeta(g, depth - 1, alpha, beta, true);
                g.undo(x, y);

                minValue = Math.min(minValue, value);
                beta = Math.min(beta, value);
                if (beta <= alpha) break;
            }
            return minValue;
        }
    }

    getBestMove(g) {
        this.nodeCount = 0;
        let bestScore = Number.MIN_SAFE_INTEGER;
        let bestMove = [-1, -1];

        const moves = this.getCandidateMoves(g);

        for (const [x, y] of moves) {
            if (!g.validPosition(x, y, this.self)) continue;

            g.set(x, y, this.self);
            if (g.Win(x, y, this.self)) {
                g.undo(x, y);
                return [x, y];
            }

            const value = this.alphaBeta(g, this.maxDepth - 1, Number.MIN_SAFE_INTEGER, Number.MAX_SAFE_INTEGER, false);
            g.undo(x, y);

            if (value > bestScore) {
                bestScore = value;
                bestMove = [x, y];
            }
        }

        return bestMove;
    }

    setDepth(depth) {
        this.maxDepth = depth;
    }

    getNodesSearched() {
        return this.nodeCount;
    }

    getSelf() {
        return this.self;
    }
}
