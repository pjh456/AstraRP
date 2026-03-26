#include <iostream>
#include <string>

#include "utils/result.hpp"

int main()
{
    using astra_rp::utils::Result;

    auto ok = Result<int, std::string>::Ok(7);
    auto err = Result<int, std::string>::Err("fail");

    if (ok.unwrap_or(3) != 7)
    {
        std::cerr << "unwrap_or should return Ok value." << std::endl;
        return 1;
    }

    if (err.unwrap_or(3) != 3)
    {
        std::cerr << "unwrap_or should return default on Err." << std::endl;
        return 1;
    }

    if (ok.unwrap_err_or("x") != "x")
    {
        std::cerr << "unwrap_err_or should return default on Ok." << std::endl;
        return 1;
    }

    if (err.unwrap_err_or("x") != "fail")
    {
        std::cerr << "unwrap_err_or should return Err value." << std::endl;
        return 1;
    }

    std::cout << "Result unwrap_or tests passed." << std::endl;
    return 0;
}
