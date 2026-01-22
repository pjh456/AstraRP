import json
from typing import List, Dict, Any
from ollama import Client
from .schema import infer_json_schema


class OllamaChatClient:
    def __init__(self, base_url: str):
        self.client = Client(host=base_url)

    def run(
        self,
        model: str,
        system_prompt: str,
        history: List[Dict[str, str]],
        output_schema: Dict[str, Any],
        options: Dict[str, Any],
    ) -> Dict[str, Any]:
        schema = infer_json_schema(output_schema)
        resp = self.client.chat(
            model=model,
            messages=[
                {"role": "system", "content": system_prompt},
                *history,
            ],
            format=schema,
            options=options,
        )

        content = resp["message"]["content"]
        return json.loads(content) if isinstance(content, str) else content
