#include <iostream>

#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"
#include "pipeline/scheduler.hpp"
#include "utils/error.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class Producer final : public BaseNode
        {
        public:
            Producer(const Str &id) : BaseNode(id, nullptr) {}

            ResultV<void> execute() override
            {
                NodePayload payload;
                payload.output = "fanout";
                m_output = payload;
                return ResultV<void>::Ok();
            }
        };

        class Consumer final : public BaseNode
        {
        private:
            Str m_parent_id;

        public:
            Consumer(const Str &id, const Str &parent)
                : BaseNode(id, nullptr), m_parent_id(parent) {}

            ResultV<void> execute() override
            {
                auto it = m_inputs.find(m_parent_id);
                if (it == m_inputs.end() || it->second.output != "fanout")
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .missing_dependency()
                            .message("missing fanout input")
                            .build());
                }
                return ResultV<void>::Ok();
            }
        };
    }
}

int main()
{
    using namespace astra_rp;

    auto graph = std::make_shared<pipeline::Graph>();
    graph->add_node(std::make_shared<pipeline::Producer>("A"));
    graph->add_node(std::make_shared<pipeline::Consumer>("B", "A"));
    graph->add_node(std::make_shared<pipeline::Consumer>("C", "A"));

    graph->add_edge("A", "B");
    graph->add_edge("A", "C");

    pipeline::Scheduler scheduler(graph, 1);
    auto res = scheduler.run();

    if (res.is_err())
    {
        std::cerr << "Scheduler fanout failed: " << res.unwrap_err().to_string() << std::endl;
        return 1;
    }

    std::cout << "Scheduler fanout test passed." << std::endl;
    return 0;
}
