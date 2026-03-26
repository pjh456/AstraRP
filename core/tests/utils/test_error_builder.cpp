#include <iostream>
#include <string>

#include "utils/error.hpp"

int main()
{
    using astra_rp::utils::ErrorBuilder;

    auto root = ErrorBuilder()
                    .core()
                    .model_load_failed()
                    .message("root")
                    .build();

    auto err = ErrorBuilder()
                   .pipeline()
                   .graph_cycle_detected()
                   .context_id("node-1")
                   .message("boom")
                   .wrap(root)
                   .build();

    auto text = err.to_string();

    if (text.find("[PIPELINE]") == std::string::npos)
    {
        std::cerr << "Missing domain in error string." << std::endl;
        return 1;
    }

    if (text.find("<node-1>") == std::string::npos)
    {
        std::cerr << "Missing context id in error string." << std::endl;
        return 1;
    }

    if (text.find("boom") == std::string::npos)
    {
        std::cerr << "Missing message in error string." << std::endl;
        return 1;
    }

    if (text.find("Caused by") == std::string::npos)
    {
        std::cerr << "Missing cause chain in error string." << std::endl;
        return 1;
    }

    std::cout << "ErrorBuilder tests passed." << std::endl;
    return 0;
}
