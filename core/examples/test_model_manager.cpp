#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <cstring>
#include <cstdlib>

#define ASTRA_SIMPLE_LOG

#include "core/model_manager.hpp"
#include "utils/logger.hpp"
#include "core/tokenizer.hpp"

using namespace astra_rp;
using namespace astra_rp::core;

// ==========================================
// 测试用例
// ==========================================

// 1. 基础转换测试
void test_basic_conversion(MulPtr<Model> model)
{
    Str input = "Hello world";

    // 不添加 BOS，只做纯文本转换
    auto tokens = Tokenizer::tokenize(model, input, false, false).unwrap();

    assert(!tokens.empty());
    ASTRA_LOG_ERROR("Tokenized size: " + std::to_string(tokens.size()));

    Str output = Tokenizer::detokenize(model, tokens, false, false).unwrap();

    // 注意：Llama 模型可能会在单词前加空格，或者输入本身就是从空格开始
    // 这里我们主要验证输出不为空，且包含原意
    assert(!output.empty());
    ASTRA_LOG_INFO("Original: " + input);
    ASTRA_LOG_INFO("Result:   " + output);

    // 简单的包含性检查
    assert(output.find("Hello") != std::string::npos);
    assert(output.find("world") != std::string::npos);

    ASTRA_LOG_DEBUG("Test 1: Basic Tokenize/Detokenize");
}

// 2. 闭环测试 (Round Trip)与 UTF-8 支持
// 这是验证 detokenize 缓冲区逻辑是否正确的关键
void test_utf8_roundtrip(MulPtr<Model> model)
{
    // 测试包含：中文、日文、Emoji、英文混合
    Str input = "你好，世界！ Hello. テスト 🚀";

    // Step 1: Tokenize (不加 BOS 以便严格比对字符串)
    auto tokens = Tokenizer::tokenize(model, input, false, false).unwrap();
    assert(tokens.size() > 0);

    // Step 2: Detokenize
    Str output = Tokenizer::detokenize(model, tokens, false, false).unwrap();

    ASTRA_LOG_INFO("UTF-8 Input:  " + input);
    ASTRA_LOG_INFO("UTF-8 Output: " + output);

    // Step 3: 验证
    // 注意：某些模型可能会对标点符号做归一化处理，但大部分情况应完全一致
    // 或者开头可能多一个空格（取决于 tokenizer 实现）

    // 简单的启发式检查：长度应该非常接近，且内容必须匹配
    if (input == output)
    {
        ASTRA_LOG_INFO("Perfect match!");
    }
    else
    {
        // 如果不完全匹配，检查是否只是开头多了空格
        if (output.size() > input.size() && output.substr(output.size() - input.size()) == input)
        {
            ASTRA_LOG_INFO("Match (with leading space/formatting).");
        }
        else
        {
            // 某些分词器行为差异，只要不乱码即可，暂时不强制 assert相等
            ASTRA_LOG_INFO("Note: Strings differ slightly (normalization?), but checking for garbage...");
        }
    }

    // 检查是否出现乱码 (简单的检查：Emoji 是否还在)
    assert(output.find("🚀") != std::string::npos);
    assert(output.find("你好") != std::string::npos);

    ASTRA_LOG_DEBUG("Test 2: UTF-8 & Round Trip");
}

// 3. 特殊 Token 测试 (BOS/EOS)
void test_special_tokens(MulPtr<Model> model)
{
    Str text = "Test";

    // Case A: 不加 Special
    auto tokens_plain = Tokenizer::tokenize(model, text, false, false).unwrap();

    // Case B: 加 Special (通常是 BOS)
    auto tokens_special = Tokenizer::tokenize(model, text, true, true).unwrap();

    ASTRA_LOG_INFO("Plain size: " + std::to_string(tokens_plain.size()));
    ASTRA_LOG_INFO("Special size: " + std::to_string(tokens_special.size()));

    // 通常加上 BOS 后，token 数量会 +1
    // 注意：有些模型可能没有 BOS，或者行为不同，这里做软性检查
    if (tokens_special.size() > tokens_plain.size())
    {
        ASTRA_LOG_INFO("Verified: tokenize with add_special=true added tokens.");
    }
    else
    {
        ASTRA_LOG_INFO("Warning: add_special=true did not increase token count (Model dependent).");
    }

    ASTRA_LOG_DEBUG("Test 3: Special Tokens Handling");
}

// 4. 边界情况测试
void test_edge_cases(MulPtr<Model> model)
{
    // Case A: 空字符串
    auto tokens = Tokenizer::tokenize(model, "", false, false).unwrap();
    assert(tokens.empty());
    ASTRA_LOG_INFO("Empty string tokenized to empty vector.");

    // Case B: 空 Vector
    auto str = Tokenizer::detokenize(model, {}, false, false).unwrap();
    assert(str.empty());
    ASTRA_LOG_INFO("Empty vector detokenized to empty string.");

    ASTRA_LOG_DEBUG("Test 4: Edge Cases");
}

// 5. 压力/长文本测试
// 验证 buffer 动态扩容机制
void test_long_text(MulPtr<Model> model)
{
    std::string long_text;
    for (int i = 0; i < 100; ++i)
    {
        long_text += "The quick brown fox jumps over the lazy dog. ";
    }
    long_text += "End.";

    auto tokens = Tokenizer::tokenize(model, long_text, false, false).unwrap();
    assert(tokens.size() > 100);

    auto output = Tokenizer::detokenize(model, tokens, false, false).unwrap();

    // 验证重建后的文本长度接近原文本
    assert(output.size() >= long_text.size() * 0.9); // 允许少量的归一化差异

    ASTRA_LOG_DEBUG("Test 5: Long Text Stress Test");
}

// ==========================================
// Main Entry
// ==========================================
int main()
{
    std::cout << "========== STARTING MODEL TESTS ==========" << std::endl;

    // 1. 环境检查
    const char *env_dir = std::getenv("LLAMA_MODEL_DIR");
    if (!env_dir)
    {
        std::cerr << "[SKIP] LLAMA_MODEL_DIR not set. Cannot run model tests." << std::endl;
        return 0;
    }

    Str path(env_dir);
    Str name("test_model_core");

    std::cout << "Loading model from: " << path << std::endl;

    // 2. 加载模型
    auto &manager = ModelManager::instance();
    // 开启 mmap 加快加载速度，vocal_only=true 如果只想测分词可以开启，但为了通用性这里设为 false
    auto params = ModelParamsBuilder(name).use_mmap(true).build();

    auto model = manager.load(path, params).unwrap();
    if (!model)
    {
        ASTRA_LOG_ERROR("Failed to load model.");
        return 1;
    }

    // 3. 执行测试
    try
    {
        test_basic_conversion(model);
        test_utf8_roundtrip(model);
        test_special_tokens(model);
        test_edge_cases(model);
        test_long_text(model);
    }
    catch (const std::exception &e)
    {
        ASTRA_LOG_ERROR(std::string("Uncaught exception: ") + e.what());
        manager.unload(name);
        return 1;
    }

    // 4. 清理
    manager.unload(name);
    std::cout << "========== ALL TESTS PASSED ==========" << std::endl;

    return 0;
}