from pathlib import Path
from typing import List, Dict, Any, Iterable
import json
import re
from tqdm import tqdm

from .task import TaskDefinition
from .runner import run_task
from .config import OllamaConfig


def iter_task_files(task_dir: Path) -> Iterable[Path]:
    for p in sorted(task_dir.iterdir()):
        if p.is_file() and p.suffix == ".json":
            yield p


def find_next_index(dir_path: Path, basename: str) -> int:
    """
    在目录中查找 basenameN.json 的最大 N，并返回 N+1
    """
    pattern = re.compile(rf"^{re.escape(basename)}(\d+)\.json$")
    max_idx = 0

    for p in dir_path.iterdir():
        if not p.is_file():
            continue
        m = pattern.match(p.name)
        if m:
            max_idx = max(max_idx, int(m.group(1)))

    return max_idx + 1


def run_task_batch(
    task_dir: str | Path,
    output_dir: str | Path,
    history: List[Dict[str, str]],
    config: OllamaConfig,
    *,
    total_runs: int,
    merge_every: int = 1,
    basename: str = "data",
    overwrite: bool = False,
) -> None:
    """
    批量运行 task JSON，并将结果流式写入磁盘

    :param overwrite:
        True  -> 清空已有输出，从 data1.json 开始
        False -> 追加写入，不覆盖已有文件（默认）
    """

    task_dir = Path(task_dir)
    output_dir = Path(output_dir)

    if merge_every <= 0:
        raise ValueError("merge_every 必须 >= 1")

    task_files = list(iter_task_files(task_dir))

    for task_file in tqdm(task_files, desc="Running tasks", unit="task"):
        task = TaskDefinition.from_json(str(task_file))

        task_out_dir = output_dir / task.name
        task_out_dir.mkdir(parents=True, exist_ok=True)

        if overwrite:
            for p in task_out_dir.glob(f"{basename}*.json"):
                p.unlink()
            file_idx = 1
        else:
            file_idx = find_next_index(task_out_dir, basename)

        buffer: List[Dict[str, Any]] = []

        for _ in tqdm(
            range(total_runs),
            desc=f"  {task.name}",
            unit="run",
            leave=False,
        ):
            result = run_task(task, history, config)
            buffer.append(result)

            if len(buffer) >= merge_every:
                out_path = task_out_dir / f"{basename}{file_idx}.json"
                with open(out_path, "w", encoding="utf-8") as f:
                    json.dump(buffer, f, ensure_ascii=False, indent=2)

                buffer.clear()
                file_idx += 1

        if buffer:
            out_path = task_out_dir / f"{basename}{file_idx}.json"
            with open(out_path, "w", encoding="utf-8") as f:
                json.dump(buffer, f, ensure_ascii=False, indent=2)
