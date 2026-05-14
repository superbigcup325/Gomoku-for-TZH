const Config = {
    directions: [
        [0, 1],  // 水平
        [1, 0],  // 垂直
        [1, 1],  // 对角线
        [1, -1] // 反对角线
    ]
};

function currentPlayer(currentTurn) {
    return (currentTurn & 1) ? Player.BLACK : Player.WHITE;
}

class GomokuPattern {
    constructor() {
        this.count = 1;
        this.openEnds = 0;
        this.gapSegments = 0;
        this.gapSpaces = 0;
        this.type = PatternType.DEFAULT;
    }

    form() { return this.type; }
    isOverline() { return this.type === PatternType.OVERLINE; }
    isFive() { return this.type === PatternType.FIVE; }
    isFlex4() { return this.type === PatternType.FLEX4; }
    isBlock4() { return this.type === PatternType.BLOCK4; }
    isFlex3() { return this.type === PatternType.FLEX3; }
    isBlock3() { return this.type === PatternType.BLOCK3; }
    isFlex2() { return this.type === PatternType.FLEX2; }
    isBlock2() { return this.type === PatternType.BLOCK2; }
    isActive() { return this.openEnds > 0; }
    hasGap() { return this.gapSegments > 0; }
    isJumpFlex2() { return this.type === PatternType.FLEX2 && this.gapSegments >= 1; }
    isJumpFlex3() { return this.type === PatternType.FLEX3 && this.gapSegments >= 1; }
    isJumpBlock4() { return this.type === PatternType.BLOCK4 && this.gapSegments >= 1; }
}

class GomokuFormation {
    constructor() {
        this.patterns = new Array(4).fill(null).map(() => new GomokuPattern());
        this.player = Player.NONE;
        this.isEvaluated = false;
    }

    countForm(targetForm) {
        let cnt = 0;
        for (let i = 0; i < 4; i++) {
            if (this.patterns[i].form() === targetForm) cnt++;
        }
        return cnt;
    }

    countFlex4() {
        let cnt = 0;
        for (let i = 0; i < 4; i++) {
            if (this.patterns[i].isFlex4()) cnt++;
        }
        return cnt;
    }

    countBlock4() {
        let cnt = 0;
        for (let i = 0; i < 4; i++) {
            if (this.patterns[i].isBlock4()) cnt++;
        }
        return cnt;
    }

    countFlex3() {
        let cnt = 0;
        for (let i = 0; i < 4; i++) {
            if (this.patterns[i].isFlex3()) cnt++;
        }
        return cnt;
    }

    hasFive() {
        for (let pattern of this.patterns) {
            if (pattern.isFive()) return true;
        }
        return false;
    }
}

class Gomoku {
    constructor(sizeVal = 15) {
        this.size = sizeVal;
        this.currentCount = 0;
        this.maxCount = sizeVal * sizeVal;
        this.graph = new Array(sizeVal * sizeVal).fill(0);
    }

    pos(x, y) {
        return (x - 1) * this.size + (y - 1);
    }

    set(x, y, currentPlayer) {
        if (!this.validPosition(x, y, currentPlayer)) {
            console.error('error: invalid position');
            return;
        }
        this.graph[this.pos(x, y)] = currentPlayer;
        this.currentCount++;
    }

    outOfRange(x, y) {
        return (x < 1 || x > this.size || y < 1 || y > this.size);
    }

    isForbidden(x, y, player) {
        if (player !== Player.BLACK) return false;

        const formation = this.analyzeAll(x, y, player);
        if (formation.hasFive()) return false;

        for (let pattern of formation.patterns) {
            if (pattern.isOverline()) return true;
        }

        const flex4 = formation.countFlex4();
        const block4 = formation.countBlock4();
        const flex3 = formation.countFlex3();

        if (flex4 >= 2 || flex4 + block4 >= 2) return true;
        if (flex3 >= 2) return true;

        return false;
    }

