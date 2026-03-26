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
    graph.add_edge("A", "B");

    const auto &in_degrees = graph.in_degrees();
    if (in_degrees.at("A") != 0 || in_degrees.at("B") != 1)
    {
        std::cerr << "Unexpected in-degrees: A=" << in_degrees.at("A")
                  << ", B=" << in_degrees.at("B") << std::endl;
        return 1;
    }

    const auto &links = graph.link_table();
    auto it = links.find("A");
    if (it == links.end() || it->second.size() != 1 || it->second[0] != "B")
    {
        std::cerr << "Unexpected link table for A." << std::endl;
        return 1;
    }

    std::cout << "Graph in-degree tracking test passed." << std::endl;
    return 0;
}
