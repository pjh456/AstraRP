#include <iostream>
#include <string>
#include <stdexcept>

#include "utils/result.hpp"

int main()
{
    using astra_rp::utils::Result;

    auto ok = Result<int, std::string>::Ok(5);
    auto err = Result<int, std::string>::Err("nope");

    bool threw = false;
    try
    {
        (void)ok.unwrap_err();
    }
    catch (const std::logic_error &)
    {
        threw = true;
    }

    if (!threw)
    {
        std::cerr << "Expected unwrap_err on Ok to throw." << std::endl;
        return 1;
    }

    threw = false;
    try
    {
        (void)err.unwrap();
    }
    catch (const std::logic_error &)
    {
        threw = true;
    }

    if (!threw)
    {
        std::cerr << "Expected unwrap on Err to throw." << std::endl;
        return 1;
    }

    std::cout << "Result unwrap exception tests passed." << std::endl;
    return 0;
}
