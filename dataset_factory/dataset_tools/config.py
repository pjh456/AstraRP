from dataclasses import dataclass
import os


@dataclass(frozen=True)
class OllamaConfig:
    base_url: str
    model: str
    temperature: float = 0.7


def _require(name: str) -> str:
    value = os.getenv(name)
    if not value:
        raise RuntimeError(f"缺少配置项: {name}（未在 .env 或环境变量中找到）")
    return value


def load_ollama_config() -> OllamaConfig:
    return OllamaConfig(
        base_url=_require("OLLAMA_BASE_URL"),
        model=_require("OLLAMA_MODEL"),
        temperature=float(os.getenv("OLLAMA_TEMPERATURE", "0.7")),
    )
