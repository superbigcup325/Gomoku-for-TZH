const PATTERN_RADIUS = 4;
const PATTERN_WINDOW = 9;
const PATTERN_CENTER = 4;

const Player = {
    NONE: 0,
    BLACK: 1,
    WHITE: 2
};

const PatternType = {
    OVERLINE: 0,
    FIVE: 1,
    FLEX4: 2,
    BLOCK4: 3,
    FLEX3: 4,
    BLOCK3: 5,
    FLEX2: 6,
    BLOCK2: 7,
    FLEX1: 8,
    BLOCK1: 9,
    FORBIDDEN: 10,
    DEFAULT: 11
};

class PatternInfo {
    constructor() {
        this.count = 1;
        this.openEnds = 0;
        this.gapSegments = 0;
        this.gapSpaces = 0;
        this.type = PatternType.DEFAULT;
    }
}

class PatternDB {
    constructor() {
        this.table = new Map();
        this.initialized = false;
        // 延迟初始化，避免阻塞主线程
    }

    static getInstance() {
        if (!PatternDB.instance) {
            PatternDB.instance = new PatternDB();
        }
        return PatternDB.instance;
    }

    ensureInitialized() {
        if (!this.initialized) {
            console.log('⏳ 正在初始化棋型数据库...');
            const startTime = performance.now();

            this.initialize();
            this.initialized = true;

            const endTime = performance.now();
            console.log(`✅ 棋型数据库初始化完成 (${Math.round(endTime - startTime)}ms, ${this.table.size}个模式)`);
        }
    }

    makeKey(cells) {
        let k = 0;
        for (let i = 0; i < PATTERN_WINDOW; i++) {
            k |= ((cells[i] & 3) << (i * 2));
        }
        return k;
    }

    addPatternByKey(key, count, openEnds, gapSegments, gapSpaces, type) {
        if (!this.table.has(key)) {
            const info = new PatternInfo();
            info.count = count;
            info.openEnds = openEnds;
            info.gapSegments = gapSegments;
            info.gapSpaces = gapSpaces;
            info.type = type;
            this.table.set(key, info);
        }
    }

    addPatternByCells(cells, count, openEnds, gapSegments, gapSpaces, type) {
        this.addPatternByKey(this.makeKey(cells), count, openEnds, gapSegments, gapSpaces, type);
    }

    initialize() {
        this.generateFivePatterns();
        this.generateOverlinePatterns();
        this.addFlex4Patterns();
        this.addBlock4Patterns();
        this.addFlex3Patterns();
        this.addBlock3Patterns();
        this.addFlex2Patterns();
        this.addBlock2Patterns();
    }

    generateFivePatterns() {
        const positions = [
            [0, 1, 2, 3, 4],
            [1, 2, 3, 4, 5],
            [2, 3, 4, 5, 6],
            [3, 4, 5, 6, 7],
            [4, 5, 6, 7, 8]
        ];

        for (let p = 0; p < 5; p++) {
            let remaining = [];
            let ri = 0;
            for (let i = 0; i < PATTERN_WINDOW; i++) {
                let isInPos = false;
                for (let j = 0; j < 5; j++) {
                    if (i === positions[p][j]) { isInPos = true; break; }
                }
                if (!isInPos) remaining[ri++] = i;
            }
            for (let mask = 0; mask < 16; mask++) {
                let cells = new Array(PATTERN_WINDOW).fill(0);
                for (let j = 0; j < 5; j++) cells[positions[p][j]] = 1;
                for (let b = 0; b < 4; b++) {
                    cells[remaining[b]] = ((mask >> b) & 1) ? 2 : 0;
                }
                this.addPatternByCells(cells, 5, 0, 0, 0, PatternType.FIVE);
            }
        }
    }

    generateOverlinePatterns() {
        for (let len = 6; len <= PATTERN_WINDOW; len++) {
            for (let start = 0; start <= PATTERN_WINDOW - len; start++) {
                if (start > PATTERN_CENTER || start + len <= PATTERN_CENTER) continue;
                let remaining = [];
                let ri = 0;
                for (let i = 0; i < PATTERN_WINDOW; i++) {
                    if (i < start || i >= start + len) remaining[ri++] = i;
                }
                let combos = 1 << ri;
                for (let mask = 0; mask < combos; mask++) {
                    let cells = new Array(PATTERN_WINDOW).fill(0);
                    for (let j = start; j < start + len; j++) cells[j] = 1;
                    for (let b = 0; b < ri; b++) {
                        cells[remaining[b]] = ((mask >> b) & 1) ? 2 : 0;
                    }
                    this.addPatternByCells(cells, len, 0, 0, 0, PatternType.OVERLINE);
                }
            }
        }
    }

