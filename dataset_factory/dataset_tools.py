import json
from typing import List, Dict, Any
from ollama import Client
import os


def to_json_schema(example: Dict[str, Any]) -> Dict[str, Any]:
    """
    将 example-style schema 转为 Ollama 支持的 JSON Schema
    """
    properties = {}
    required = []

    for key, value in example.items():
        required.append(key)

        if isinstance(value, bool):
            t = "boolean"
        elif isinstance(value, int):
            t = "integer"
        elif isinstance(value, float):
            t = "number"
        elif isinstance(value, list):
            t = "array"
        elif isinstance(value, dict):
            t = "object"
        else:
            t = "string"

        properties[key] = {"type": t}

    return {
        "type": "object",
        "properties": properties,
        "required": required,
        "additionalProperties": False,
    }


def generate_astrarp_data(
    base_url: str,
    model: str,
    system_prompt: str,
    history: List[Dict[str, str]],
    output_schema: Dict[str, Any],
    temperature: float = 0.7,
) -> Dict[str, Any]:
    """
    调用本地 Ollama 生成结构化 RP 训练数据
    调用时需要关闭代理，否则可能会出现 502 错误！
    :param base_url: Ollama API URL
    :param model: 模型名称 (如 qwen2:72b, llama3)
    :param system_prompt: 系统提示词，定义任务角色
    :param history: 对话历史，格式为 [{"role": "user", "content": "..."}]
    :param output_schema: 期望的 JSON 结构示例或描述
    :param temperature: 随机度
    :return: 解析后的 JSON 数据
    """

    # 1. 初始化 Client（支持远程）
    client = Client(host=base_url)

    # 2. 构造 messages（不再靠 prompt 约束 JSON）
    messages = [
        {"role": "system", "content": system_prompt},
        *history,
    ]

    # 3. 构造 JSON Schema
    json_schema = to_json_schema(output_schema)

    try:
        response = client.chat(
            model=model,
            messages=messages,
            format=json_schema,
            options={
                "temperature": temperature,
            },
        )

        content = response["message"]["content"]

        # 4. 兜底解析（理论上这里已经是合法 JSON）
        if isinstance(content, str):
            return json.loads(content)
        elif isinstance(content, dict):
            return content
        else:
            return {}

    except json.JSONDecodeError as e:
        print(f"[JSON 解析失败] {e}")
        print("原始输出：", content)
        return {}

    except Exception as e:
        print(f"[请求出错] {e}")
        return {}
