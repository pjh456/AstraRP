#include <iostream>

#include "utils/error.hpp"

int main()
{
    using namespace astra_rp::utils;

    auto err = ErrorBuilder()
                   .infer()
                   .engine_decode_failed()
                   .message("decode")
                   .build();

    if (err.domain() != ErrorDomain::Infer)
    {
        std::cerr << "Error domain mismatch." << std::endl;
        return 1;
    }

    if (err.code() != ErrorCode::EngineDecodeFailed)
    {
        std::cerr << "Error code mismatch." << std::endl;
        return 1;
    }

    if (err.message() != "decode")
    {
        std::cerr << "Error message mismatch." << std::endl;
        return 1;
    }

    std::cout << "ErrorBuilder fields test passed." << std::endl;
    return 0;
}
