#include <iostream>

#include "utils/error.hpp"

int main()
{
    using namespace astra_rp::utils;

    if (std::string(domain_to_string(ErrorDomain::Core)) != "CORE")
    {
        std::cerr << "Core domain string mismatch." << std::endl;
        return 1;
    }
    if (std::string(domain_to_string(ErrorDomain::Infer)) != "INFER")
    {
        std::cerr << "Infer domain string mismatch." << std::endl;
        return 1;
    }
    if (std::string(domain_to_string(ErrorDomain::Pipeline)) != "PIPELINE")
    {
        std::cerr << "Pipeline domain string mismatch." << std::endl;
        return 1;
    }
    if (std::string(domain_to_string(ErrorDomain::System)) != "SYSTEM")
    {
        std::cerr << "System domain string mismatch." << std::endl;
        return 1;
    }

    std::cout << "Error domain strings test passed." << std::endl;
    return 0;
}
