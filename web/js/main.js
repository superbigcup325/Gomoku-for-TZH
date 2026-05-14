document.addEventListener('DOMContentLoaded', function() {
    console.log('🎮 五子棋AI系统初始化...');

    // 显示加载动画
    showLoading('正在初始化AI引擎...');

    // 使用setTimeout让UI先渲染
    setTimeout(function() {
        try {
            // 初始化UI（轻量级操作）
            const ui = new GomokuUI('gameBoard');

            // 初始化游戏（此时会触发PatternDB懒加载）
            const game = new GomokuGame();
            game.init(ui);

            // 绑定控制面板事件
            bindControlEvents(game);

            // 隐藏加载动画
            hideLoading();

            // 显示初始消息
            game.showMessage('系统就绪！请开始新游戏', 'success');
            console.log('✅ 系统初始化完成');

        } catch (error) {
            console.error('❌ 初始化失败:', error);
            hideLoading();
            alert('系统初始化失败：' + error.message + '\n请刷新页面重试');
        }
    }, 100);  // 给UI 100ms 渲染时间
});

function bindControlEvents(game) {
    // 开始新游戏按钮
    const startBtn = document.getElementById('startBtn');
    startBtn.addEventListener('click', function() {
        game.startNewGame();
    });

    // 悔棋按钮
    const undoBtn = document.getElementById('undoBtn');
    undoBtn.addEventListener('click', function() {
        game.undo();
    });

    // 游戏模式切换
    const gameModeSelect = document.getElementById('gameMode');
    gameModeSelect.addEventListener('change', function() {
        updateAISettingsVisibility();
    });

    // AI算法切换
    const aiAlgorithmSelect = document.getElementById('aiAlgorithm');
    aiAlgorithmSelect.addEventListener('change', function() {
        updateAISettingsVisibility();
    });

    // 初始化显示状态
    updateAISettingsVisibility();

    console.log('✅ 控制面板事件绑定完成');
}

function updateAISettingsVisibility() {
    const gameMode = document.getElementById('gameMode').value;
    const aiAlgorithm = document.getElementById('aiAlgorithm').value;

    const aiSettings = document.getElementById('aiSettings');
    const engineSettings = document.getElementById('engineSettings');
    const minimaxSettings = document.getElementById('minimaxSettings');
    const mctsSettings = document.getElementById('mctsSettings');

    if (gameMode === 'pve') {
        aiSettings.style.display = 'block';
        engineSettings.style.display = 'block';

        if (aiAlgorithm === 'minimax') {
            minimaxSettings.style.display = 'block';
            mctsSettings.style.display = 'none';
        } else {
            minimaxSettings.style.display = 'none';
            mctsSettings.style.display = 'block';
        }
    } else {
        aiSettings.style.display = 'none';
        engineSettings.style.display = 'none';
        minimaxSettings.style.display = 'none';
        mctsSettings.style.display = 'none';
    }
}

// 工具函数：显示加载动画
function showLoading(message = '加载中...') {
    const overlay = document.createElement('div');
    overlay.className = 'loading-overlay';
    overlay.id = 'loadingOverlay';
    overlay.innerHTML = `
        <div class="loading-spinner">
            <div>${message}</div>
        </div>
    `;
    document.body.appendChild(overlay);
}

// 工具函数：隐藏加载动画
function hideLoading() {
    const overlay = document.getElementById('loadingOverlay');
    if (overlay) {
        overlay.remove();
    }
}

// 错误处理
window.onerror = function(msg, url, line, col, error) {
    console.error('全局错误:', msg, '\n位置:', url, ':', line);
    return false;
};

// 性能监控（可选）
if (window.performance) {
    window.addEventListener('load', function() {
        setTimeout(function() {
            const perfData = performance.getEntriesByType('navigation')[0];
            if (perfData) {
                console.log(`⏱️ 页面加载时间: ${Math.round(perfData.loadEventEnd - perfData.startTime)}ms`);
            }
        }, 0);
    });
}
