#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <thread>
#include <future>
#include <atomic>

#define ASTRA_SIMPLE_LOG

#include "core/batch.hpp"
#include "core/batch_manager.hpp"

#include "utils/logger.hpp"

using namespace astra_rp::core;

// ---------------------------------------------------------
// 测试 1: Batch 类的移动语义 (Move Semantics)
// 验证手动实现的移动构造和赋值是否防止了 Double Free
// ---------------------------------------------------------
// void test_batch_move_semantics()
// {
//     {
//         // 测试移动构造
//         Batch b1(512, 1);
//         // 模拟填充一些数据，确保资源被分配
//         b1.add(101, 0, {0}, false);
//         void *original_token_ptr = b1.raw().token;

//         // 执行移动构造
//         Batch b2(std::move(b1));

//         // 验证源对象 (b1) 资源已悬空
//         assert(b1.raw().token == nullptr);
//         assert(b1.raw().n_tokens == 0);

//         // 验证目标对象 (b2) 接管资源
//         assert(b2.raw().token == original_token_ptr);
//         assert(b2.size() == 1);
//         assert(b2.raw().token[0] == 101);

//         // b1 析构时不会 crash (因为 token 为 nullptr)
//         // b2 析构时会释放 original_token_ptr
//     }
//     ASTRA_LOG_INFO("Move Constructor verified.");

//     {
//         // 测试移动赋值
//         Batch b3(512, 1);
//         b3.add(202, 0, {0}, false);
//         void *ptr_b3 = b3.raw().token;

//         Batch b4(256, 1); // b4 原本有自己的资源
//         void *ptr_b4_old = b4.raw().token;

//         // 执行移动赋值
//         b4 = std::move(b3);

//         // 验证 b3 被置空
//         assert(b3.raw().token == nullptr);

//         // 验证 b4 释放了旧资源 (无法直接检测 free，但可以检测新指针)
//         assert(b4.raw().token == ptr_b3);
//         assert(b4.raw().token != ptr_b4_old);
//         assert(b4.size() == 1);
//         assert(b4.raw().token[0] == 202);

//         // 自我赋值测试 (边缘情况)
//         Batch *b4_ptr = &b4;
//         (*b4_ptr) = std::move(b4);
//         assert(b4.raw().token == ptr_b3); // 应该保持不变
//     }
//     ASTRA_LOG_INFO("Move Assignment verified.");

//     ASTRA_LOG_DEBUG("Test 1: Batch Move Semantics & RAII");
// }

// ---------------------------------------------------------
// 测试 2: 复杂序列数据完整性
// ---------------------------------------------------------
void test_data_integrity()
{
    auto &manager = BatchManager::instance();
    auto batch_res = manager.acquire(128, 4); // 需要支持每个 token 属于 4 个序列
    assert(batch_res.is_ok());
    auto batch = batch_res.unwrap();

    std::vector<int32_t> seq_ids = {10, 20, 30};
    batch->add(999, 5, seq_ids, true);

    llama_batch raw = batch->raw();

    // 检查基础数据
    assert(raw.n_tokens == 1);
    assert(raw.token[0] == 999);
    assert(raw.pos[0] == 5);
    assert(raw.logits[0] == true);

    // 检查序列数据
    assert(raw.n_seq_id[0] == 3);
    assert(raw.seq_id[0][0] == 10);
    assert(raw.seq_id[0][1] == 20);
    assert(raw.seq_id[0][2] == 30);

    ASTRA_LOG_DEBUG("Test 2: Data Integrity & Multi-Sequence support");
}

