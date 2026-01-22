import os
from dotenv import load_dotenv, find_dotenv
from dataset_tools.config import load_ollama_config

load_dotenv(find_dotenv(usecwd=True))
cfg = load_ollama_config()

from dataset_tools.batch_runner import run_task_batch

BASE_PATH = os.path.dirname(os.path.dirname(__file__))
TASK_PATH = os.path.join(BASE_PATH, "tasks")
OUTPUT_PATH = os.path.join(BASE_PATH, "datasets")

history = [
    {
        "role": "user",
        "content": "（递给你一盒亲手做的巧克力）这是给你的，虽然做得不太好，但不许拒绝！",
    }
]

run_task_batch(
    task_dir=TASK_PATH,
    output_dir=OUTPUT_PATH,
    history=history,
    config=cfg,
    total_runs=6,
    merge_every=2,
    basename="data",
    overwrite=False,
)
