#include <iostream>

#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class DummyNode final : public BaseNode
        {
        public:
            DummyNode(const Str &id, MulPtr<EventBus> bus = nullptr)
                : BaseNode(id, bus) {}

            ResultV<void> execute() override
            {
                return ResultV<void>::Ok();
            }
        };
    }
}

int main()
{
    using namespace astra_rp;

    // Case 1: valid DAG
    {
        pipeline::Graph graph;
        graph.add_node(std::make_shared<pipeline::DummyNode>("A"));
        graph.add_node(std::make_shared<pipeline::DummyNode>("B"));
        graph.add_edge("A", "B");

        auto res = graph.validate();
        if (res.is_err())
        {
            std::cerr << "Expected valid DAG, got error: " << res.unwrap_err().to_string() << std::endl;
            return 1;
        }
    }

    // Case 2: cycle detection
    {
        pipeline::Graph graph;
        graph.add_node(std::make_shared<pipeline::DummyNode>("A"));
        graph.add_node(std::make_shared<pipeline::DummyNode>("B"));
        graph.add_edge("A", "B");
        graph.add_edge("B", "A");

        auto res = graph.validate();
        if (res.is_ok())
        {
            std::cerr << "Expected cycle validation to fail, but it passed." << std::endl;
            return 1;
        }
    }

    // Case 3: edge to unknown node
    {
        pipeline::Graph graph;
        graph.add_node(std::make_shared<pipeline::DummyNode>("A"));
        graph.add_edge("A", "MISSING");

        auto res = graph.validate();
        if (res.is_ok())
        {
            std::cerr << "Expected validation to fail for missing node, but it passed." << std::endl;
            return 1;
        }
    }

    std::cout << "Graph validation tests passed." << std::endl;
    return 0;
}
