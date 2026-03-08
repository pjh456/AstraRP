#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdlib>
#include <future>

#define ASTRA_SIMPLE_LOG

#include "core/model_manager.hpp"
#include "core/context_params.hpp"
#include "core/sampler_chain.hpp"
#include "core/sampler_params.hpp"
#include "core/tokenizer.hpp"
#include "infer/session.hpp"
#include "infer/task.hpp"
#include "infer/engine.hpp"
#include "utils/logger.hpp"

using namespace astra_rp;
using namespace astra_rp::core;
using namespace astra_rp::infer;

// ---------------------------------------------------------
// 测试 1: 流式输出推断 (基于异步 Task 调度)
// ---------------------------------------------------------
void test_streaming_inference(MulPtr<Model> model)
{
    ASTRA_LOG_INFO("--- Test 1: Streaming Inference (Async Engine) ---");

    // 1. 初始化会话与参数 (纯状态)
    auto ctx_params =
        ContextParamsBuilder()
            .context_size(2048)
            .batch_size(512) // n_batch: Engine 会据此动态分块处理长文本
            .build();
    auto sampler_res = SamplerChainBuilder().greedy().build();
    assert(sampler_res.is_ok());
    auto sampler = std::move(sampler_res.unwrap());

    auto session_res = Session::create(model, ctx_params, std::move(sampler), 0);
    assert(session_res.is_ok());
    auto session = std::move(session_res.unwrap());

    // 2. 准备 Prompt 并将其转换为 Token
    Str prompt = "User: Hello, count from 1 to 5.\nAssistant: ";
    ASTRA_LOG_INFO("Feeding Prompt: " + prompt);
    auto prompt_tokens = Tokenizer::tokenize(model, prompt, true, true).unwrap();

    // 3. 构建推理任务 (Task)
    int generated_count = 0;

    auto task_res =
        TaskBuilder(session)
            .prompt(prompt)
            .max_tokens(50) // 手动限制一下，不然它会自动跑到 Context 上限
            .on_text_generated(
                [&](const Str &text)
                {
                    std::cout << text << std::flush;
                    generated_count++;
                    return true;
                })
            .on_finish(
                []()
                {
                    std::cout << std::endl; // 结束时换行
                })
            .on_error(
                [](const utils::Error &err)
                {
                    ASTRA_LOG_ERROR("Task failed during execution: " + err.to_string());
                })
            .build();

    assert(task_res.is_ok());

    auto task = task_res.unwrap();
    // 5. 将任务提交给后台 Engine (非阻塞)
    Engine::instance().submit(task);

    // 6. 主线程挂起等待，直到 Engine 后台运算完成并 set_value
    task->wait();

    // 7. 验证状态突变
    assert(task->state() == TaskState::FINISHED);
    assert(session->n_past() > 0); // 确认 Session 里的 KV Cache 进度已经被 Engine 推进

    ASTRA_LOG_DEBUG("Streaming generation finished. Tokens generated: " + std::to_string(generated_count));
}

// ---------------------------------------------------------
// 测试 2: KV Cache清理与多轮会话复用
// ---------------------------------------------------------
void test_session_clear_and_generate(MulPtr<Model> model)
{
    ASTRA_LOG_INFO("--- Test 2: Session Clear & Full Generate (Async Engine) ---");

    auto ctx_params = ContextParamsBuilder().context_size(2048).batch_size(512).build();

    auto sampler_res = SamplerChainBuilder().top_k(40).top_p(0.9f, 1).temperature(0.7f).seed(1337).build();
    assert(sampler_res.is_ok());
    auto sampler = sampler_res.unwrap();

    auto session_res = Session::create(model, ctx_params, std::move(sampler), 0);
    assert(session_res.is_ok());
    auto session = session_res.unwrap();

    // 封装一个简单的同步生成辅助函数，展示如何复用 Task 逻辑
    auto run_prompt = [&](const Str &prompt) -> Str
    {
        Str output_buffer;

        auto task_res =
            TaskBuilder(session)
                .prompt(prompt)
                .max_tokens(20)
                .on_text_generated(
                    [&](auto str) -> bool
                    {
                        output_buffer += str;
                        return true;
                    })
                .on_error(
                    [](auto err)
                    {
                        ASTRA_LOG_ERROR("Task failed: " + err.to_string());
                    })
                .build();
        assert(task_res.is_ok());
        auto task = task_res.unwrap();

        // 提交并等待
        Engine::instance().submit(task);
        task->wait();

        return output_buffer;
    };

    // [第 1 轮]
    Str prompt1 = "User: What is the capital of France?\nAssistant: ";
    Str response1 = run_prompt(prompt1);
    ASTRA_LOG_INFO("Round 1 Output: " + response1);

    // [清理会话] - 抹除历史记忆，避免 KV Cache 污染
    session->clear();
    assert(session->n_past() == 0); // 确认进度重置
    ASTRA_LOG_INFO("Session cleared.");

    //[第 2 轮]
    Str prompt2 = "User: What is the capital of Japan?\nAssistant: ";
    Str response2 = run_prompt(prompt2);
    ASTRA_LOG_INFO("Round 2 Output: " + response2);

    // 验证第二轮能够正常输出且没发生引擎错误
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

    // 在跑任何推理之前，必须唤醒全局 Engine 守护线程
    Engine::instance().start();

    try
    {
        test_streaming_inference(model);
        test_session_clear_and_generate(model);
    }
    catch (const std::exception &e)
    {
        ASTRA_LOG_ERROR(std::string("Uncaught exception: ") + e.what());
        Engine::instance().stop();
        model_manager.unload(name);
        return 1;
    }

    // 【重要改动】测试结束后，关闭后台 Engine 线程
    Engine::instance().stop();
    model_manager.unload(name);

    std::cout << "========== ALL INFERENCE TESTS PASSED ==========" << std::endl;

    return 0;
}