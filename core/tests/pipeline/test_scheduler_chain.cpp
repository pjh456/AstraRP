#include <iostream>

#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"
#include "pipeline/scheduler.hpp"
#include "utils/error.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class ProduceNode final : public BaseNode
        {
        public:
            ProduceNode(const Str &id) : BaseNode(id, nullptr) {}

            ResultV<void> execute() override
            {
                NodePayload payload;
                payload.output = "step1";
                m_output = payload;
                return ResultV<void>::Ok();
            }
        };

        class TransformNode final : public BaseNode
        {
        private:
            Str m_parent_id;

        public:
            TransformNode(const Str &id, const Str &parent)
                : BaseNode(id, nullptr), m_parent_id(parent) {}

            ResultV<void> execute() override
            {
                auto it = m_inputs.find(m_parent_id);
                if (it == m_inputs.end() || it->second.output != "step1")
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .missing_dependency()
                            .message("missing step1")
                            .build());
                }

                NodePayload payload;
                payload.output = "step2";
                m_output = payload;
                return ResultV<void>::Ok();
            }
        };

        class ConsumeNode final : public BaseNode
        {
        private:
            Str m_parent_id;

        public:
            ConsumeNode(const Str &id, const Str &parent)
                : BaseNode(id, nullptr), m_parent_id(parent) {}

            ResultV<void> execute() override
            {
                auto it = m_inputs.find(m_parent_id);
                if (it == m_inputs.end() || it->second.output != "step2")
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .missing_dependency()
                            .message("missing step2")
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
    graph->add_node(std::make_shared<pipeline::ProduceNode>("A"));
    graph->add_node(std::make_shared<pipeline::TransformNode>("B", "A"));
    graph->add_node(std::make_shared<pipeline::ConsumeNode>("C", "B"));

    graph->add_edge("A", "B");
    graph->add_edge("B", "C");

    pipeline::Scheduler scheduler(graph, 1);
    auto res = scheduler.run();

    if (res.is_err())
    {
        std::cerr << "Scheduler chain failed: " << res.unwrap_err().to_string() << std::endl;
        return 1;
    }

    std::cout << "Scheduler chain test passed." << std::endl;
    return 0;
}
