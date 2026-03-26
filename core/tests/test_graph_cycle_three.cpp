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

    graph.add_edge("A", "B");
    graph.add_edge("B", "C");
    graph.add_edge("C", "A");

    auto res = graph.validate();
    if (res.is_ok())
    {
        std::cerr << "Expected 3-node cycle to fail validation, but it passed." << std::endl;
        return 1;
    }

    std::cout << "Graph 3-node cycle test passed." << std::endl;
    return 0;
}
