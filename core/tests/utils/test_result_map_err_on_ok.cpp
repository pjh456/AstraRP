#include <iostream>
#include <string>

#include "utils/result.hpp"

int main()
{
    using astra_rp::utils::Result;

    auto ok = Result<int, std::string>::Ok(3);
    auto mapped = ok.map_err([](const std::string &msg) { return msg.size(); });

    if (mapped.is_err())
    {
        std::cerr << "map_err on Ok should remain Ok." << std::endl;
        return 1;
    }

    if (mapped.unwrap() != 3)
    {
        std::cerr << "map_err on Ok should preserve value." << std::endl;
        return 1;
    }

    std::cout << "Result map_err on Ok test passed." << std::endl;
    return 0;
}
