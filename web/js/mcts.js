class MCTSNode {
    constructor(x = -1, y = -1, player = Player.NONE, parent = null) {
        this.x = x;
        this.y = y;
        this.player = player;
        this.visits = 0;
        this.wins = 0;
        this.children = [];
        this.parent = parent;
    }

    isLeaf() {
        return this.children.length === 0;
    }

    ucb(exploration) {
        if (this.visits === 0) return 1e9; // 未访问节点优先探索
        return (this.wins / this.visits) + exploration * Math.sqrt(2 * Math.log(this.parent.visits + 1) / this.visits);
    }
}

class MCTS {
    constructor(selfValue, iterationsValue = 1000, explorationValue = 1.414) {
        this.self = selfValue;
        this.opponent = (selfValue === Player.BLACK) ? Player.WHITE : Player.BLACK;
        this.iterations = iterationsValue;
        this.exploration = explorationValue;
        this.nodeCount = 0;

        // 使用 xorshift128+ 高质量伪随机数生成器（接近 C++ std::mt19937 质量）
        this.s0 = Date.now() || 1;
        this.s1 = (Date.now() >> 31) || 1;
        if ((this.s0 & 0xffffffff) === 0) this.s0 = 1;
        if ((this.s1 & 0xffffffff) === 0) this.s1 = 2;
    }

    // xorshift128+ 随机数生成算法（高质量）
    random() {
        let s1 = this.s0;
        const s0 = this.s1;
        this.s0 = s0;
        s1 ^= s1 << 23;
        s1 ^= s1 >> 17;
        s1 ^= s0;
        s1 ^= s0 >> 26;
        this.s1 = s1;
        return (this.s0 + this.s1) / 4294967296;  // 归一化到 [0, 1)
    }

    randint(min, max) {
        return Math.floor(this.random() * (max - min + 1)) + min;
    }

    quickEval(g, x, y, player) {
        let score = 0;
        const opp = (player === Player.BLACK) ? Player.WHITE : Player.BLACK;

        for (let [dx, dy] of Config.directions) {
            const p = g.analyzeForm(x, y, dx, dy, player);
            const oppP = g.analyzeForm(x, y, dx, dy, opp);

            if (p.count >= 5) score += 100000;
            else if (p.count === 4 && p.openEnds === 2) score += 10000;
            else if (p.count === 4 && p.openEnds === 1) score += 1000;
            else if (p.count === 3 && p.openEnds === 2) score += 500;
            else if (p.count === 3 && p.openEnds === 1) score += 100;
            else if (p.count === 2 && p.openEnds === 2) score += 50;
            else if (p.count === 2 && p.openEnds === 1) score += 10;
            else if (p.count === 1 && p.openEnds === 2) score += 5;

            if (oppP.count >= 5) score += 50000;
            else if (oppP.count === 4 && oppP.openEnds === 2) score += 5000;
            else if (oppP.count === 4 && oppP.openEnds === 1) score += 800;
            else if (oppP.count === 3 && oppP.openEnds === 2) score += 300;
            else if (oppP.count === 3 && oppP.openEnds === 1) score += 50;
        }

        return score;
    }

