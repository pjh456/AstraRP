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

    graph.add_edge("A", "C");
    graph.add_edge("B", "C");

    const auto &in_degrees = graph.in_degrees();
    if (in_degrees.at("C") != 2)
    {
        std::cerr << "Expected C in-degree to be 2, got " << in_degrees.at("C") << std::endl;
        return 1;
    }

    auto res = graph.validate();
    if (res.is_err())
    {
        std::cerr << "Graph with multiple parents should be valid." << std::endl;
        return 1;
    }

    std::cout << "Graph multiple parents test passed." << std::endl;
    return 0;
}
