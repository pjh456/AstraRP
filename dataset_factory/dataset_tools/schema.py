from typing import Dict, Any


def infer_json_schema(example: Dict[str, Any]) -> Dict[str, Any]:
    properties = {}
    required = []

    for k, v in example.items():
        required.append(k)
        if isinstance(v, bool):
            t = "boolean"
        elif isinstance(v, int):
            t = "integer"
        elif isinstance(v, float):
            t = "number"
        elif isinstance(v, list):
            t = "array"
        elif isinstance(v, dict):
            t = "object"
        else:
            t = "string"
        properties[k] = {"type": t}

    return {
        "type": "object",
        "properties": properties,
        "required": required,
        "additionalProperties": False,
    }
