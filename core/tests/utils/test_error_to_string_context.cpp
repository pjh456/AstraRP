#include <iostream>
#include <string>

#include "utils/error.hpp"

int main()
{
    using namespace astra_rp::utils;

    auto err = ErrorBuilder()
                   .pipeline()
                   .graph_cycle_detected()
                   .context_id("node-x")
                   .message("oops")
                   .build();

    auto text = err.to_string();
    if (text.find("<node-x>") == std::string::npos)
    {
        std::cerr << "Expected context id in to_string output." << std::endl;
        return 1;
    }

    std::cout << "Error to_string context test passed." << std::endl;
    return 0;
}
