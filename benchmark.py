import subprocess
import itertools
import json
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, asdict
from typing import List, Dict, Tuple
import matplotlib.pyplot as plt
import matplotlib
import numpy as np

matplotlib.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'Arial Unicode MS']
matplotlib.rcParams['axes.unicode_minus'] = False


def print_progress_bar(completed: int, total: int, bar_length: int = 40):
    """打印可视化进度条"""
    percent = completed / total
    filled_length = int(bar_length * percent)
    bar = '█' * filled_length + '░' * (bar_length - filled_length)
    eta_str = ""
    if completed > 0:
        elapsed = time.time() - print_progress_bar.start_time
        eta = elapsed * (total - completed) / completed
        eta_str = f" ETA: {eta:.1f}s"
    
    sys.stdout.write(f"\r[{'='*50}] {completed}/{total} [{bar}] {percent:.1%}{eta_str}")
    sys.stdout.flush()


print_progress_bar.start_time = 0


@dataclass
class MatchConfig:
    board_size: int = 38
    minimax_depth: int = 5
    defend_weight: float = 1.12
    mcts_iterations: int = 50000
    minimax_first: bool = False

    def to_args(self) -> List[str]:
        if self.minimax_first:
            minimax_color = "black"
            mcts_color = "white"
        else:
            minimax_color = "white"
            mcts_color = "black"
        return [
            "--size", str(self.board_size),
            "--minimax-color", minimax_color,
            "--minimax-depth", str(self.minimax_depth),
            "--defend-weight", str(self.defend_weight),
            "--mcts-color", mcts_color,
            "--mcts-iterations", str(self.mcts_iterations),
        ]

    def to_key(self) -> str:
        first = "minimax_first" if self.minimax_first else "mcts_first"
        return f"d{self.minimax_depth}_w{self.defend_weight}_i{self.mcts_iterations}_{first}"

    def to_label(self) -> str:
        first = "Minimax先" if self.minimax_first else "MCTS先"
        return f"深度{self.minimax_depth}/权重{self.defend_weight}/MCTS{self.mcts_iterations}/{first}"


@dataclass
class MatchResult:
    config: MatchConfig
    winner: str
    output: str
    elapsed_ms: float


class Compiler:
    def __init__(self, source_dir: str, exe_name: str = "benchmark_vs.exe"):
        self.source_dir = source_dir
        self.exe_path = os.path.join(source_dir, exe_name)
        self.source_file = os.path.join(source_dir, "ver_minimax_vs_mcts.cpp")

    def needs_rebuild(self) -> bool:
        if not os.path.exists(self.exe_path):
            return True
        exe_mtime = os.path.getmtime(self.exe_path)
        for fname in ["ver_minimax_vs_mcts.cpp", "board.h", "minimax.h", "mcts.h"]:
            fpath = os.path.join(self.source_dir, fname)
            if os.path.exists(fpath) and os.path.getmtime(fpath) > exe_mtime:
                return True
        return False

    def build(self) -> bool:
        if not self.needs_rebuild():
            print(f"[Compiler] 可执行文件已是最新: {self.exe_path}")
            return True
        cmd = [
            "g++", "-std=c++17", "-O2", "-o", self.exe_path,
            self.source_file
        ]
        print(f"[Compiler] 编译命令: {' '.join(cmd)}")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=self.source_dir)
            if result.returncode != 0:
                print(f"[Compiler] 编译失败:\n{result.stderr}")
                return False
            print(f"[Compiler] 编译成功: {self.exe_path}")
            return True
        except FileNotFoundError:
            print("[Compiler] 错误: 找不到g++编译器，请确保MinGW或GCC已安装并加入PATH")
            return False


class MatchExecutor:
    def __init__(self, exe_path: str, timeout: int = 300):
        self.exe_path = exe_path
        self.timeout = timeout

    def run_single(self, config: MatchConfig) -> MatchResult:
        cmd = [self.exe_path] + config.to_args()
        start = time.perf_counter()
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=self.timeout
            )
            elapsed = (time.perf_counter() - start) * 1000
            output = result.stdout.strip()
            winner = self._parse_winner(output)
            return MatchResult(config, winner, output, elapsed)
        except subprocess.TimeoutExpired:
            elapsed = (time.perf_counter() - start) * 1000
            return MatchResult(config, "timeout", "", elapsed)
        except Exception as e:
            elapsed = (time.perf_counter() - start) * 1000
            return MatchResult(config, "error", str(e), elapsed)

    @staticmethod
    def _parse_winner(output: str) -> str:
        lines = output.lower().splitlines()
        for line in lines:
            if "minimax win" in line:
                return "minimax"
            if "mcts win" in line:
                return "mcts"
            if "draw" in line:
                return "draw"
        return "unknown"


def run_match_task(args: Tuple[str, MatchConfig]) -> MatchResult:
    exe_path, config = args
    executor = MatchExecutor(exe_path)
    return executor.run_single(config)


