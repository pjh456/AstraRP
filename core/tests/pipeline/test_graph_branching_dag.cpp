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

    pipeline::Graph graph;
    graph.add_node(std::make_shared<pipeline::DummyNode>("A"));
    graph.add_node(std::make_shared<pipeline::DummyNode>("B"));
    graph.add_node(std::make_shared<pipeline::DummyNode>("C"));
    graph.add_node(std::make_shared<pipeline::DummyNode>("D"));

    graph.add_edge("A", "B");
    graph.add_edge("A", "C");
    graph.add_edge("B", "D");
    graph.add_edge("C", "D");

    auto res = graph.validate();
    if (res.is_err())
    {
        std::cerr << "Branching DAG should validate." << std::endl;
        return 1;
    }

    std::cout << "Graph branching DAG test passed." << std::endl;
    return 0;
}
