#include <iostream>
#include <string>

#include "utils/result.hpp"

using VoidResult = astra_rp::utils::Result<void, std::string>;

VoidResult ok_step()
{
    return VoidResult::Ok();
}

VoidResult fail_step()
{
    return VoidResult::Err("fail");
}

int main()
{
    auto ok = ok_step().and_then([]() { return ok_step(); });
    if (ok.is_err())
    {
        std::cerr << "Void and_then ok path failed." << std::endl;
        return 1;
    }

    auto err = ok_step().and_then([]() { return fail_step(); });
    if (err.is_ok() || err.unwrap_err() != "fail")
    {
        std::cerr << "Void and_then error propagation failed." << std::endl;
        return 1;
    }

    std::cout << "Result<void> and_then test passed." << std::endl;
    return 0;
}
