// --- START OF FILE test_inference.cpp ---

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdlib>

#define ASTRA_SIMPLE_LOG

#include "core/model_manager.hpp"
#include "core/context_params.hpp"
#include "core/sampler_chain.hpp"
#include "core/sampler_params.hpp"
#include "core/tokenizer.hpp"
#include "infer/session.hpp"
#include "infer/engine.hpp"

// 引入新的微任务
#include "infer/tokenize_task.hpp"
#include "infer/decode_task.hpp"
#include "infer/sample_task.hpp"

#include "utils/logger.hpp"

using namespace astra_rp;
using namespace astra_rp::core;
using namespace astra_rp::infer;

// ---------------------------------------------------------
// 辅助函数：手动组装微任务，驱动自回归生成
// 这等同于原先 TaskBuilder 在后台做的事情
// ---------------------------------------------------------
Str run_inference_loop(MulPtr<Session> session, const Str &prompt, int max_tokens)
{
    Str output_buffer;
    int generated_count = 0;
    auto vocab = llama_model_get_vocab(session->model()->raw());

    // 1. Tokenize (Prefill 阶段前置)
    TokenizeTask tok_task(session->model(), prompt);
    auto tok_res = tok_task.execute();
    assert(tok_res.is_ok());
    Vec<Token> pending_tokens = tok_task.tokens();

    // 2. 自回归循环
    while (generated_count < max_tokens)
    {
        // 检查 Context 容量
        if (session->n_past() + pending_tokens.size() > session->context()->capacity())
        {
            ASTRA_LOG_WARN("Context capacity reached. Stopping.");
            break;
        }

        // A. 解码
        DecodeTask dec_task(session, pending_tokens);
        auto dec_res = dec_task.execute();
        assert(dec_res.is_ok());
        pending_tokens.clear();

        // B. 采样
        SampleTask samp_task(session);
        auto samp_res = samp_task.execute();
        assert(samp_res.is_ok());
        Token next_token = samp_task.token();

        // C. 解码为字符串并流式输出
        auto str_res = Tokenizer::detokenize(session->model(), {next_token});
        if (str_res.is_ok())
        {
            Str text = str_res.unwrap();
            std::cout << text << std::flush; // 流式打印
            output_buffer += text;
        }

        // D. 检查结束符
        if (llama_vocab_is_eog(vocab, next_token))
        {
            break;
        }

        // 加入下一轮
        pending_tokens.push_back(next_token);
        generated_count++;
    }

    std::cout << std::endl; // 结束时换行
    return output_buffer;
}

// ---------------------------------------------------------
// 测试 1: 流式输出推断 (基于同步微任务链)
// ---------------------------------------------------------
void test_streaming_inference(MulPtr<Model> model)
{
    ASTRA_LOG_INFO("--- Test 1: Streaming Inference (Micro-Tasks) ---");

    auto ctx_params =
        ContextParamsBuilder()
            .context_size(2048)
            .batch_size(512)
            .build();

    auto sampler_res = SamplerChainBuilder().greedy().build();
    assert(sampler_res.is_ok());
    auto sampler = std::move(sampler_res.unwrap());

    auto session_res = Session::create(model, ctx_params, std::move(sampler), 0);
    assert(session_res.is_ok());
    auto session = std::move(session_res.unwrap());

    Str prompt = "User: Hello, count from 1 to 5.\nAssistant: ";
    ASTRA_LOG_INFO("Feeding Prompt: " + prompt);

    // 运行我们的微任务循环
    run_inference_loop(session, prompt, 50);

    // 验证状态突变
    assert(session->n_past() > 0);
    ASTRA_LOG_DEBUG("Streaming generation finished. Session n_past: " + std::to_string(session->n_past()));
}

// ---------------------------------------------------------
// 测试 2: KV Cache清理与多轮会话复用
// ---------------------------------------------------------
void test_session_clear_and_generate(MulPtr<Model> model)
{
    ASTRA_LOG_INFO("--- Test 2: Session Clear & Full Generate (Micro-Tasks) ---");

    auto ctx_params = ContextParamsBuilder().context_size(2048).batch_size(512).build();
    auto sampler_res = SamplerChainBuilder().top_k(40).top_p(0.9f, 1).temperature(0.7f).seed(1337).build();
    assert(sampler_res.is_ok());
    auto sampler = std::move(sampler_res.unwrap());

    auto session_res = Session::create(model, ctx_params, std::move(sampler), 0);
    assert(session_res.is_ok());
    auto session = std::move(session_res.unwrap());

    // [第 1 轮]
    Str prompt1 = "User: What is the capital of France?\nAssistant: ";
    Str response1 = run_inference_loop(session, prompt1, 20);
    ASTRA_LOG_INFO("Round 1 Output: " + response1);

    //[清理会话] - 抹除历史记忆，避免 KV Cache 污染
    session->clear();
    assert(session->n_past() == 0); // 确认进度重置
    ASTRA_LOG_INFO("Session cleared.");

    // [第 2 轮]
    Str prompt2 = "User: What is the capital of Japan?\nAssistant: ";
    Str response2 = run_inference_loop(session, prompt2, 20);
    ASTRA_LOG_INFO("Round 2 Output: " + response2);

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

    auto params = ModelParamsBuilder(name).use_mmap(true).build();
    auto &model_manager = ModelManager::instance();
    auto model_res = model_manager.load(path, params);

    if (model_res.is_err())
    {
        ASTRA_LOG_ERROR("Failed to load model: " + model_res.unwrap_err().to_string());
        return 1;
    }

    auto model = model_res.unwrap();

    // 注意：删除了 Engine::instance().start() / stop()
    // 因为现在的 Engine 是无状态同步调用工具，不需要后台线程！

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