// ---------------------------------------------------------
// 测试 3: BatchManager 对象池复用逻辑
// ---------------------------------------------------------
void test_pool_reuse_logic()
{
    auto &manager = BatchManager::instance();

    void *addr1 = nullptr;

    // 作用域 1
    {
        auto b1_res = manager.acquire(512);
        assert(b1_res.is_ok());
        auto b1 = b1_res.unwrap();

        addr1 = (void *)b1.get();
        b1->add(1, 0, {0}, false);
        assert(b1->size() == 1);
        // b1 离开作用域，归还给池子
    }

    // 作用域 2：申请相同规格，应该复用
    {
        auto b2_res = manager.acquire(512);
        assert(b2_res.is_ok());
        auto b2 = b2_res.unwrap();

        void *addr2 = (void *)b2.get();

        assert(addr1 == addr2);  // 地址必须相同
        assert(b2->size() == 0); // 必须被 clear 干净
    }

    // 作用域 3：申请更大规格，不应复用小的
    {
        auto b3_res = manager.acquire(1024); // 此时池子里有一个 512 的
        assert(b3_res.is_ok());
        auto b3 = b3_res.unwrap();

        void *addr3 = (void *)b3.get();

        assert(addr1 != addr3); // 应该是新对象
        assert(b3->capacity() >= 1024);
    }

    ASTRA_LOG_DEBUG("Test 3: Pool Reuse & Capacity Matching");
}

// ---------------------------------------------------------
// 测试 4: 边界检查与异常
// ---------------------------------------------------------
void test_boundaries()
{
    auto &manager = BatchManager::instance();
    auto batch_res = manager.acquire(1, 1);
    assert(batch_res.is_ok());
    auto batch = batch_res.unwrap();

    // 1. 测试 Token 容量溢出
    try
    {
        for (int i = 0; i <= batch->capacity(); ++i)
            batch->add(i, i, {0}, false);
        assert(false);
    }
    catch (const std::runtime_error &e)
    {
        assert(std::string(e.what()).find("Batch token capacity exceeded") != std::string::npos);
        ASTRA_LOG_INFO("Caught expected token overflow exception.");
    }

    batch->clear();

    // 2. 测试 Seq 容量溢出
    try
    {
        std::vector<int> buf;
        buf.reserve(batch->max_seqs());
        for (int i = 0; i <= batch->max_seqs(); ++i)
            buf.push_back(i + 1);

        batch->add(1, 1, buf, false);
        assert(false);
    }
    catch (const std::runtime_error &e)
    {
        assert(std::string(e.what()).find("Batch sequence capacity exceeded") != std::string::npos);
        ASTRA_LOG_INFO("Caught expected sequence overflow exception.");
    }

    ASTRA_LOG_DEBUG("Test 4: Boundary Checks");
}

// ---------------------------------------------------------
// 测试 5: 多线程并发测试 (Thread Safety)
// ---------------------------------------------------------
void test_concurrency()
{
    auto &manager = BatchManager::instance();
    const int num_threads = 10;
    const int loops_per_thread = 50;

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    auto thread_func = [&](int id)
    {
        for (int i = 0; i < loops_per_thread; ++i)
        {
            // 随机申请不同大小，增加池管理的复杂性
            int size = (i % 2 == 0) ? 512 : 1024;

            auto batch_res = manager.acquire(size);
            assert(batch_res.is_ok());
            auto batch = batch_res.unwrap();

            // 简单操作
            batch->add(id, i, {0}, false);

            // 模拟一点耗时
            std::this_thread::yield();

            // batch 离开作用域，自动 release (涉及 mutex lock)
        }
        success_count++;
    };

    ASTRA_LOG_INFO("Launching 10 threads, each acquiring/releasing 50 times...");

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_func, i);
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }

    assert(success_count == num_threads);
    ASTRA_LOG_DEBUG("Test 5: Concurrency & Thread Safety");
}

int main()
{
    std::cout << "========== STARTING UNIT TESTS ==========" << std::endl;

    try
    {
        // test_batch_move_semantics();
        test_data_integrity();
        test_pool_reuse_logic();
        test_boundaries();
        test_concurrency();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] Uncaught exception in test suite: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "========== ALL TESTS PASSED ==========" << std::endl;
    return 0;
}