/**
 * PatternDB 修复诊断工具
 * 精确定位每个模式的匹配状态
 */

function diagnosePatternFix() {
    console.log('🔍 PatternDB 修复精确诊断');
    console.log('='.repeat(70));
    
    const db = PatternDB.getInstance();
    db.ensureInitialized();
    
    let testResults = [];
    let passCount = 0;
    let failCount = 0;
    
    function diagnose(name, lineArray, expectedType) {
        const key = db.encodeLine(lineArray, 1);
        const result = db.lookup(key);
        
        const actualType = result ? result.type : PatternType.DEFAULT;
        const actualName = result ? 
            Object.keys(PatternType).find(k => PatternType[k] === result.type) : 
            'DEFAULT (未匹配)';
        
        const expectedName = Object.keys(PatternType).find(k => PatternType[k] === expectedType);
        const isPass = actualType === expectedType;
        
        if (isPass) {
            passCount++;
        } else {
            failCount++;
        }
        
        testResults.push({
            name: name,
            line: lineArray,
            expected: expectedName,
            actual: actualName,
            key: key.toString(16).toUpperCase(),
            isPass: isPass,
            hasResult: !!result
        });
        
        // 输出详细信息
        console.log(`\n${isPass ? '✅' : '❌'} ${name}`);
        console.log(`   输入数组: [${lineArray.join(',')}]`);
        console.log(`   编码Key:  0x${key.toString(16).toUpperCase().padStart(8, '0')}`);
        console.log(`   预期类型: ${expectedName}`);
        console.log(`   实际类型: ${actualName}`);
        if (!result) {
            console.log(`   ⚠️  PatternDB中不存在此模式！`);
        }
    }
    
    console.log('\n📋 第一组：P0 FLEX3 跳活三间隙防守（应识别为BLOCK3）\n');
    
    // 基本形态：_ X O X X _ _ _
    diagnose('FLEX3-1011-基本', [0,0,1,2,1,1,0,0,0], PatternType.BLOCK3);
    diagnose('FLEX3-1011-左封', [2,0,1,2,1,1,0,0,0], PatternType.BLOCK3);
    diagnose('FLEX3-1011-右封', [0,0,1,2,1,1,2,0,0], PatternType.BLOCK3);
    diagnose('FLEX3-1011-双封', [2,0,1,2,1,1,2,2,0], PatternType.BLOCK3);
    
    // 基本形态：_ X X O X _ _ _
    diagnose('FLEX3-1101-基本', [0,0,1,1,2,1,0,0,0], PatternType.BLOCK3);
    diagnose('FLEX3-1101-左封', [2,0,1,1,2,1,0,0,0], PatternType.BLOCK3);
    diagnose('FLEX3-1101-右封', [0,0,1,1,2,1,2,0,0], PatternType.BLOCK3);
    
    // 偏移形态
    diagnose('FLEX3-1011-偏移', [0,0,0,1,2,1,1,0,0], PatternType.BLOCK3);
    diagnose('FLEX3-1101-偏移', [0,0,0,1,1,2,1,0,0], PatternType.BLOCK3);
    
    console.log('\n📋 第二组：P1 FLEX2 跳活二间隙防守（应识别为BLOCK2）\n');
    
    // 基本形态：_ _ X O X _ _ _
    diagnose('FLEX2-101-基本', [0,0,0,1,2,1,0,0,0], PatternType.BLOCK2);
    diagnose('FLEX2-101-左封', [2,0,0,1,2,1,0,0,0], PatternType.BLOCK2);
    diagnose('FLEX2-101-右封', [0,0,0,1,2,1,2,0,0], PatternType.BLOCK2);
    
    // 1001 形态
    diagnose('FLEX2-1001-基本', [0,0,0,1,2,0,1,0,0], PatternType.BLOCK2);
    
    // 偏移形态
    diagnose('FLEX2-101-偏移', [0,0,0,0,1,2,1,0,0], PatternType.BLOCK2);
    
    console.log('\n' + '='.repeat(70));
    console.log('📊 诊断结果汇总');
    console.log('='.repeat(70));
    console.log(`总测试数: ${testResults.length}`);
    console.log(`✅ 通过: ${passCount}`);
    console.log(`❌ 失败: ${failCount}`);
    console.log(`通过率: ${(passCount/testResults.length*100).toFixed(1)}%`);
    
    if (failCount > 0) {
        console.log('\n⚠️  失败的测试用例:');
        testResults.filter(r => !r.isPass).forEach((r, i) => {
            console.log(`\n${i+1}. ${r.name}`);
            console.log(`   数组: [${r.line.join(',')}]`);
            console.log(`   Key:  0x${r.key}`);
            console.log(`   预期: ${r.expected} → 实际: ${r.actual}`);
            
            // 提供修复建议
            console.log(`   🔧 修复建议:`);
            console.log(`      在 pattern.js 的 addBlock3/addBlock2 函数中添加:`);
            console.log(`      addBlock${r.actual.includes('BLOCK3') ? '3' : '2'}(${r.line.join(',')}, ...);`);
        });
    } else {
        console.log('\n🎉 所有测试全部通过！PatternDB 修复完美！');
    }
    
    return {
        total: testResults.length,
        passed: passCount,
        failed: failCount,
        results: testResults
    };
}

window.diagnosePatternFix = diagnosePatternFix;
console.log('✅ 诊断工具已加载，请运行: diagnosePatternFix()');
