#include <iostream>

#include "utils/error.hpp"

int main()
{
    using namespace astra_rp::utils;

    auto err = ErrorBuilder()
                   .system()
                   .invalid_argument()
                   .context_id("ctx-1")
                   .message("bad")
                   .build();

    if (err.context_id() != "ctx-1")
    {
        std::cerr << "Context id mismatch." << std::endl;
        return 1;
    }

    std::cout << "Error context_id test passed." << std::endl;
    return 0;
}
