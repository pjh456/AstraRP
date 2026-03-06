#include <iostream>
#include <vector>
#include <cassert>
#include <thread>
#include <atomic>
#include <string>

#define ASTRA_SIMPLE_LOG

#include "core/context_manager.hpp"
#include "core/model_manager.hpp"
#include "core/model.hpp"
#include "core/context_params.hpp"
#include "utils/logger.hpp"

using namespace astra_rp;
using namespace astra_rp::core;

// ---------------------------------------------------------
// 测试 1: 基础生命周期与属性验证
// ---------------------------------------------------------
void test_context_lifecycle(MulPtr<Model> model)
{
    auto &manager = ContextManager::instance();

    // 显式指定 n_ctx = 256
    auto params = ContextParamsBuilder()
                      .context_size(256)
                      .batch_size(128)
                      .build();

    auto ctx = manager.acquire(model, params);

    assert(ctx != nullptr);
    assert(ctx->raw() != nullptr);
    assert(ctx->capacity() >= 256); // llama.cpp 可能会对齐，所以是 >=

    // 验证 model 引用保持有效
    assert(ctx->model().name() == model->name());

    ASTRA_LOG_INFO("Context capacity verified: " + std::to_string(ctx->capacity()));
    ASTRA_LOG_DEBUG("Test 1: Basic Lifecycle & Properties");
}

// ---------------------------------------------------------
// 测试 2: 对象池复用逻辑
// ---------------------------------------------------------
void test_pool_reuse_logic(MulPtr<Model> model)
{
    auto &manager = ContextManager::instance();
    auto params = ContextParamsBuilder().context_size(512).build();

    llama_context *raw_ptr1 = nullptr;

    // 作用域 1: 申请并归还
    {
        auto ctx1 = manager.acquire(model, params);
        raw_ptr1 = ctx1->raw();
    }

    // 作用域 2: 再次申请，规格相同，必须复用
    {
        auto ctx2 = manager.acquire(model, params);
        assert(ctx2->raw() == raw_ptr1);
        ASTRA_LOG_INFO("Context reused successfully.");
    }

    // 作用域 3: 申请更高规格的 Context (1024)，无法复用池子中 512 的
    {
        auto larger_params = ContextParamsBuilder().context_size(1024).build();
        auto ctx3 = manager.acquire(model, larger_params);

        // 由于容量要求变大，应该分配了一个全新的 context (前提是刚才 512 的没有被对齐到>=1024)
        if (ctx3->capacity() > 512)
        {
            assert(ctx3->raw() != raw_ptr1);
            ASTRA_LOG_INFO("New Context created for larger capacity requirement.");
        }
    }

    ASTRA_LOG_DEBUG("Test 2: Pool Reuse & Capacity Matching");
}

// ---------------------------------------------------------
// 测试 3: 状态 (State) 获取与覆盖
// ---------------------------------------------------------
void test_state_management(MulPtr<Model> model)
{
    auto &manager = ContextManager::instance();
    auto params = ContextParamsBuilder().context_size(128).build();

    auto ctx = manager.acquire(model, params);

    // 1. 获取状态大小
    size_t state_size = ctx->state_size(0);
    assert(state_size > 0);
    ASTRA_LOG_INFO("State size is " + std::to_string(state_size) + " bytes.");

    // 2. 导出状态
    auto state_data = ctx->get_state(0);
    assert(state_data.size() == state_size);

    // 3. 覆盖状态
    size_t bytes_written = ctx->set_state(state_data, 0);
    assert(bytes_written == state_size);

    ASTRA_LOG_DEBUG("Test 3: State Management (Save/Load)");
}

// ---------------------------------------------------------
// 测试 4: 多线程并发安全 (Thread Safety)
// ---------------------------------------------------------
void test_concurrency(MulPtr<Model> model)
{
    auto &manager = ContextManager::instance();
    const int num_threads = 8;
    const int loops_per_thread = 10;

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    auto thread_func = [&]()
    {
        for (int i = 0; i < loops_per_thread; ++i)
        {
            // 混合申请不同容量的 Context
            uint32_t req_size = (i % 2 == 0) ? 128 : 256;
            auto params = ContextParamsBuilder().context_size(req_size).build();

            auto ctx = manager.acquire(model, params);
            assert(ctx->capacity() >= req_size);

            // 简单验证清理函数不会崩溃
            ctx->clear(true);

            // 模拟耗时，打乱线程执行顺序
            std::this_thread::yield();
        }
        success_count++;
    };

    ASTRA_LOG_INFO("Launching 8 threads, each acquiring/releasing 10 times...");

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_func);
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }

    assert(success_count == num_threads);
    ASTRA_LOG_DEBUG("Test 4: Concurrency & Thread Safety");
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main()
{
    std::cout << "========== STARTING CONTEXT TESTS ==========" << std::endl;

    // 1. 初始化环境变量与模型
    const char *env_dir = std::getenv("LLAMA_MODEL_DIR");
    if (!env_dir)
    {
        std::cerr << "[SKIP] LLAMA_MODEL_DIR environment variable not set. Tests cannot run without a model." << std::endl;
        return 0;
    }

    astra_rp::Str path(env_dir);
    astra_rp::Str name("test_model_for_context");

    std::cout << "Loading model from: " << path << std::endl;

    auto &model_manager = ModelManager::instance();
    auto model_params = ModelParamsBuilder(name).use_mmap(true).build();

    auto model = model_manager.load(path, model_params);
    if (!model || !model->raw())
    {
        std::cerr << "[FATAL] Failed to load model." << std::endl;
        return 1;
    }

    // 2. 执行测试套件
    try
    {
        test_context_lifecycle(model);
        test_pool_reuse_logic(model);
        test_state_management(model);
        test_concurrency(model);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] Uncaught exception in test suite: " << e.what() << std::endl;
        model_manager.unload(name);
        return 1;
    }

    // 3. 卸载模型清理资源
    model_manager.unload(name);

    std::cout << "========== ALL TESTS PASSED ==========" << std::endl;
    return 0;
}