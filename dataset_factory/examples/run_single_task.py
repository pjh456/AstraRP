import os
from dotenv import load_dotenv, find_dotenv
from dataset_tools.config import load_ollama_config

load_dotenv(find_dotenv(usecwd=True))
cfg = load_ollama_config()

from dataset_tools.task import TaskDefinition
from dataset_tools.runner import run_task
from dataset_tools.utils.test import pretty_print

TASK_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "tasks")

task = TaskDefinition.from_json(os.path.join(TASK_PATH, "intent_emotion.json"))

history = [
    {
        "role": "user",
        "content": "（递给你一盒亲手做的巧克力）这是给你的，虽然做得不太好，但不许拒绝！",
    }
]

result = run_task(task, history, cfg)
pretty_print(result)
