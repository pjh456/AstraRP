#include <iostream>

#include "pipeline/graph.hpp"

int main()
{
    using namespace astra_rp;

    pipeline::Graph graph;
    auto res = graph.validate();

    if (res.is_err())
    {
        std::cerr << "Empty graph should be valid, got error: "
                  << res.unwrap_err().to_string() << std::endl;
        return 1;
    }

    std::cout << "Graph empty validation test passed." << std::endl;
    return 0;
}
