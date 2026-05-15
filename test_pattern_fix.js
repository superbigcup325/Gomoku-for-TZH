/**
 * PatternDB 修复验证测试
 * 用于验证 P0-P3 修复是否生效
 * 
 * 使用方法：
 * 1. 在 index.html 中临时添加: <script src="test_pattern_fix.js"></script>
 * 2. 打开浏览器控制台
 * 3. 运行 runPatternFixTests()
 */

function runPatternFixTests() {
    console.log('🧪 PatternDB 修复验证测试');
    console.log('='.repeat(60));
    
    const db = PatternDB.getInstance();
    db.ensureInitialized();
    
    let totalTests = 0;
    let passedTests = 0;
    let failedTests = 0;
    
    function testCase(name, lineArray, expectedType, expectedMinScore = 0) {
        totalTests++;
        const key = db.encodeLine(lineArray, 1);
        const result = db.lookup(key);
        
        const typeName = result ? Object.keys(PatternType).find(k => PatternType[k] === result.type) : 'DEFAULT';
        const score = result ? getPatternScore(result.type) : 0;
        
        const typeMatch = result && result.type === expectedType;
        const scoreOk = score >= expectedMinScore;
        
        if (typeMatch && scoreOk) {
            passedTests++;
            console.log(`✅ ${name}`);
            console.log(`   模式: [${lineArray.join(',')}] → ${typeName} (${score}分)`);
        } else {
            failedTests++;
            console.log(`❌ ${name}`);
            console.log(`   模式: [${lineArray.join(',')}]`);
            console.log(`   预期: ${Object.keys(PatternType).find(k => PatternType[k] === expectedType)} (≥${expectedMinScore}分)`);
            console.log(`   实际: ${typeName} (${score}分)`);
            if (!result) {
                console.log(`   ⚠️ 未匹配到任何模式！`);
            }
        }
        console.log('');
    }
    
    function getPatternScore(type) {
        switch(type) {
            case PatternType.FIVE: case PatternType.OVERLINE: return 10000000;
            case PatternType.FLEX4: return 5000000;
            case PatternType.BLOCK4: return 3000000;
            case PatternType.FLEX3: return 50000;
            case PatternType.BLOCK3: return 1000;
            case PatternType.FLEX2: return 200;
            case PatternType.BLOCK2: return 50;
            case PatternType.FLEX1: return 10;
            case PatternType.BLOCK1: return 2;
            default: return 0;
        }
    }
    
    // ============================================
    // P0 测试：FLEX3 间隙防守 → 应识别为 BLOCK3
    // ============================================
    console.log('\n📋 P0 测试组：FLEX3 跳活三间隙防守\n');
    
    // 跳活三 1011 (X_XX) - 间隙[3]被填
    testCase(
        '跳活三1011 - 间隙被填(基本形态)',
        [0, 0, 1, 2, 1, 1, 0, 0, 0],  // _ X O X X _ _ _
        PatternType.BLOCK3,
        1000  // BLOCK3 基础分
    );
    
    testCase(
        '跳活三1011 - 间隙被填(左端被封)',
        [2, 0, 1, 2, 1, 1, 0, 0, 0],  // O X O X X _ _ _
        PatternType.BLOCK3,
        1000
    );
    
    testCase(
        '跳活三1011 - 间隙被填(右端被封)',
        [0, 0, 1, 2, 1, 1, 2, 0, 0],  // _ X O X X O _ _
        PatternType.BLOCK3,
        1000
    );
    
    // 跳活三 1101 (XX_X) - 间隙[4]被填
    testCase(
        '跳活三1101 - 间隙被填(基本形态)',
        [0, 0, 1, 1, 2, 1, 0, 0, 0],  // _ X X O X _ _ _
        PatternType.BLOCK3,
        1000
    );
    
    testCase(
        '跳活三1101 - 间隙被填(左端被封)',
        [2, 0, 1, 1, 2, 1, 0, 0, 0],  // O X X O X _ _ _
        PatternType.BLOCK3,
        1000
    );
    
    // 跳活三偏移变体
    testCase(
        '跳活三1011偏移 - 间隙被填',
        [0, 0, 0, 1, 2, 1, 1, 0, 0],  // _ _ X O X X _ _
        PatternType.BLOCK3,
        1000
    );
    
    testCase(
        '跳活三1101偏移 - 间隙被填',
        [0, 0, 0, 1, 1, 2, 1, 0, 0],  // _ _ X X O X _ _
        PatternType.BLOCK3,
        1000
    );
    
    // ============================================
    // P1 测试：FLEX2 间隙防守 → 应识别为 BLOCK2
    // ============================================
    console.log('📋 P1 测试组：FLEX2 跳活二间隙防守\n');
    
    // 跳活二 101 (X_X) - 间隙被填
    testCase(
        '跳活二101 - 间隙被填(基本形态)',
        [0, 0, 0, 1, 2, 1, 0, 0, 0],  // _ _ X O X _ _ _
        PatternType.BLOCK2,
        50  // BLOCK2 基础分
    );
    
    testCase(
        '跳活二101 - 间隙被填(左端被封)',
        [2, 0, 0, 1, 2, 1, 0, 0, 0],  // O _ X O X _ _ _
        PatternType.BLOCK2,
        50
    );
    
    // 跳活二 1001 (X__X) - 第一间隙被填
    testCase(
        '跳活二1001 - 第一间隙被填',
        [0, 0, 0, 1, 2, 0, 1, 0, 0],  // _ _ X O _ X _ _
        PatternType.BLOCK2,
        50
    );
    
    // 跳活二偏移
    testCase(
        '跳活二101偏移 - 间隙被填',
        [0, 0, 0, 0, 1, 2, 1, 0, 0],  // _ _ _ X O X _ _
        PatternType.BLOCK2,
        50
    );
    
    // ============================================
    // 对比测试：修复前后差异验证
    // ============================================
    console.log('📋 对比测试：修复前后的关键场景\n');
    
    // 场景：对手有跳活三，AI考虑在间隙位防守
    console.log('🎯 场景：白棋跳活三 ○_○○，AI黑棋考虑在间隙位落子\n');
    
    const flex3GapDefend = [0, 0, 1, 2, 1, 1, 0, 0, 0];  // _ X O X X _ _ _
    const key3 = db.encodeLine(flex3GapDefend, 1);
    const result3 = db.lookup(key3);
    
    if (result3 && result3.type === PatternType.BLOCK3) {
        console.log('✅ 修复成功！间隙防守位置现可正确识别为 BLOCK3');
        console.log(`   评分: ${getPatternScore(result3.type)} 分（修复前为 0 分）`);
        console.log(`   影响: AI 现在会优先选择此位置进行防守`);
        passedTests++;
    } else {
        console.log('❌ 修复失败！间隙防守位置仍未被识别');
        failedTests++;
    }
    console.log('');
    
    // 场景：对手有跳活二，AI考虑在间隙位防守
    console.log('🎯 场景：白棋跳活二 ○_○，AI黑棋考虑在间隙位落子\n');
    
    const flex2GapDefend = [0, 0, 0, 1, 2, 1, 0, 0, 0];  // _ _ X O X _ _ _
    const key2 = db.encodeLine(flex2GapDefend, 1);
    const result2 = db.lookup(key2);
    
    if (result2 && result2.type === PatternType.BLOCK2) {
        console.log('✅ 修复成功！间隙防守位置现可正确识别为 BLOCK2');
        console.log(`   评分: ${getPatternScore(result2.type)} 分（修复前为 0 分）`);
        console.log(`   影响: AI 会将此位置纳入防守候选`);
        passedTests++;
    } else {
        console.log('❌ 修复失败！间隙防守位置仍未被识别');
        failedTests++;
    }
    console.log('');
    
    // ============================================
    // 测试结果汇总
    // ============================================
    console.log('='.repeat(60));
    console.log('📊 测试结果汇总');
    console.log('='.repeat(60));
    console.log(`总测试数: ${totalTests}`);
    console.log(`✅ 通过: ${passedTests} (${(passedTests/totalTests*100).toFixed(1)}%)`);
    console.log(`❌ 失败: ${failedTests} (${(failedTests/totalTests*100).toFixed(1)}%)`);
    console.log('='.repeat(60));
    
    if (failedTests === 0) {
        console.log('🎉 所有测试通过！PatternDB 修复验证成功！');
    } else {
        console.log('⚠️ 存在失败的测试，请检查上述错误信息');
    }
    
    return {
        total: totalTests,
        passed: passedTests,
        failed: failedTests,
        successRate: (passedTests / totalTests * 100).toFixed(1) + '%'
    };
}

// 导出函数
window.runPatternFixTests = runPatternFixTests;
console.log('✅ PatternDB 修复测试已加载');
console.log('请在控制台运行: runPatternFixTests()');