    addFlex4Patterns() {
        const positions = [
            [1, 2, 3, 4],
            [2, 3, 4, 5],
            [3, 4, 5, 6],
            [4, 5, 6, 7]
        ];
        const leftOpen = [0, 1, 2, 3];
        const rightOpen = [5, 6, 7, 8];

        for (let p = 0; p < 4; p++) {
            let remaining = [];
            let ri = 0;
            for (let i = 0; i < PATTERN_WINDOW; i++) {
                let isInPos = false;
                for (let j = 0; j < 4; j++) {
                    if (i === positions[p][j]) { isInPos = true; break; }
                }
                if (!isInPos && i !== leftOpen[p] && i !== rightOpen[p]) remaining[ri++] = i;
            }
            for (let mask = 0; mask < (1 << ri); mask++) {
                let cells = new Array(PATTERN_WINDOW).fill(0);
                for (let j = 0; j < 4; j++) cells[positions[p][j]] = 1;
                cells[leftOpen[p]] = 0;
                cells[rightOpen[p]] = 0;
                for (let b = 0; b < ri; b++) {
                    cells[remaining[b]] = ((mask >> b) & 1) ? 2 : 0;
                }
                this.addPatternByCells(cells, 4, 2, 0, 0, PatternType.FLEX4);
            }
        }
    }

