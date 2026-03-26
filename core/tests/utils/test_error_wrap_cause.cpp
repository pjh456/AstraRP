#include <iostream>
#include <string>

#include "utils/error.hpp"

int main()
{
    using namespace astra_rp::utils;

    auto cause = ErrorBuilder()
                     .core()
                     .model_load_failed()
                     .message("root")
                     .build();

    auto err = ErrorBuilder()
                   .infer()
                   .engine_decode_failed()
                   .message("top")
                   .wrap(cause)
                   .build();

    if (!err.cause())
    {
        std::cerr << "Expected wrapped cause to exist." << std::endl;
        return 1;
    }

    if (err.cause()->message() != "root")
    {
        std::cerr << "Wrapped cause message mismatch." << std::endl;
        return 1;
    }

    auto text = err.to_string();
    if (text.find("Caused by") == std::string::npos)
    {
        std::cerr << "Expected to_string to include cause chain." << std::endl;
        return 1;
    }

    std::cout << "Error wrap cause test passed." << std::endl;
    return 0;
}
