import requests


def debug_call(base_url: str, model: str):
    url = f"{base_url}/api/chat"
    payload = {
        "model": model,
        "messages": [{"role": "user", "content": "hi"}],
        "stream": False,
    }
    # 显式禁用代理
    proxies = {"http": None, "https": None}
    response = requests.post(url, json=payload, proxies=proxies)
    print(response.status_code)
    print(response.text)


debug_call("http://localhost:11434", "qwen2.5:7B")
