#include <iostream>
#include <atomic>

#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"
#include "pipeline/scheduler.hpp"

namespace astra_rp
{
    namespace pipeline
    {
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

    std::atomic<int> counter{0};

    auto graph = std::make_shared<pipeline::Graph>();
    graph->add_node(std::make_shared<pipeline::CountingNode>("A", counter));
    graph->add_node(std::make_shared<pipeline::CountingNode>("B", counter));

    pipeline::Scheduler scheduler(graph, 2);
    auto res = scheduler.run();

    if (res.is_err())
    {
        std::cerr << "Scheduler run failed: " << res.unwrap_err().to_string() << std::endl;
        return 1;
    }

    if (counter.load() != 2)
    {
        std::cerr << "Expected both root nodes to execute, got " << counter.load() << std::endl;
        return 1;
    }

    std::cout << "Scheduler multiple roots test passed." << std::endl;
    return 0;
}
