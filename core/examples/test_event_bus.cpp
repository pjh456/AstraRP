#include <iostream>
#include <vector>
#include <cassert>
#include <thread>
#include <atomic>
#include <string>

#define ASTRA_SIMPLE_LOG

#include "utils/logger.hpp"
#include "pipeline/event_bus.hpp"

using namespace astra_rp;
using namespace astra_rp::pipeline;

// ---------------------------------------------------------
// 测试 1: 基础单线程订阅与发布
// ---------------------------------------------------------
void test_basic_pubsub()
{
    auto bus = MulPtr<EventBus>(new EventBus());

    int token_calls = 0;
    int state_calls = 0;

    bus->subscribe_token([&](const Str &node_id, const Str &token)
                         {
        assert(node_id == "NodeA");
        assert(token == "Hello");
        token_calls++; });

    bus->subscribe_state([&](const Str &node_id, NodeState state)
                         {
        assert(node_id == "NodeB");
        assert(state == NodeState::RUNNING);
        state_calls++; });

    bus->publish_token("NodeA", "Hello");
    bus->publish_state("NodeB", NodeState::RUNNING);

    assert(token_calls == 1);
    assert(state_calls == 1);

    ASTRA_LOG_INFO("Basic Pub/Sub functionality verified.");
}

// ---------------------------------------------------------
// 测试 1.5: 模板化接口订阅与发布
// ---------------------------------------------------------
void test_template_pubsub()
{
    auto bus = MulPtr<EventBus>(new EventBus());

    int token_calls = 0;
    int state_calls = 0;
    int error_calls = 0;

    bus->subscribe<TokenEvent>(
        [&](const Str &node_id, const Str &token)
        {
            assert(node_id == "NodeC");
            assert(token == "Template Hello");
            token_calls++;
        });

    bus->subscribe<StateEvent>(
        [&](const Str &node_id, NodeState state)
        {
            assert(node_id == "NodeD");
            assert(state == NodeState::READY);
            state_calls++;
        });

    bus->subscribe<ErrorEvent>(
        [&](const Str &node_id, utils::Error err)
        {
            assert(node_id == "NodeE");
            assert(err.code == 42);
            error_calls++;
        });

    bus->publish(TokenEvent{"NodeC", "Template Hello"});
    bus->publish(StateEvent{"NodeD", NodeState::READY});
    bus->publish(ErrorEvent{
        "NodeE",
        utils::ErrorBuilder()
            .core()
            .message("template error")
            .build()});

    assert(token_calls == 1);
    assert(state_calls == 1);
    assert(error_calls == 1);

    ASTRA_LOG_INFO("Template Pub/Sub functionality verified.");
}

// ---------------------------------------------------------
// 测试 2: 多线程并发发布安全
// ---------------------------------------------------------
void test_thread_safety()
{
    auto bus = MulPtr<EventBus>(new EventBus());

    std::atomic<int> received_tokens{0};
    std::atomic<int> received_errors{0};

    // 注册多个监听器
    bus->subscribe_token([&](const Str &, const Str &)
                         { received_tokens++; });
    bus->subscribe_error([&](const Str &, utils::Error)
                         { received_errors++; });

    const int num_threads = 8;
    const int msgs_per_thread = 500;
    std::vector<std::thread> threads;

    // 并发轰炸 EventBus
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&, i]()
                             {
            for (int j = 0; j < msgs_per_thread; ++j)
            {
                if (j % 2 == 0)
                    bus->publish_token("Thread_" + std::to_string(i), "data");
                else
                    bus->publish_error("Thread_" + std::to_string(i), utils::Error());
            } });
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }

    // 验证所有消息都被安全接收，没有由于 Data Race 丢失
    assert(received_tokens == num_threads * (msgs_per_thread / 2));
    assert(received_errors == num_threads * (msgs_per_thread / 2));

    ASTRA_LOG_INFO("EventBus thread safety verified under high concurrency.");
}

int main()
{
    std::cout << "========== STARTING EVENT_BUS TESTS ==========" << std::endl;
    try
    {
        test_basic_pubsub();
        test_template_pubsub();
        test_thread_safety();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] EventBus Test failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "========== ALL EVENT_BUS TESTS PASSED ==========" << std::endl;
    return 0;
}