    validPosition(x, y, player) {
        if (this.outOfRange(x, y)) return false;
        if (this.graph[this.pos(x, y)] !== 0) return false;
        if (this.isForbidden(x, y, player)) return false;
        return true;
    }

    Win(x, y, currentPlayer) {
        for (let [dx, dy] of Config.directions) {
            let count = 1;
            let nx = x + dx, ny = y + dy;
            while (!this.outOfRange(nx, ny) && this.graph[this.pos(nx, ny)] === currentPlayer) {
                count++;
                nx += dx;
                ny += dy;
            }
            nx = x - dx;
            ny = y - dy;
            while (!this.outOfRange(nx, ny) && this.graph[this.pos(nx, ny)] === currentPlayer) {
                count++;
                nx -= dx;
                ny -= dy;
            }
            if (count >= 5) return true;
        }
        return false;
    }

    GameOver() {
        return this.currentCount >= this.maxCount;
    }

    getGraph() {
        return [...this.graph];
    }

    getSize() {
        return this.size;
    }

    getCurrentCount() {
        return this.currentCount;
    }

    undo(x, y) {
        if (this.outOfRange(x, y)) return;
        if (this.currentCount === 0) return;
        if (this.graph[this.pos(x, y)] === Player.NONE) return;
        this.currentCount--;
        this.graph[this.pos(x, y)] = 0;
    }

    getColor(x, y) {
        if (this.outOfRange(x, y)) return Player.NONE;
        const val = this.graph[this.pos(x, y)];
        if (val === Player.BLACK) return Player.BLACK;
        if (val === Player.WHITE) return Player.WHITE;
        return Player.NONE;
    }

    analyzeAll(x, y, player) {
        const formation = new GomokuFormation();
        this.graph[this.pos(x, y)] = player;
        formation.player = player;

        let idx = 0;
        for (let [dx, dy] of Config.directions) {
            formation.patterns[idx++] = this.analyzeForm(x, y, dx, dy, player);
        }

        formation.isEvaluated = true;
        this.graph[this.pos(x, y)] = 0;
        return formation;
    }

    analyzeForm(x, y, dx, dy, player) {
        const pattern = new GomokuPattern();
        const line = new Array(PATTERN_WINDOW).fill(0);
        line[PATTERN_CENTER] = player;

        const BLOCK_MARKER = 3;

        for (let step = 1; step <= PATTERN_RADIUS; step++) {
            let nx = x + dx * step, ny = y + dy * step;
            if (!this.outOfRange(nx, ny)) {
                const color = this.getColor(nx, ny);
                if (color === player) line[PATTERN_CENTER + step] = player;
                else if (color === Player.NONE) line[PATTERN_CENTER + step] = 0;
                else line[PATTERN_CENTER + step] = BLOCK_MARKER;
            } else {
                line[PATTERN_CENTER + step] = BLOCK_MARKER;
            }

            let px = x - dx * step, py = y - dy * step;
            if (!this.outOfRange(px, py)) {
                const color = this.getColor(px, py);
                if (color === player) line[PATTERN_CENTER - step] = player;
                else if (color === Player.NONE) line[PATTERN_CENTER - step] = 0;
                else line[PATTERN_CENTER - step] = BLOCK_MARKER;
            } else {
                line[PATTERN_CENTER - step] = BLOCK_MARKER;
            }
        }

        const db = PatternDB.getInstance();
        const key = db.encodeLine(line, player);
        const info = db.lookup(key);

        if (info) {
            pattern.count = info.count;
            pattern.openEnds = info.openEnds;
            pattern.gapSegments = info.gapSegments;
            pattern.gapSpaces = info.gapSpaces;
            pattern.type = info.type;
        } else {
            pattern.count = 1;
            pattern.openEnds = 0;
            pattern.gapSegments = 0;
            pattern.gapSpaces = 0;
            pattern.type = PatternType.DEFAULT;
        }

        return pattern;
    }

    clone() {
        const newGomoku = new Gomoku(this.size);
        newGomoku.graph = [...this.graph];
        newGomoku.currentCount = this.currentCount;
        return newGomoku;
    }
}
