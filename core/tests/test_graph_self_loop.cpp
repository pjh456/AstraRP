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
    graph.add_edge("A", "A");

    auto res = graph.validate();
    if (res.is_ok())
    {
        std::cerr << "Expected self-loop to fail validation, but it passed." << std::endl;
        return 1;
    }

    std::cout << "Graph self-loop test passed." << std::endl;
    return 0;
}