    getCandidateMoves(g, range, maxCount) {
        const size = g.getSize();
        const visited = new Array(size * size).fill(false);
        const candidates = [];

        for (let x = 1; x <= size; x++) {
            for (let y = 1; y <= size; y++) {
                if (g.getColor(x, y) !== Player.NONE) {
                    for (let dx = -range; dx <= range; dx++) {
                        for (let dy = -range; dy <= range; dy++) {
                            if (dx === 0 && dy === 0) continue;
                            const nx = x + dx, ny = y + dy;
                            if (!g.outOfRange(nx, ny) && g.getColor(nx, ny) === Player.NONE) {
                                const idx = (nx - 1) * size + (ny - 1);
                                if (!visited[idx]) {
                                    // 过滤禁手位置（仅对黑棋检测）
                                    if (g.isForbidden(nx, ny, Player.BLACK)) {
                                        visited[idx] = true;
                                        continue;
                                    }
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
            const center = Math.floor((size + 1) / 2);
            // 如果中心点是禁手，寻找最近的合法位置
            if (!g.isForbidden(center, center, Player.BLACK)) {
                candidates.push([center, center]);
            } else {
                for (let r = 1; r <= 3; r++) {
                    for (let dx = -r; dx <= r; dx++) {
                        for (let dy = -r; dy <= r; dy++) {
                            const nx = center + dx, ny = center + dy;
                            if (!g.outOfRange(nx, ny) && g.getColor(nx, ny) === Player.NONE) {
                                if (!g.isForbidden(nx, ny, Player.BLACK)) {
                                    candidates.push([nx, ny]);
                                    return candidates;
                                }
                            }
                        }
                    }
                }
            }
            return candidates;
        }

        // 快速评估并排序
        candidates.sort((a, b) => {
            const scoreA = this.quickEval(g, a[0], a[1], this.self) + this.quickEval(g, a[0], a[1], this.opponent);
            const scoreB = this.quickEval(g, b[0], b[1], this.self) + this.quickEval(g, b[0], b[1], this.opponent);
            return scoreB - scoreA;
        });

        if (candidates.length > maxCount) candidates.length = maxCount;
        return candidates;
    }

    select(node, g) {
        while (!node.isLeaf()) {
            let best = null;
            let bestUCB = -1;

            for (const child of node.children) {
                const ucb = child.ucb(this.exploration);
                if (ucb > bestUCB) {
                    bestUCB = ucb;
                    best = child;
                }
            }

            if (best === null) return node;

            g.set(best.x, best.y, best.player);
            node = best;
        }

        return node;
    }

    expand(node, g) {
        let nextPlayer;
        if (node.player === Player.NONE) {
            nextPlayer = this.self;
        } else {
            nextPlayer = (node.player === Player.BLACK) ? Player.WHITE : Player.BLACK;
        }

        const moves = this.getCandidateMoves(g, 2, 20); // 每层展开20个候选

        for (const [x, y] of moves) {
            const validation = g.validPosition(x, y, nextPlayer);
            if (!validation.valid) continue;

            let alreadyChild = false;
            for (const child of node.children) {
                if (child.x === x && child.y === y) {
                    alreadyChild = true;
                    break;
                }
            }

            if (!alreadyChild) {
                node.children.push(new MCTSNode(x, y, nextPlayer, node));
            }
        }
    }

    simulate(g, node) {
        let currentPlayer;
        if (node.player === Player.NONE) {
            currentPlayer = this.self;
        } else {
            currentPlayer = (node.player === Player.BLACK) ? Player.WHITE : Player.BLACK;
        }

        // 先检查当前状态是否已经结束
        if (node.player !== Player.NONE && g.Win(node.x, node.y, node.player)) {
            return node.player === this.self ? 1 : 0;
        }
        if (g.GameOver()) return 0;

        const sim = g.clone();
        const size = sim.getSize();
        const maxSteps = Math.min(size * size, 80); // 限制模拟步数

        for (let step = 0; step < maxSteps && !sim.GameOver(); step++) {
            const moves = this.getCandidateMoves(sim, 2, 35); // 每步35个候选
            if (moves.length === 0) break;

            // 构建权重数组
            const weights = [];
            for (const [mx, my] of moves) {
                let s = 0;
                s += this.quickEval(sim, mx, my, currentPlayer);
                const oppPlayer = (currentPlayer === Player.BLACK) ? Player.WHITE : Player.BLACK;
                s += this.quickEval(sim, mx, my, oppPlayer) / 2;
                weights.push(s + 1);
            }

            // 带权随机选择（简化版）
            const totalWeight = weights.reduce((a, b) => a + b, 0);
            let randomWeight = this.random() * totalWeight;
            let idx = 0;
            for (let i = 0; i < weights.length; i++) {
                randomWeight -= weights[i];
                if (randomWeight <= 0) {
                    idx = i;
                    break;
                }
            }

            const [mx, my] = moves[idx];

            // 检查位置是否有效（包括禁手规则）
            const simValidation = sim.validPosition(mx, my, currentPlayer);
            if (!simValidation.valid) {
                continue;
            }

            sim.set(mx, my, currentPlayer);

            if (sim.Win(mx, my, currentPlayer)) {
                return currentPlayer === this.self ? 1 : 0;
            }

            currentPlayer = (currentPlayer === Player.BLACK) ? Player.WHITE : Player.BLACK;
        }

        return 0; // 平局
    }

    backpropagate(node, result) {
        while (node !== null) {
            node.visits++;
            if (node.player === this.self) {
                node.wins += result;
            } else if (node.player === this.opponent) {
                node.wins += (1 - result);
            }
            node = node.parent;
        }
    }

    getBestMove(g) {
        this.nodeCount = 0;
        const root = new MCTSNode();

        // 根节点获取候选并按评分排序
        const rootMoves = this.getCandidateMoves(g, 3, 60); // 根节点60个候选

        // 全部展开为根节点的子节点
        for (const [x, y] of rootMoves) {
            const rootValidation = g.validPosition(x, y, this.self);
            if (rootValidation.valid) {
                root.children.push(new MCTSNode(x, y, this.self, root));
            }
        }

        if (root.children.length === 0) return [-1, -1];

        // MCTS主循环
        for (let i = 0; i < this.iterations; i++) {
            const sim = g.clone();

            // 选择
            const node = this.select(root, sim);

            // 扩展和模拟
            if (!sim.GameOver()) {
                const terminal = (node.player !== Player.NONE && sim.Win(node.x, node.y, node.player));
                if (!terminal) {
                    this.expand(node, sim);
                    // 从新扩展的节点中随机选一个进行模拟
                    if (node.children.length > 0) {
                        const childIdx = this.randint(0, node.children.length - 1);
                        const selectedNode = node.children[childIdx];
                        sim.set(selectedNode.x, selectedNode.y, selectedNode.player);
                    }
                }
            }

            // 模拟
            const result = this.simulate(sim, node);
            // 回传
            this.backpropagate(node, result);
            this.nodeCount++;
        }

        // 选择访问次数最多的子节点
        let bestX = -1, bestY = -1;
        let maxVisits = -1;
        let bestWinRate = -1;

        for (const child of root.children) {
            if (child.visits > 0) {
                const winRate = child.wins / child.visits;
                // 综合考虑访问次数和胜率
                if (child.visits > maxVisits ||
                    (child.visits === maxVisits && winRate > bestWinRate)) {
                    maxVisits = child.visits;
                    bestWinRate = winRate;
                    bestX = child.x;
                    bestY = child.y;
                }
            }
        }

        // 如果没有任何节点被访问过，选择评分最高的
        if (bestX === -1) {
            bestX = root.children[0].x;
            bestY = root.children[0].y;
        }

        return [bestX, bestY];
    }

    setIterations(iter) {
        this.iterations = iter;
    }

    getIterations() {
        return this.iterations;
    }

    getNodesSearched() {
        return this.nodeCount;
    }

    getSelf() {
        return this.self;
    }
}
