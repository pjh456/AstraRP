#include <iostream>
#include <string>

#include "utils/result.hpp"

using IntResult = astra_rp::utils::Result<int, std::string>;
using VoidResult = astra_rp::utils::Result<void, std::string>;

IntResult parse_value(bool ok)
{
    if (ok)
        return IntResult::Ok(1);
    return IntResult::Err("bad");
}

IntResult add_one(bool ok)
{
    ASSIGN_OR_RETURN(value, parse_value(ok));
    return IntResult::Ok(value + 1);
}

VoidResult step(bool ok)
{
    if (ok)
        return VoidResult::Ok();
    return VoidResult::Err("nope");
}

VoidResult run_steps(bool ok_first, bool ok_second)
{
    TRY(step(ok_first));
    TRY(step(ok_second));
    return VoidResult::Ok();
}

int main()
{
    auto ok = add_one(true);
    if (ok.is_err() || ok.unwrap() != 2)
    {
        std::cerr << "ASSIGN_OR_RETURN success path failed." << std::endl;
        return 1;
    }

    auto err = add_one(false);
    if (err.is_ok() || err.unwrap_err() != "bad")
    {
        std::cerr << "ASSIGN_OR_RETURN error path failed." << std::endl;
        return 1;
    }

    auto steps_ok = run_steps(true, true);
    if (steps_ok.is_err())
    {
        std::cerr << "TRY success path failed." << std::endl;
        return 1;
    }

    auto steps_err = run_steps(true, false);
    if (steps_err.is_ok() || steps_err.unwrap_err() != "nope")
    {
        std::cerr << "TRY error path failed." << std::endl;
        return 1;
    }

    std::cout << "Result macro tests passed." << std::endl;
    return 0;
}
