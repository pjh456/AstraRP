#include <iostream>
#include <algorithm>

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
    graph.add_edge("A", "C");

    const auto &links = graph.link_table();
    auto it = links.find("A");
    if (it == links.end() || it->second.size() != 2)
    {
        std::cerr << "Expected two children in link table." << std::endl;
        return 1;
    }

    auto has_b = std::find(it->second.begin(), it->second.end(), "B") != it->second.end();
    auto has_c = std::find(it->second.begin(), it->second.end(), "C") != it->second.end();
    if (!has_b || !has_c)
    {
        std::cerr << "Link table missing expected children." << std::endl;
        return 1;
    }

    std::cout << "Graph link table children test passed." << std::endl;
    return 0;
}
