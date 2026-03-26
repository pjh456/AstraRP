#include <iostream>
#include <string>

#include "utils/result.hpp"

int main()
{
    using astra_rp::utils::Result;

    auto ok = Result<int, std::string>::Ok(42);
    if (!ok.is_ok() || ok.unwrap() != 42)
    {
        std::cerr << "Result Ok unwrap failed." << std::endl;
        return 1;
    }

    auto err = Result<int, std::string>::Err("boom");
    if (!err.is_err() || err.unwrap_err() != "boom")
    {
        std::cerr << "Result Err unwrap failed." << std::endl;
        return 1;
    }

    auto mapped = ok.map([](int v) { return v * 2; });
    if (mapped.is_err() || mapped.unwrap() != 84)
    {
        std::cerr << "Result map failed." << std::endl;
        return 1;
    }

    auto mapped_err = err.map_err([](const std::string &msg) { return msg.size(); });
    if (mapped_err.is_ok() || mapped_err.unwrap_err() != 4)
    {
        std::cerr << "Result map_err failed." << std::endl;
        return 1;
    }

    auto chained = ok.and_then([](int v) { return Result<int, std::string>::Ok(v + 1); });
    if (chained.is_err() || chained.unwrap() != 43)
    {
        std::cerr << "Result and_then failed." << std::endl;
        return 1;
    }

    auto chained_err = err.and_then([](int v) { return Result<int, std::string>::Ok(v + 1); });
    if (chained_err.is_ok() || chained_err.unwrap_err() != "boom")
    {
        std::cerr << "Result and_then on Err failed." << std::endl;
        return 1;
    }

    auto ok_void = Result<void, std::string>::Ok();
    bool void_mapped = false;
    auto ok_void_mapped = ok_void.map([&]() { void_mapped = true; });
    if (ok_void_mapped.is_err() || !void_mapped)
    {
        std::cerr << "Result<void> map failed." << std::endl;
        return 1;
    }

    auto err_void = Result<void, std::string>::Err("nope");
    auto err_void_mapped = err_void.map_err([](const std::string &msg) { return msg + "!"; });
    if (err_void_mapped.is_ok() || err_void_mapped.unwrap_err() != "nope!")
    {
        std::cerr << "Result<void> map_err failed." << std::endl;
        return 1;
    }

    std::cout << "Result basic tests passed." << std::endl;
    return 0;
}
