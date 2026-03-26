#include <iostream>

#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"
#include "pipeline/scheduler.hpp"
#include "utils/error.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class ProducingNode final : public BaseNode
        {
        public:
            ProducingNode(const Str &id, MulPtr<EventBus> bus = nullptr)
                : BaseNode(id, bus) {}

            ResultV<void> execute() override
            {
                NodePayload payload;
                payload.output = "hello";
                m_output = payload;
                return ResultV<void>::Ok();
            }
        };

        class ConsumingNode final : public BaseNode
        {
        private:
            Str m_parent_id;

        public:
            ConsumingNode(const Str &id, const Str &parent_id, MulPtr<EventBus> bus = nullptr)
                : BaseNode(id, bus), m_parent_id(parent_id) {}

            ResultV<void> execute() override
            {
                auto it = m_inputs.find(m_parent_id);
                if (it == m_inputs.end())
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .missing_dependency()
                            .message("Missing input from parent: " + m_parent_id)
                            .build());
                }

                if (it->second.output != "hello")
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .invalid_argument()
                            .message("Unexpected input payload")
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

    pipeline::Graph graph;
    graph.add_node(std::make_shared<pipeline::ProducingNode>("A"));
    graph.add_node(std::make_shared<pipeline::ConsumingNode>("B", "A"));
    graph.add_edge("A", "B");

    pipeline::Scheduler scheduler(std::make_shared<pipeline::Graph>(graph), 1);
    auto res = scheduler.run();

    if (res.is_err())
    {
        std::cerr << "Scheduler run failed: " << res.unwrap_err().to_string() << std::endl;
        return 1;
    }

    std::cout << "Scheduler basic dependency test passed." << std::endl;
    return 0;
}