    addBlock4Patterns() {
        const addBlock4 = (...args) => {
            const c = args.slice(0, 9);
            const openEnds = args[9];
            const gapSeg = args[10];
            const gapSp = args[11];
            this.addPatternByCells(c, 4, openEnds, gapSeg, gapSp, PatternType.BLOCK4);
        };

        // 4连一端被堵
        addBlock4(0,1,1,1,1,2,0,0,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,0,0,2, 1,0,0);
        addBlock4(0,1,1,1,1,2,0,2,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,0,2,2, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,0,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,0,2, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,2,0, 1,0,0);
        addBlock4(0,1,1,1,1,2,2,2,2, 1,0,0);

        addBlock4(2,1,1,1,1,0,0,0,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,0,0,2, 1,0,0);
        addBlock4(2,1,1,1,1,0,0,2,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,0,2,2, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,0,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,0,2, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,2,0, 1,0,0);
        addBlock4(2,1,1,1,1,0,2,2,2, 1,0,0);

        // 4连两端被堵
        addBlock4(2,1,1,1,1,2,0,0,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,0,0,2, 0,0,0);
        addBlock4(2,1,1,1,1,2,0,2,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,0,2,2, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,0,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,0,2, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,2,0, 0,0,0);
        addBlock4(2,1,1,1,1,2,2,2,2, 0,0,0);

        // 4连偏移 [2,3,4,5]
        addBlock4(0,0,1,1,1,1,2,0,0, 1,0,0);
        addBlock4(0,0,1,1,1,1,2,0,2, 1,0,0);
        addBlock4(0,0,1,1,1,1,2,2,0, 1,0,0);
        addBlock4(0,0,1,1,1,1,2,2,2, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,0,0, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,0,2, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,2,0, 1,0,0);
        addBlock4(2,0,1,1,1,1,2,2,2, 1,0,0);

        addBlock4(0,2,1,1,1,1,0,0,0, 1,0,0);
        addBlock4(0,2,1,1,1,1,0,0,2, 1,0,0);
        addBlock4(0,2,1,1,1,1,0,2,0, 1,0,0);
        addBlock4(0,2,1,1,1,1,0,2,2, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,0,0, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,0,2, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,2,0, 1,0,0);
        addBlock4(2,2,1,1,1,1,0,2,2, 1,0,0);

        addBlock4(0,2,1,1,1,1,2,0,0, 0,0,0);
        addBlock4(0,2,1,1,1,1,2,0,2, 0,0,0);
        addBlock4(0,2,1,1,1,1,2,2,0, 0,0,0);
        addBlock4(0,2,1,1,1,1,2,2,2, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,0,0, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,0,2, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,2,0, 0,0,0);
        addBlock4(2,2,1,1,1,1,2,2,2, 0,0,0);

        // 4连偏移 [3,4,5,6]
        addBlock4(0,0,0,1,1,1,1,2,0, 1,0,0);
        addBlock4(0,0,0,1,1,1,1,2,2, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,2, 1,0,0);
        addBlock4(2,0,0,1,1,1,1,2,0, 1,0,0);
        addBlock4(2,0,0,1,1,1,1,2,2, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,2, 1,0,0);

        addBlock4(0,0,2,1,1,1,1,0,0, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,0,2, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(0,0,2,1,1,1,1,2,2, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,0,0, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,0,2, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,0, 1,0,0);
        addBlock4(2,0,2,1,1,1,1,2,2, 1,0,0);

        addBlock4(0,0,2,1,1,1,1,2,0, 0,0,0);
        addBlock4(0,0,2,1,1,1,1,2,2, 0,0,0);
        addBlock4(2,0,2,1,1,1,1,2,0, 0,0,0);
        addBlock4(2,0,2,1,1,1,1,2,2, 0,0,0);

        // 4连偏移 [4,5,6,7]
        addBlock4(0,0,0,0,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,2,0,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,2,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,0,0,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,2,0,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,2,2,1,1,1,1,2, 1,0,0);

        addBlock4(0,0,0,2,1,1,1,1,0, 1,0,0);
        addBlock4(0,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(0,0,2,2,1,1,1,1,0, 1,0,0);
        addBlock4(0,0,2,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,0,2,1,1,1,1,0, 1,0,0);
        addBlock4(2,0,0,2,1,1,1,1,2, 1,0,0);
        addBlock4(2,0,2,2,1,1,1,1,0, 1,0,0);
        addBlock4(2,0,2,2,1,1,1,1,2, 1,0,0);

        addBlock4(0,0,0,2,1,1,1,1,2, 0,0,0);
        addBlock4(0,0,2,2,1,1,1,1,2, 0,0,0);
        addBlock4(2,0,0,2,1,1,1,1,2, 0,0,0);
        addBlock4(2,0,2,2,1,1,1,1,2, 0,0,0);

        // 跳冲四 11011
        addBlock4(0,0,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(0,0,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(0,0,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(0,0,1,1,0,1,1,2,2, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(0,2,1,1,0,1,1,2,2, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(2,0,1,1,0,1,1,2,2, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,0,0, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,0,2, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,2,0, 1,1,1);
        addBlock4(2,2,1,1,0,1,1,2,2, 1,1,1);

        // 跳冲四 10111
        addBlock4(0,0,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(0,0,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(0,0,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(0,0,1,0,1,1,1,2,2, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(0,2,1,0,1,1,1,2,2, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(2,0,1,0,1,1,1,2,2, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,0,0, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,0,2, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,2,0, 1,1,1);
        addBlock4(2,2,1,0,1,1,1,2,2, 1,1,1);

        // 跳冲四 11101
        addBlock4(0,0,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(0,0,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(0,0,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(0,0,1,1,1,0,1,2,2, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(0,2,1,1,1,0,1,2,2, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(2,0,1,1,1,0,1,2,2, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,0,0, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,0,2, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,2,0, 1,1,1);
        addBlock4(2,2,1,1,1,0,1,2,2, 1,1,1);

        // 跳冲四偏移
        addBlock4(0,0,0,1,1,0,1,1,0, 1,1,1);
        addBlock4(0,0,0,1,1,0,1,1,2, 1,1,1);
        addBlock4(0,0,2,1,1,0,1,1,0, 1,1,1);
        addBlock4(0,0,2,1,1,0,1,1,2, 1,1,1);
        addBlock4(2,0,0,1,1,0,1,1,0, 1,1,1);
        addBlock4(2,0,0,1,1,0,1,1,2, 1,1,1);
        addBlock4(2,0,2,1,1,0,1,1,0, 1,1,1);
        addBlock4(2,0,2,1,1,0,1,1,2, 1,1,1);

        addBlock4(0,0,0,1,0,1,1,1,0, 1,1,1);
        addBlock4(0,0,0,1,0,1,1,1,2, 1,1,1);
        addBlock4(0,0,2,1,0,1,1,1,0, 1,1,1);
        addBlock4(0,0,2,1,0,1,1,1,2, 1,1,1);
        addBlock4(2,0,0,1,0,1,1,1,0, 1,1,1);
        addBlock4(2,0,0,1,0,1,1,1,2, 1,1,1);
        addBlock4(2,0,2,1,0,1,1,1,0, 1,1,1);
        addBlock4(2,0,2,1,0,1,1,1,2, 1,1,1);

        addBlock4(0,0,0,1,1,1,0,1,0, 1,1,1);
        addBlock4(0,0,0,1,1,1,0,1,2, 1,1,1);
        addBlock4(0,0,2,1,1,1,0,1,0, 1,1,1);
        addBlock4(0,0,2,1,1,1,0,1,2, 1,1,1);
        addBlock4(2,0,0,1,1,1,0,1,0, 1,1,1);
        addBlock4(2,0,0,1,1,1,0,1,2, 1,1,1);
        addBlock4(2,0,2,1,1,1,0,1,0, 1,1,1);
        addBlock4(2,0,2,1,1,1,0,1,2, 1,1,1);

        // 跳冲四两端被堵
        addBlock4(2,2,1,1,0,1,1,2,2, 0,1,1);
        addBlock4(2,2,1,0,1,1,1,2,2, 0,1,1);
        addBlock4(2,2,1,1,1,0,1,2,2, 0,1,1);
    }

    addFlex3Patterns() {
        const addFlex3 = (...args) => {
            const c = args.slice(0, 9);
            const gapSeg = args[9];
            const gapSp = args[10];
            this.addPatternByCells(c, 3, 2, gapSeg, gapSp, PatternType.FLEX3);
        };

        // 3连 [2,3,4]
        addFlex3(0,0,1,1,1,0,0,0,0, 0,0);
        addFlex3(0,0,1,1,1,0,0,0,2, 0,0);
        addFlex3(0,0,1,1,1,0,0,2,0, 0,0);
        addFlex3(0,0,1,1,1,0,0,2,2, 0,0);
        addFlex3(2,0,1,1,1,0,0,0,0, 0,0);
        addFlex3(2,0,1,1,1,0,0,0,2, 0,0);
        addFlex3(2,0,1,1,1,0,0,2,0, 0,0);
        addFlex3(2,0,1,1,1,0,0,2,2, 0,0);

        // 3连 [3,4,5]
        addFlex3(0,0,0,1,1,1,0,0,0, 0,0);
        addFlex3(0,0,0,1,1,1,0,0,2, 0,0);
        addFlex3(0,0,2,1,1,1,0,0,0, 0,0);
        addFlex3(0,0,2,1,1,1,0,0,2, 0,0);
        addFlex3(2,0,0,1,1,1,0,0,0, 0,0);
        addFlex3(2,0,0,1,1,1,0,0,2, 0,0);
        addFlex3(2,0,2,1,1,1,0,0,0, 0,0);
        addFlex3(2,0,2,1,1,1,0,0,2, 0,0);

        // 3连 [4,5,6]
        addFlex3(0,0,0,0,1,1,1,0,0, 0,0);
        addFlex3(0,0,0,2,1,1,1,0,0, 0,0);
        addFlex3(2,0,0,0,1,1,1,0,0, 0,0);
        addFlex3(2,0,0,2,1,1,1,0,0, 0,0);

        // 跳活三 1011
        addFlex3(0,0,1,0,1,1,0,0,0, 1,1);
        addFlex3(0,0,1,0,1,1,0,0,2, 1,1);
        addFlex3(2,0,1,0,1,1,0,0,0, 1,1);
        addFlex3(2,0,1,0,1,1,0,0,2, 1,1);

        // 跳活三 1101
        addFlex3(0,0,1,1,0,1,0,0,0, 1,1);
        addFlex3(0,0,1,1,0,1,0,0,2, 1,1);
        addFlex3(2,0,1,1,0,1,0,0,0, 1,1);
        addFlex3(2,0,1,1,0,1,0,0,2, 1,1);

        // 跳活三 1011偏移
        addFlex3(0,0,0,1,0,1,1,0,0, 1,1);
        addFlex3(0,0,0,1,0,1,1,0,2, 1,1);
        addFlex3(2,0,0,1,0,1,1,0,0, 1,1);
        addFlex3(2,0,0,1,0,1,1,0,2, 1,1);

        // 跳活三 1101偏移
        addFlex3(0,0,0,1,1,0,1,0,0, 1,1);
        addFlex3(0,0,0,1,1,0,1,0,2, 1,1);
        addFlex3(2,0,0,1,1,0,1,0,0, 1,1);
        addFlex3(2,0,0,1,1,0,1,0,2, 1,1);
    }

    addBlock3Patterns() {
        const addBlock3 = (...args) => {
            const c = args.slice(0, 9);
            const openEnds = args[9];
            const gapSeg = args[10];
            const gapSp = args[11];
            this.addPatternByCells(c, 3, openEnds, gapSeg, gapSp, PatternType.BLOCK3);
        };

        // 3连一端被堵 [2,3,4]
        addBlock3(0,0,1,1,1,2,0,0,0, 1,0,0);
        addBlock3(0,0,1,1,1,2,0,0,2, 1,0,0);
        addBlock3(0,0,1,1,1,2,0,2,0, 1,0,0);
        addBlock3(0,0,1,1,1,2,0,2,2, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,0,0, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,0,2, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,2,0, 1,0,0);
        addBlock3(2,0,1,1,1,2,0,2,2, 1,0,0);

        addBlock3(0,2,1,1,1,0,0,0,0, 1,0,0);
        addBlock3(0,2,1,1,1,0,0,0,2, 1,0,0);
        addBlock3(0,2,1,1,1,0,0,2,0, 1,0,0);
        addBlock3(0,2,1,1,1,0,0,2,2, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,0,0, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,0,2, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,2,0, 1,0,0);
        addBlock3(2,2,1,1,1,0,0,2,2, 1,0,0);

        // 3连两端被堵 [2,3,4]
        addBlock3(0,2,1,1,1,2,0,0,0, 0,0,0);
        addBlock3(0,2,1,1,1,2,0,0,2, 0,0,0);
        addBlock3(0,2,1,1,1,2,0,2,0, 0,0,0);
        addBlock3(0,2,1,1,1,2,0,2,2, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,0,0, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,0,2, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,2,0, 0,0,0);
        addBlock3(2,2,1,1,1,2,0,2,2, 0,0,0);

        // 3连一端被堵 [3,4,5]
        addBlock3(0,0,0,1,1,1,2,0,0, 1,0,0);
        addBlock3(0,0,0,1,1,1,2,0,2, 1,0,0);
        addBlock3(0,0,0,1,1,1,2,2,0, 1,0,0);
        addBlock3(0,0,0,1,1,1,2,2,2, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,0,0, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,0,2, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,2,0, 1,0,0);
        addBlock3(2,0,0,1,1,1,2,2,2, 1,0,0);

        addBlock3(0,0,2,1,1,1,0,0,0, 1,0,0);
        addBlock3(0,0,2,1,1,1,0,0,2, 1,0,0);
        addBlock3(0,0,2,1,1,1,0,2,0, 1,0,0);
        addBlock3(0,0,2,1,1,1,0,2,2, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,0,0, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,0,2, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,2,0, 1,0,0);
        addBlock3(2,0,2,1,1,1,0,2,2, 1,0,0);

        // 3连两端被堵 [3,4,5]
        addBlock3(0,0,2,1,1,1,2,0,0, 0,0,0);
        addBlock3(0,0,2,1,1,1,2,0,2, 0,0,0);
        addBlock3(0,0,2,1,1,1,2,2,0, 0,0,0);
        addBlock3(0,0,2,1,1,1,2,2,2, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,0,0, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,0,2, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,2,0, 0,0,0);
        addBlock3(2,0,2,1,1,1,2,2,2, 0,0,0);

        // 3连一端被堵 [4,5,6]
        addBlock3(0,0,0,0,1,1,1,2,0, 1,0,0);
        addBlock3(0,0,0,0,1,1,1,2,2, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 1,0,0);
        addBlock3(2,0,0,0,1,1,1,2,0, 1,0,0);
        addBlock3(2,0,0,0,1,1,1,2,2, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 1,0,0);

        addBlock3(0,0,0,2,1,1,1,0,0, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,0,2, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,0,0, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,0,2, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 1,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 1,0,0);

        // 3连两端被堵 [4,5,6]
        addBlock3(0,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 0,0,0);

        // 跳眠三 1011一端被堵
        addBlock3(0,2,1,0,1,1,0,0,0, 1,1,1);
        addBlock3(0,2,1,0,1,1,0,0,2, 1,1,1);
        addBlock3(0,2,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(0,2,1,0,1,1,0,2,2, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,0,0, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,0,2, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(2,2,1,0,1,1,0,2,2, 1,1,1);

        // 跳眠三 1101一端被堵
        addBlock3(0,2,1,1,0,1,0,0,0, 1,1,1);
        addBlock3(0,2,1,1,0,1,0,0,2, 1,1,1);
        addBlock3(0,2,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(0,2,1,1,0,1,0,2,2, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,0,0, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,0,2, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(2,2,1,1,0,1,0,2,2, 1,1,1);

        // 跳眠三 1011右端被堵
        addBlock3(0,0,1,0,1,1,2,0,0, 1,1,1);
        addBlock3(0,0,1,0,1,1,2,0,2, 1,1,1);
        addBlock3(0,0,1,0,1,1,2,2,0, 1,1,1);
        addBlock3(0,0,1,0,1,1,2,2,2, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,0,0, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,0,2, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,2,0, 1,1,1);
        addBlock3(2,0,1,0,1,1,2,2,2, 1,1,1);

        // 跳眠三 1101右端被堵
        addBlock3(0,0,1,1,0,1,2,0,0, 1,1,1);
        addBlock3(0,0,1,1,0,1,2,0,2, 1,1,1);
        addBlock3(0,0,1,1,0,1,2,2,0, 1,1,1);
        addBlock3(0,0,1,1,0,1,2,2,2, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,0,0, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,0,2, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,2,0, 1,1,1);
        addBlock3(2,0,1,1,0,1,2,2,2, 1,1,1);

        // 跳眠三两端被堵
        addBlock3(2,2,1,0,1,1,2,0,0, 0,1,1);
        addBlock3(2,2,1,0,1,1,2,0,2, 0,1,1);
        addBlock3(2,2,1,0,1,1,2,2,0, 0,1,1);
        addBlock3(2,2,1,0,1,1,2,2,2, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,0,0, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,0,2, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,2,0, 0,1,1);
        addBlock3(2,2,1,1,0,1,2,2,2, 0,1,1);

        // 伪活三(只能形成冲四)
        addBlock3(0,0,1,1,1,0,2,0,0, 0,0,0);
        addBlock3(0,0,1,1,1,0,2,0,2, 0,0,0);
        addBlock3(0,0,1,1,1,0,2,2,0, 0,0,0);
        addBlock3(0,0,1,1,1,0,2,2,2, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,0,0, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,0,2, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,2,0, 0,0,0);
        addBlock3(2,0,1,1,1,0,2,2,2, 0,0,0);

        addBlock3(0,0,0,1,1,1,0,2,0, 0,0,0);
        addBlock3(0,0,0,1,1,1,0,2,2, 0,0,0);
        addBlock3(0,0,2,1,1,1,0,2,0, 0,0,0);
        addBlock3(0,0,2,1,1,1,0,2,2, 0,0,0);
        addBlock3(2,0,0,1,1,1,0,2,0, 0,0,0);
        addBlock3(2,0,0,1,1,1,0,2,2, 0,0,0);
        addBlock3(2,0,2,1,1,1,0,2,0, 0,0,0);
        addBlock3(2,0,2,1,1,1,0,2,2, 0,0,0);

        addBlock3(0,0,0,0,1,1,1,0,2, 0,0,0);
        addBlock3(0,0,0,0,1,1,1,2,0, 0,0,0);
        addBlock3(0,0,0,0,1,1,1,2,2, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,0,2, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(0,0,0,2,1,1,1,2,2, 0,0,0);
        addBlock3(2,0,0,0,1,1,1,0,2, 0,0,0);
        addBlock3(2,0,0,0,1,1,1,2,0, 0,0,0);
        addBlock3(2,0,0,0,1,1,1,2,2, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,0,2, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,0, 0,0,0);
        addBlock3(2,0,0,2,1,1,1,2,2, 0,0,0);

        // 跳眠三 1011右端受限
        addBlock3(0,0,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(0,0,1,0,1,1,0,2,2, 1,1,1);
        addBlock3(2,0,1,0,1,1,0,2,0, 1,1,1);
        addBlock3(2,0,1,0,1,1,0,2,2, 1,1,1);

        // 跳眠三 1101右端受限
        addBlock3(0,0,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(0,0,1,1,0,1,0,2,2, 1,1,1);
        addBlock3(2,0,1,1,0,1,0,2,0, 1,1,1);
        addBlock3(2,0,1,1,0,1,0,2,2, 1,1,1);

        // 跳眠三 1011偏移右端受限
        addBlock3(0,0,0,1,0,1,1,2,0, 1,1,1);
        addBlock3(0,0,0,1,0,1,1,2,2, 1,1,1);
        addBlock3(2,0,0,1,0,1,1,2,0, 1,1,1);
        addBlock3(2,0,0,1,0,1,1,2,2, 1,1,1);

        // 跳眠三 1101偏移右端受限
        addBlock3(0,0,0,1,1,0,1,2,0, 1,1,1);
        addBlock3(0,0,0,1,1,0,1,2,2, 1,1,1);
        addBlock3(2,0,0,1,1,0,1,2,0, 1,1,1);
        addBlock3(2,0,0,1,1,0,1,2,2, 1,1,1);

        // 跳眠三 10101两端开放
        addBlock3(0,0,1,0,1,0,1,0,0, 0,2,2);
        addBlock3(0,0,1,0,1,0,1,0,2, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,0,0, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,0,2, 0,2,2);

        // 跳眠三 10101偏移两端开放
        addBlock3(0,0,0,1,0,1,0,1,0, 0,2,2);
        addBlock3(0,0,0,1,0,1,0,1,2, 0,2,2);
        addBlock3(2,0,0,1,0,1,0,1,0, 0,2,2);
        addBlock3(2,0,0,1,0,1,0,1,2, 0,2,2);

        // 跳眠三 10011两端开放
        addBlock3(0,0,1,0,0,1,1,0,0, 0,1,2);
        addBlock3(0,0,1,0,0,1,1,0,2, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,0,0, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,0,2, 0,1,2);

        // 跳眠三 11001两端开放
        addBlock3(0,0,1,1,0,0,1,0,0, 0,1,2);
        addBlock3(0,0,1,1,0,0,1,0,2, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,0,0, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,0,2, 0,1,2);

        // 跳眠三 10101右端受限
        addBlock3(0,0,1,0,1,0,1,2,0, 0,2,2);
        addBlock3(0,0,1,0,1,0,1,2,2, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,2,0, 0,2,2);
        addBlock3(2,0,1,0,1,0,1,2,2, 0,2,2);

        // 跳眠三 10101偏移左端受限
        addBlock3(0,0,2,1,0,1,0,1,0, 0,2,2);
        addBlock3(0,0,2,1,0,1,0,1,2, 0,2,2);
        addBlock3(2,0,2,1,0,1,0,1,0, 0,2,2);
        addBlock3(2,0,2,1,0,1,0,1,2, 0,2,2);

        // 跳眠三 10011右端受限
        addBlock3(0,0,1,0,0,1,1,2,0, 0,1,2);
        addBlock3(0,0,1,0,0,1,1,2,2, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,2,0, 0,1,2);
        addBlock3(2,0,1,0,0,1,1,2,2, 0,1,2);

        // 跳眠三 11001右端受限
        addBlock3(0,0,1,1,0,0,1,2,0, 0,1,2);
        addBlock3(0,0,1,1,0,0,1,2,2, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,2,0, 0,1,2);
        addBlock3(2,0,1,1,0,0,1,2,2, 0,1,2);
    }

    addFlex2Patterns() {
        const addFlex2 = (...args) => {
            const c = args.slice(0, 9);
            const gapSeg = args[9];
            const gapSp = args[10];
            this.addPatternByCells(c, 2, 2, gapSeg, gapSp, PatternType.FLEX2);
        };

        // 2连 [3,4]
        addFlex2(0,0,0,1,1,0,0,0,0, 0,0);
        addFlex2(0,0,0,1,1,0,0,0,2, 0,0);
        addFlex2(0,0,0,1,1,0,0,2,0, 0,0);
        addFlex2(0,0,0,1,1,0,0,2,2, 0,0);
        addFlex2(2,0,0,1,1,0,0,0,0, 0,0);
        addFlex2(2,0,0,1,1,0,0,0,2, 0,0);
        addFlex2(2,0,0,1,1,0,0,2,0, 0,0);
        addFlex2(2,0,0,1,1,0,0,2,2, 0,0);

        // 2连 [4,5]
        addFlex2(0,0,0,0,1,1,0,0,0, 0,0);
        addFlex2(0,0,0,0,1,1,0,0,2, 0,0);
        addFlex2(0,0,0,0,1,1,0,2,0, 0,0);
        addFlex2(0,0,0,0,1,1,0,2,2, 0,0);
        addFlex2(2,0,0,0,1,1,0,0,0, 0,0);
        addFlex2(2,0,0,0,1,1,0,0,2, 0,0);
        addFlex2(2,0,0,0,1,1,0,2,0, 0,0);
        addFlex2(2,0,0,0,1,1,0,2,2, 0,0);

        // 跳活二 101
        addFlex2(0,0,0,1,0,1,0,0,0, 1,1);
        addFlex2(0,0,0,1,0,1,0,0,2, 1,1);
        addFlex2(0,0,0,1,0,1,0,2,0, 1,1);
        addFlex2(0,0,0,1,0,1,0,2,2, 1,1);
        addFlex2(2,0,0,1,0,1,0,0,0, 1,1);
        addFlex2(2,0,0,1,0,1,0,0,2, 1,1);
        addFlex2(2,0,0,1,0,1,0,2,0, 1,1);
        addFlex2(2,0,0,1,0,1,0,2,2, 1,1);

        // 跳活二 1001
        addFlex2(0,0,0,1,0,0,1,0,0, 1,2);
        addFlex2(0,0,0,1,0,0,1,0,2, 1,2);
        addFlex2(0,0,0,1,0,0,1,2,0, 1,2);
        addFlex2(0,0,0,1,0,0,1,2,2, 1,2);
        addFlex2(2,0,0,1,0,0,1,0,0, 1,2);
        addFlex2(2,0,0,1,0,0,1,0,2, 1,2);
        addFlex2(2,0,0,1,0,0,1,2,0, 1,2);
        addFlex2(2,0,0,1,0,0,1,2,2, 1,2);
    }

    addBlock2Patterns() {
        const addBlock2 = (...args) => {
            const c = args.slice(0, 9);
            const openEnds = args[9];
            const gapSeg = args[10];
            const gapSp = args[11];
            this.addPatternByCells(c, 2, openEnds, gapSeg, gapSp, PatternType.BLOCK2);
        };

        // 2连一端被堵 [3,4]
        addBlock2(0,0,0,1,1,2,0,0,0, 1,0,0);
        addBlock2(0,0,0,1,1,2,0,0,2, 1,0,0);
        addBlock2(0,0,0,1,1,2,0,2,0, 1,0,0);
        addBlock2(0,0,0,1,1,2,0,2,2, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,0,0, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,0,2, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,2,0, 1,0,0);
        addBlock2(2,0,0,1,1,2,0,2,2, 1,0,0);

        addBlock2(0,0,2,1,1,0,0,0,0, 1,0,0);
        addBlock2(0,0,2,1,1,0,0,0,2, 1,0,0);
        addBlock2(0,0,2,1,1,0,0,2,0, 1,0,0);
        addBlock2(0,0,2,1,1,0,0,2,2, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,0,0, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,0,2, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,2,0, 1,0,0);
        addBlock2(2,0,2,1,1,0,0,2,2, 1,0,0);

        // 2连两端被堵 [3,4]
        addBlock2(0,0,2,1,1,2,0,0,0, 0,0,0);
        addBlock2(0,0,2,1,1,2,0,0,2, 0,0,0);
        addBlock2(0,0,2,1,1,2,0,2,0, 0,0,0);
        addBlock2(0,0,2,1,1,2,0,2,2, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,0,0, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,0,2, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,2,0, 0,0,0);
        addBlock2(2,0,2,1,1,2,0,2,2, 0,0,0);

        // 2连一端被堵 [4,5]
        addBlock2(0,0,0,0,1,1,2,0,0, 1,0,0);
        addBlock2(0,0,0,0,1,1,2,0,2, 1,0,0);
        addBlock2(0,0,0,0,1,1,2,2,0, 1,0,0);
        addBlock2(0,0,0,0,1,1,2,2,2, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,0,0, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,0,2, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,2,0, 1,0,0);
        addBlock2(2,0,0,0,1,1,2,2,2, 1,0,0);

        addBlock2(0,0,0,2,1,1,0,0,0, 1,0,0);
        addBlock2(0,0,0,2,1,1,0,0,2, 1,0,0);
        addBlock2(0,0,0,2,1,1,0,2,0, 1,0,0);
        addBlock2(0,0,0,2,1,1,0,2,2, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,0,0, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,0,2, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,2,0, 1,0,0);
        addBlock2(2,0,0,2,1,1,0,2,2, 1,0,0);

        // 2连两端被堵 [4,5]
        addBlock2(0,0,0,2,1,1,2,0,0, 0,0,0);
        addBlock2(0,0,0,2,1,1,2,0,2, 0,0,0);
        addBlock2(0,0,0,2,1,1,2,2,0, 0,0,0);
        addBlock2(0,0,0,2,1,1,2,2,2, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,0,0, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,0,2, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,2,0, 0,0,0);
        addBlock2(2,0,0,2,1,1,2,2,2, 0,0,0);
    }

    encodeLine(line, playerValue) {
        let cells = new Array(PATTERN_WINDOW);
        for (let i = 0; i < PATTERN_WINDOW; i++) {
            if (playerValue === 1) {
                cells[i] = line[i];
            } else {
                if (line[i] === 1) cells[i] = 2;
                else if (line[i] === 2) cells[i] = 1;
                else cells[i] = line[i];
            }
        }
        return this.makeKey(cells);
    }

    lookup(key) {
        this.ensureInitialized();
        return this.table.get(key) || null;
    }
}
