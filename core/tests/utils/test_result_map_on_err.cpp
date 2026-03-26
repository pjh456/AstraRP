#include <iostream>
#include <string>

#include "utils/result.hpp"

int main()
{
    using astra_rp::utils::Result;

    auto err = Result<int, std::string>::Err("bad");
    auto mapped = err.map([](int v) { return v + 1; });

    if (mapped.is_ok())
    {
        std::cerr << "map on Err should remain Err." << std::endl;
        return 1;
    }

    if (mapped.unwrap_err() != "bad")
    {
        std::cerr << "map on Err should preserve error." << std::endl;
        return 1;
    }

    std::cout << "Result map on Err test passed." << std::endl;
    return 0;
}
