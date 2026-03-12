#include <iostream>
#include <string>
#include <cassert>

#define ASTRA_SIMPLE_LOG

#include "core/model_manager.hpp"
#include "core/context_params.hpp"
#include "core/sampler_chain.hpp"
#include "core/tokenizer.hpp"
#include "infer/session.hpp"
#include "infer/tokenize_task.hpp"
#include "infer/decode_task.hpp"
#include "infer/sample_task.hpp"
#include "utils/logger.hpp"

using namespace astra_rp;
using namespace astra_rp::core;
using namespace astra_rp::infer;

void test_basic_inference()
{
    // 模型加载只需要做一次，未设置环境变量直接跳过即可
    const char *env_dir = std::getenv("LLAMA_MODEL_DIR");
    if (!env_dir)
    {
        ASTRA_LOG_INFO("LLAMA_MODEL_DIR not set, skipping inference test.");
        return;
    }

    // 1. 加载模型
    auto &mm = ModelManager::instance();
    auto model_res = mm.load(env_dir, ModelParamsBuilder("test_model").build());
    assert(model_res.is_ok());
    auto model = model_res.unwrap();

    // 2. 初始化上下文参数与采样器
    auto ctx_params = ContextParamsBuilder().context_size(2048).batch_size(512).build();

    auto sampler_res = SamplerChainBuilder().greedy().build();
    assert(sampler_res.is_ok());
    auto sampler = std::move(sampler_res.unwrap());

    // 3. 创建 Session
    auto session_res = Session::create(model, ctx_params, std::move(sampler), 0);
    assert(session_res.is_ok());
    auto session = session_res.unwrap();

    // 4. 执行推理循环
    Str prompt = "User: Hello, count from 1 to 5.\nAssistant: ";
    int max_tokens = 50;
    int generated_count = 0;

    session->clear(); // 清理 KV Cache

    TokenizeTask tok_task(model, prompt);
    tok_task.execute();
    Vec<Token> pending_tokens = tok_task.tokens();

    assert(!pending_tokens.empty());

    for (int i = 0; i < max_tokens; ++i)
    {
        DecodeTask dec_task(session, pending_tokens);
        dec_task.execute();
        pending_tokens.clear();

        SampleTask samp_task(session);
        samp_task.execute();
        Token next = samp_task.token();

        if (llama_vocab_is_eog(llama_model_get_vocab(model->raw()), next))
            break;

        pending_tokens.push_back(next);
        generated_count++;
    }

    // 简单断言验证至少输出了 Token
    assert(generated_count > 0);
    ASTRA_LOG_INFO("Inference successfully generated " + std::to_string(generated_count) + " tokens.");
}

int main()
{
    std::cout << "========== STARTING INFERENCE TESTS ==========" << std::endl;
    try
    {
        test_basic_inference();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] Inference Test failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "========== ALL INFERENCE TESTS PASSED ==========" << std::endl;
    return 0;
}