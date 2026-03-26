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
    graph.add_edge("MISSING", "A");

    auto res = graph.validate();
    if (res.is_ok())
    {
        std::cerr << "Expected validation to fail for missing source node, but it passed." << std::endl;
        return 1;
    }

    std::cout << "Graph missing source test passed." << std::endl;
    return 0;
}