class Benchmark:
    def __init__(self, source_dir: str, rounds_per_config: int = 10, max_workers: int = None):
        self.source_dir = source_dir
        self.rounds_per_config = rounds_per_config
        self.max_workers = max_workers or os.cpu_count()
        self.compiler = Compiler(source_dir)
        self.results: List[MatchResult] = []

    def generate_configs(self) -> List[MatchConfig]:
        default_depth = 5
        default_weight = 1.12
        default_iterations = 50000
        first_flags = [True, False]

        configs = []

        # 控制变量法：每次只变化一个参数
        # 1. 变化 minimax_depth
        for d in [3, 4, 5, 6]:
            for first in first_flags:
                c = MatchConfig(
                    minimax_depth=d,
                    defend_weight=default_weight,
                    mcts_iterations=default_iterations,
                    minimax_first=first
                )
                configs.extend([c] * self.rounds_per_config)

        # 2. 变化 defend_weight
        for w in [1.0, 1.1, 1.12, 1.2, 1.3]:
            for first in first_flags:
                c = MatchConfig(
                    minimax_depth=default_depth,
                    defend_weight=w,
                    mcts_iterations=default_iterations,
                    minimax_first=first
                )
                configs.extend([c] * self.rounds_per_config)

        # 3. 变化 mcts_iterations
        for i in [10000, 30000, 50000, 100000]:
            for first in first_flags:
                c = MatchConfig(
                    minimax_depth=default_depth,
                    defend_weight=default_weight,
                    mcts_iterations=i,
                    minimax_first=first
                )
                configs.extend([c] * self.rounds_per_config)

        return configs

    def run(self):
        if not self.compiler.build():
            sys.exit(1)

        configs = self.generate_configs()
        total = len(configs)
        print(f"[Benchmark] 总对局数: {total} (参数组合: {total // self.rounds_per_config}, 每配置{self.rounds_per_config}盘)")
        print(f"[Benchmark] 并行 workers: {self.max_workers}")

        self.results = []
        completed = 0
        tasks = [(self.compiler.exe_path, cfg) for cfg in configs]

        # 初始化进度条起始时间
        print_progress_bar.start_time = time.time()

        try:
            with ProcessPoolExecutor(max_workers=self.max_workers) as pool:
                futures = {pool.submit(run_match_task, t): t for t in tasks}
                for future in as_completed(futures):
                    result = future.result()
                    self.results.append(result)
                    completed += 1
                    print_progress_bar(completed, total)
        except KeyboardInterrupt:
            print("\n[Benchmark] 用户中断，保存已完成的结果...")
        except Exception as e:
            print(f"\n[Benchmark] 运行过程中发生错误: {e}")
        finally:
            # 完成后换行
            print()

        if self.results:
            self._save_results()
            self._analyze()
            self._plot()
        else:
            print("[Benchmark] 没有完成任何对局")

    def _save_results(self):
        data = []
        for r in self.results:
            data.append({
                "config": asdict(r.config),
                "winner": r.winner,
                "elapsed_ms": r.elapsed_ms,
            })
        path = os.path.join(self.source_dir, "benchmark_results.json")
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        print(f"[Benchmark] 结果已保存: {path}")

    def _analyze(self):
        stats: Dict[str, Dict] = {}
        for r in self.results:
            key = r.config.to_key()
            label = r.config.to_label()
            if key not in stats:
                stats[key] = {
                    "label": label,
                    "minimax": 0,
                    "mcts": 0,
                    "draw": 0,
                    "timeout": 0,
                    "error": 0,
                    "total": 0,
                    "avg_time_ms": 0.0,
                }
            s = stats[key]
            s[r.winner] += 1
            s["total"] += 1
            s["avg_time_ms"] += r.elapsed_ms

        for s in stats.values():
            s["avg_time_ms"] /= s["total"]

        print("\n" + "="*80)
        print("统计结果")
        print("="*80)
        for key in sorted(stats.keys()):
            s = stats[key]
            total = s["total"]
            print(f"\n{s['label']}")
            print(f"  Minimax胜: {s['minimax']} ({s['minimax']*100/total:.1f}%)")
            print(f"  MCTS胜:    {s['mcts']} ({s['mcts']*100/total:.1f}%)")
            print(f"  平局:      {s['draw']} ({s['draw']*100/total:.1f}%)")
            if s["timeout"] > 0:
                print(f"  超时:      {s['timeout']}")
            if s["error"] > 0:
                print(f"  错误:      {s['error']}")
            print(f"  平均耗时:  {s['avg_time_ms']:.1f}ms")

        self.stats = stats

    def _plot(self):
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle("Minimax vs MCTS 基准测试结果", fontsize=16)

        self._plot_depth_winrate(axes[0, 0])
        self._plot_weight_winrate(axes[0, 1])
        self._plot_mcts_iter_winrate(axes[1, 0])
        self._plot_first_move_winrate(axes[1, 1])

        plt.tight_layout(rect=[0, 0.03, 1, 0.95])
        plot_path = os.path.join(self.source_dir, "benchmark_plot.png")
        plt.savefig(plot_path, dpi=150, bbox_inches="tight")
        print(f"\n[Benchmark] 图表已保存: {plot_path}")
        plt.show()

    def _aggregate_by(self, key_func) -> Dict:
        agg: Dict[str, Dict] = {}
        for r in self.results:
            k = key_func(r.config)
            if k not in agg:
                agg[k] = {"minimax": 0, "mcts": 0, "draw": 0, "total": 0}
            agg[k][r.winner] += 1
            agg[k]["total"] += 1
        return agg

    def _plot_depth_winrate(self, ax):
        data = self._aggregate_by(lambda c: c.minimax_depth)
        depths = sorted(data.keys())
        minimax_rates = [data[d]["minimax"] / data[d]["total"] * 100 for d in depths]
        mcts_rates = [data[d]["mcts"] / data[d]["total"] * 100 for d in depths]
        draw_rates = [data[d]["draw"] / data[d]["total"] * 100 for d in depths]

        ax.plot(depths, minimax_rates, "o-", label="Minimax胜率", linewidth=2)
        ax.plot(depths, mcts_rates, "s-", label="MCTS胜率", linewidth=2)
        ax.plot(depths, draw_rates, "^-", label="平局率", linewidth=2)
        ax.set_xlabel("Minimax搜索深度")
        ax.set_ylabel("胜率 (%)")
        ax.set_title("不同搜索深度下的胜率")
        ax.legend()
        ax.grid(True, alpha=0.3)

    def _plot_weight_winrate(self, ax):
        data = self._aggregate_by(lambda c: c.defend_weight)
        weights = sorted(data.keys())
        minimax_rates = [data[w]["minimax"] / data[w]["total"] * 100 for w in weights]
        mcts_rates = [data[w]["mcts"] / data[w]["total"] * 100 for w in weights]
        draw_rates = [data[w]["draw"] / data[w]["total"] * 100 for w in weights]

        ax.plot(weights, minimax_rates, "o-", label="Minimax胜率", linewidth=2)
        ax.plot(weights, mcts_rates, "s-", label="MCTS胜率", linewidth=2)
        ax.plot(weights, draw_rates, "^-", label="平局率", linewidth=2)
        ax.set_xlabel("防守权重")
        ax.set_ylabel("胜率 (%)")
        ax.set_title("不同防守权重下的胜率")
        ax.legend()
        ax.grid(True, alpha=0.3)

    def _plot_mcts_iter_winrate(self, ax):
        data = self._aggregate_by(lambda c: c.mcts_iterations)
        iters = sorted(data.keys())
        minimax_rates = [data[i]["minimax"] / data[i]["total"] * 100 for i in iters]
        mcts_rates = [data[i]["mcts"] / data[i]["total"] * 100 for i in iters]
        draw_rates = [data[i]["draw"] / data[i]["total"] * 100 for i in iters]

        ax.plot(iters, minimax_rates, "o-", label="Minimax胜率", linewidth=2)
        ax.plot(iters, mcts_rates, "s-", label="MCTS胜率", linewidth=2)
        ax.plot(iters, draw_rates, "^-", label="平局率", linewidth=2)
        ax.set_xlabel("MCTS迭代次数")
        ax.set_ylabel("胜率 (%)")
        ax.set_title("不同MCTS迭代次数下的胜率")
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_xscale("log")

    def _plot_first_move_winrate(self, ax):
        data = self._aggregate_by(lambda c: "Minimax先" if c.minimax_first else "MCTS先")
        order = ["MCTS先", "Minimax先"]
        minimax_rates = [data[o]["minimax"] / data[o]["total"] * 100 for o in order]
        mcts_rates = [data[o]["mcts"] / data[o]["total"] * 100 for o in order]
        draw_rates = [data[o]["draw"] / data[o]["total"] * 100 for o in order]

        x = np.arange(len(order))
        width = 0.25
        ax.bar(x - width, minimax_rates, width, label="Minimax胜率")
        ax.bar(x, mcts_rates, width, label="MCTS胜率")
        ax.bar(x + width, draw_rates, width, label="平局率")
        ax.set_xticks(x)
        ax.set_xticklabels(order)
        ax.set_ylabel("胜率 (%)")
        ax.set_title("先后手对胜率的影响")
        ax.legend()
        ax.grid(True, alpha=0.3, axis="y")


def main():
    source_dir = os.path.dirname(os.path.abspath(__file__))
    benchmark = Benchmark(source_dir, rounds_per_config=20)
    benchmark.run()


if __name__ == "__main__":
    main()
