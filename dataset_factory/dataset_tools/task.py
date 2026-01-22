import json
from dataclasses import dataclass
from typing import Dict, Any


@dataclass
class TaskDefinition:
    name: str
    system_prompt: str
    output_schema: Dict[str, Any]
    options: Dict[str, Any]

    @staticmethod
    def from_json(path: str) -> "TaskDefinition":
        with open(path, "r", encoding="utf-8") as f:
            raw = json.load(f)

        return TaskDefinition(
            name=raw["name"],
            system_prompt=raw["system_prompt"],
            output_schema=raw["output_schema"],
            options=raw.get("options", {}),
        )
