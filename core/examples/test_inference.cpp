#include <benchmark/benchmark.h>

#define ASTRA_IGNORE_LOG

#include "core/model_manager.hpp"
#include "core/context_params.hpp"
#include "core/sampler_chain.hpp"
#include "core/tokenizer.hpp"
#include "infer/session.hpp"
#include "infer/tokenize_task.hpp"
#include "infer/decode_task.hpp"
#include "infer/sample_task.hpp"

using namespace astra_rp;
using namespace astra_rp::core;
using namespace astra_rp::infer;

// 定义基准测试装置
class InferenceFixture : public benchmark::Fixture
{
public:
    MulPtr<Model> model;
    MulPtr<Session> session;
    ContextParams ctx_params;

    void SetUp(const ::benchmark::State &state)
    {
        // 模型加载只需要做一次
        const char *env_dir = std::getenv("LLAMA_MODEL_DIR");
        if (!env_dir)
            return;

        auto &mm = ModelManager::instance();
        model = mm.load(env_dir, ModelParamsBuilder("bench_model").build()).unwrap();

        ctx_params = ContextParamsBuilder().context_size(2048).batch_size(512).build();
        auto sampler = SamplerChainBuilder().greedy().build().unwrap();
        session = Session::create(model, ctx_params, std::move(sampler), 0).unwrap();
    }
};

// 测量核心循环
BENCHMARK_F(InferenceFixture, StreamingInference)(benchmark::State &state)
{
    Str prompt = "User: Hello, count from 1 to 5.\nAssistant: ";
    int max_tokens = 50;

    for (auto _ : state)
    {
        // --- 循环内的性能关键点 ---
        session->clear(); // 关键：每次迭代必须清理 KV Cache

        TokenizeTask tok_task(model, prompt);
        tok_task.execute();
        Vec<Token> pending_tokens = tok_task.tokens();

        for (int i = 0; i < max_tokens; ++i)
        {
            DecodeTask dec_task(session, pending_tokens);
            dec_task.execute();
            pending_tokens.clear();

            SampleTask samp_task(session);
            samp_task.execute();
            Token next = samp_task.token();

            benchmark::DoNotOptimize(next); // 防止编译器优化掉采样结果

            if (llama_vocab_is_eog(llama_model_get_vocab(model->raw()), next))
                break;
            pending_tokens.push_back(next);
        }
        // -------------------------
    }
}

BENCHMARK_MAIN();