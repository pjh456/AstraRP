from dataset_tools import generate_astrarp_data
import json
import os

os.environ["http_proxy"] = ""
os.environ["https_proxy"] = ""
os.environ["no_proxy"] = "*"

# 1. 定义任务：模拟一个感知层模型的标注任务
task_system_prompt = """
你是一个高级对话分析专家。你的任务是分析角色扮演中用户的最后一句话。
你需要判断：
1. 用户的真实意图（intent）。
2. 用户的情绪（emotion）。
3. 这一句话对角色好感度的影响（affinity_score: -10 到 10）。
"""

# 2. 定义对话上下文
chat_history = [
    {
        "role": "user",
        "content": "（递给你一盒亲手做的巧克力）这是给你的，虽然做得不太好，但不许拒绝！",
    }
]

# 3. 定义期望的输出格式 (Schema)
expected_format = {
    "intent": "gift_giving",
    "emotion": "shy_but_forceful",
    "affinity_delta": 5,
    "reason": "用户通过实际行动表达关心，虽然语气强硬但属于典型的傲娇行为。",
}

# 4. 调用
# 建议使用本地较强的模型来生成训练集，比如 qwen2:7b 或更高级别的
result = generate_astrarp_data(
    base_url="http://localhost:11434",
    model="qwen2.5:7B",
    system_prompt=task_system_prompt,
    history=chat_history,
    output_schema=expected_format,
)

print(json.dumps(result, ensure_ascii=False, indent=4))
