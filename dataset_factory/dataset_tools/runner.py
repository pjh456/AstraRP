from typing import List, Dict, Any
from .task import TaskDefinition
from .client import OllamaChatClient
from .config import OllamaConfig


def run_task(
    task: TaskDefinition, history: List[Dict[str, str]], config: OllamaConfig
) -> Dict[str, Any]:
    client = OllamaChatClient(config.base_url)
    return client.run(
        model=config.model,
        system_prompt=task.system_prompt,
        history=history,
        output_schema=task.output_schema,
        options=task.options,
    )
