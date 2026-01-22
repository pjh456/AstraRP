import json
from typing import Dict, Any


def pretty_print(result: Dict[str, Any]):
    print(json.dumps(result, ensure_ascii=False, indent=4))


def dry_run(task):
    print("=== TASK ===")
    print(task.name)
    print(task.system_prompt)
    print("Schema:", json.dumps(task.output_schema, indent=2))
