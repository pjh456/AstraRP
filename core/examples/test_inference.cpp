#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdlib>

#define ASTRA_SIMPLE_LOG

#include "core/model_manager.hpp"
#include "core/context_params.hpp"
#include "core/sampler.hpp"
#include "core/sampler_params.hpp"
#include "core/tokenizer.hpp"
#include "infer/session.hpp"
#include "utils/logger.hpp"

using namespace astra_rp;
using namespace astra_rp::core;
using namespace astra_rp::infer;

// ---------------------------------------------------------
// 测试 1: 流式输出推断 (Streaming Inference)
// ---------------------------------------------------------
void test_streaming_inference(MulPtr<Model> model)
{
    ASTRA_LOG_INFO("--- Test 1: Streaming Inference ---");

    // 1. 初始化参数
    auto ctx_params =
        ContextParamsBuilder()
            .context_size(2048)
            .batch_size(2048)
            .threads(4)
            .threads_batch(4)
            .flash_attention(true)
            .build();
    auto sampler_res = SamplerBuilder().greedy().build(); // 使用贪婪搜索，保证结果确定性
    assert(sampler_res.is_ok());
    auto sampler = sampler_res.unwrap();

    // 2. 建立会话
    auto session_res = Session::create(model, ctx_params, std::move(sampler), 0);
    assert(session_res.is_ok());
    auto session = session_res.unwrap();

    // 3. 喂入 Prompt
    Str prompt = "User: Hello, count from 1 to 5.\nAssistant: ";
    ASTRA_LOG_INFO("Feeding Prompt: " + prompt);
    auto feed_res = session->feed_prompt(prompt);
    assert(feed_res.is_ok());

    // 4. 流式生成
    ASTRA_LOG_INFO("Streaming Response: ");
    int generated_count = 0;

    // 终端可能会缓冲不完整的 UTF-8 字符（如中文），C++标准输出刷新可解决大部分问题
    while (!session->is_finished() && generated_count < 50)
    {
        auto token_res = session->generate_next();
        assert(token_res.is_ok());
        auto t = token_res.unwrap();
        if (session->is_finished())
            break;

        auto piece_res = Tokenizer::detokenize(model, {t}, false, false);
        assert(piece_res.is_ok());
        auto piece = piece_res.unwrap();
        std::cout << piece << std::flush;

        generated_count++;
    }
    std::cout << std::endl;

    assert(session->get_n_past() > 0);
    ASTRA_LOG_DEBUG("Streaming generation finished. Tokens: " + std::to_string(generated_count));
}

// ---------------------------------------------------------
// 测试 2: KV Cache清理与多轮会话复用
// ---------------------------------------------------------
void test_session_clear_and_generate(MulPtr<Model> model)
{
    ASTRA_LOG_INFO("--- Test 2: Session Clear & Full Generate ---");

    auto ctx_params =
        ContextParamsBuilder()
            .context_size(2048)
            .batch_size(2048)
            .threads(4)
            .threads_batch(4)
            .flash_attention(true)
            .build();
    auto sampler_res =
        SamplerBuilder()
            .top_k(40)
            .top_p(0.9f, 1)
            .temperature(0.7f)
            .seed(1337) // <--- 加入随机数种子，它是真正的分布采样器
            .build();
    assert(sampler_res.is_ok());
    auto sampler = sampler_res.unwrap();

    auto session_res = Session::create(model, ctx_params, std::move(sampler), 0);
    assert(session_res.is_ok());
    auto session = session_res.unwrap();

    // [第 1 轮]
    Str prompt1 = "User: What is the capital of France?\nAssistant: ";
    auto feed_res1 = session->feed_prompt(prompt1);
    assert(feed_res1.is_ok());

    auto response1_res = session->generate(20); // 限制生成20个Token
    assert(response1_res.is_ok());
    auto response1 = response1_res.unwrap();
    ASTRA_LOG_INFO("Round 1 Output: " + response1);

    // [清理会话] - 抹除历史记忆，避免 KV Cache OOM
    session->clear();
    assert(session->get_n_past() == 0);
    assert(!session->is_finished());
    ASTRA_LOG_INFO("Session cleared.");

    //[第 2 轮]
    Str prompt2 = "User: What is the capital of Japan?\nAssistant: ";
    auto feed_res2 = session->feed_prompt(prompt2);
    assert(feed_res2.is_ok());

    auto response2_res = session->generate(20);
    assert(response2_res.is_ok());
    auto response2 = response2_res.unwrap();
    ASTRA_LOG_INFO("Round 2 Output: " + response2);

    // 验证第二轮不受第一轮影响
    assert(response2.length() > 0);
    ASTRA_LOG_DEBUG("Session clear and multi-turn generation verified.");
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main()
{
    std::cout << "========== STARTING INFERENCE TESTS ==========" << std::endl;

    const char *env_dir = std::getenv("LLAMA_MODEL_DIR");
    if (!env_dir)
    {
        std::cerr << "[SKIP] LLAMA_MODEL_DIR not set. Cannot run inference tests." << std::endl;
        return 0;
    }

    Str path(env_dir);
    Str name("test_model_infer");

    std::cout << "Loading model from: " << path << std::endl;

    // 为了推断速度，可以指定 GPU 层数 (如果编译了 CUDA 支持)
    auto params = ModelParamsBuilder(name)
                      .use_mmap(true)
                      .gpu_layers(0)
                      .build();

    auto &model_manager = ModelManager::instance();
    auto model_res = model_manager.load(path, params);
    assert(model_res.is_ok());
    auto model = model_res.unwrap();

    if (!model)
    {
        ASTRA_LOG_ERROR("Failed to load model.");
        return 1;
    }

    try
    {
        test_streaming_inference(model);
        test_session_clear_and_generate(model);
    }
    catch (const std::exception &e)
    {
        ASTRA_LOG_ERROR(std::string("Uncaught exception: ") + e.what());
        model_manager.unload(name);
        return 1;
    }

    model_manager.unload(name);
    std::cout << "========== ALL INFERENCE TESTS PASSED ==========" << std::endl;

    return 0;
}