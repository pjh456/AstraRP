#include <iostream>
#include <atomic>

#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"
#include "pipeline/scheduler.hpp"
#include "utils/error.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class FailingNode final : public BaseNode
        {
        public:
            FailingNode(const Str &id, MulPtr<EventBus> bus = nullptr)
                : BaseNode(id, bus) {}

            ResultV<void> execute() override
            {
                return ResultV<void>::Err(
                    utils::ErrorBuilder()
                        .pipeline()
                        .node_execution_failed()
                        .message("intentional failure")
                        .build());
            }
        };

        class CountingNode final : public BaseNode
        {
        private:
            std::atomic<int> &m_counter;

        public:
            CountingNode(const Str &id, std::atomic<int> &counter, MulPtr<EventBus> bus = nullptr)
                : BaseNode(id, bus), m_counter(counter) {}

            ResultV<void> execute() override
            {
                m_counter.fetch_add(1);
                return ResultV<void>::Ok();
            }
        };
    }
}

int main()
{
    using namespace astra_rp;

    std::atomic<int> executed_count{0};

    auto graph = std::make_shared<pipeline::Graph>();
    graph->add_node(std::make_shared<pipeline::FailingNode>("A"));
    graph->add_node(std::make_shared<pipeline::CountingNode>("B", executed_count));
    graph->add_edge("A", "B");

    pipeline::Scheduler scheduler(graph, 1);
    auto res = scheduler.run();

    if (res.is_ok())
    {
        std::cerr << "Expected scheduler to fail, but it succeeded." << std::endl;
        return 1;
    }

    if (res.unwrap_err().code() != utils::ErrorCode::NodeExecutionFailed)
    {
        std::cerr << "Unexpected error code: " << static_cast<int>(res.unwrap_err().code()) << std::endl;
        return 1;
    }

    if (executed_count.load() != 0)
    {
        std::cerr << "Dependent node should not execute after failure." << std::endl;
        return 1;
    }

    std::cout << "Scheduler failure handling test passed." << std::endl;
    return 0;
}
