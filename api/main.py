from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List, Optional, Dict, Any
import subprocess
import json
import os
import sys
import time

app = FastAPI(
    title="五子棋AI对比测试系统",
    description="C++ vs JavaScript AI引擎对比接口",
    version="1.0.0"
)

# 允许跨域（前端调用）
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# 数据模型
class Move(BaseModel):
    x: int
    y: int


class GameRequest(BaseModel):
    boardSize: int = 15
    currentTurn: int = 0
    selfPlayer: int = 1  # 1=BLACK, 2=WHITE
    algorithm: str = "minimax"  # "minimax" or "mcts"
    searchDepth: int = 4
    iterations: int = 1000
    history: List[Move] = []
    engine: str = "cpp"  # "cpp" or "js"


class GameResponse(BaseModel):
    success: bool
    move: Optional[Dict[str, int]] = None
    engine: str = ""
    algorithm: str = ""
    nodesSearched: int = 0
    thinkTime: float = 0.0
    error: Optional[str] = None


# C++可执行文件路径
CPP_EXECUTABLE = None

def find_cpp_executable():
    """查找C++编译后的可执行文件"""
    global CPP_EXECUTABLE

    # 可能的位置列表
    possible_paths = [
        "../ver_api.exe",  # Windows编译后
        "../ver_api",      # Linux/Mac编译后
        "../../build/ver_api.exe",
        "ver_api.exe",
        "ver_api",
    ]

    for path in possible_paths:
        if os.path.exists(path):
            CPP_EXECUTABLE = path
            print(f"✅ 找到C++可执行文件: {path}")
            return True

    print("⚠️ 未找到C++可执行文件，将仅使用JS引擎")
    return False


def call_cpp_engine(request: GameRequest) -> Dict[str, Any]:
    """调用C++引擎获取最佳走法"""

    if not CPP_EXECUTABLE:
        raise HTTPException(status_code=500, detail="C++可执行文件未找到")

    # 构建输入数据（简单格式）
    input_data = f"# Gomoku AI Request\n"
    input_data += f"boardSize={request.boardSize}\n"
    input_data += f"selfPlayer={request.selfPlayer}\n"
    input_data += f"algorithm={request.algorithm}\n"

    if request.algorithm == "minimax":
        input_data += f"searchDepth={request.searchDepth}\n"
    else:
        input_data += f"iterations={request.iterations}\n"

    # 添加历史走法
    for move in request.history:
        input_data += f"move={move.x},{move.y}\n"

    try:
        start_time = time.time()

        # 调用C++程序
        process = subprocess.Popen(
            [CPP_EXECUTABLE],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )

        stdout, stderr = process.communicate(input=input_data, timeout=30.0)  # 30秒超时

        think_time = time.time() - start_time

        if process.returncode != 0:
            raise Exception(f"C++程序返回错误码 {process.returncode}: {stderr}")

        # 解析输出
        result = json.loads(stdout.strip())

        return {
            **result,
            "thinkTime": round(think_time * 1000, 2)  # 毫秒
        }

    except subprocess.TimeoutExpired:
        process.kill()
        raise HTTPException(status_code=504, detail="C++计算超时（>30秒）")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"C++引擎错误: {str(e)}")


@app.on_event("startup")
async def startup_event():
    """启动时查找C++可执行文件"""
    find_cpp_executable()


@app.get("/")
async def root():
    return {
        "message": "五子棋AI对比测试系统 API",
        "version": "1.0.0",
        "endpoints": {
            "/api/move": "POST - 获取AI最佳走法",
            "/api/compare": "POST - 对比C++和JS结果",
            "/health": "GET - 健康检查"
        }
    }


@app.get("/health")
async def health_check():
    return {
        "status": "healthy",
        "cpp_available": CPP_EXECUTABLE is not None,
        "cpp_path": CPP_EXECUTABLE
    }


@app.post("/api/move", response_model=GameResponse)
async def get_best_move(request: GameRequest):
    """
    获取AI最佳走法

    - engine: "cpp" (使用C++引擎) 或 "js" (预留，暂未实现)
    - algorithm: "minimax" 或 "mcts"
    """

    if request.engine == "cpp":
        result = call_cpp_engine(request)
        return GameResponse(**result)
    else:
        raise HTTPException(
            status_code=400,
            detail=f"不支持的引擎类型: {request.engine} (当前仅支持 'cpp')"
        )


@app.post("/api/compare")
async def compare_engines(request: GameRequest):
    """
    对比C++和JS两个引擎的结果

    返回两个引擎的最佳走法，用于验证一致性
    """

    results = {}

    # 调用C++引擎
    try:
        cpp_result = call_cpp_engine(request)
        results["cpp"] = cpp_result
    except Exception as e:
        results["cpp"] = {"error": str(e), "success": False}

    # JS引擎暂时返回占位符（后续可以实现）
    results["js"] = {
        "success": True,
        "move": {"x": -1, "y": -1},
        "engine": "javascript",
        "algorithm": request.algorithm,
        "note": "JS引擎请通过前端直接调用"
    }

    return {
        "comparison": results,
        "match": (
            results["cpp"].get("success", False) and
            results["cpp"].get("move", {}).get("x", -1) == results["js"].get("move", {}).get("x", -1)
        )
    }


@app.post("/api/benchmark")
async def benchmark_ai(request: GameRequest, runs: int = 5):
    """
    性能基准测试：运行多次并统计结果
    """

    times = []
    moves = []

    for i in range(runs):
        start = time.time()
        result = call_cpp_engine(request)
        elapsed = time.time() - start

        times.append(elapsed)
        if result.get("success"):
            moves.append(result["move"])

    return {
        "algorithm": request.algorithm,
        "runs": runs,
        "statistics": {
            "avg_time_ms": round(sum(times) / len(times) * 1000, 2),
            "min_time_ms": round(min(times) * 1000, 2),
            "max_time_ms": round(max(times) * 1000, 2),
            "total_time_s": round(sum(times), 2)
        },
        "moves_consistent": len(set((m["x"], m["y"]) for m in moves)) == 1 if moves else False,
        "sample_moves": moves[:3]
    }


if __name__ == "__main__":
    import uvicorn
    print("[INFO] Starting Gomoku AI Comparison System...")
    print("[INFO] API URL: http://localhost:8000")
    print("[INFO] API Docs: http://localhost:8000/docs")

    uvicorn.run(app, host="0.0.0.0", port=8000)
