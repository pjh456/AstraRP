#ifndef INCLUDE_ASTRA_RP_GENERATION_CONFIG_HPP
#define INCLUDE_ASTRA_RP_GENERATION_CONFIG_HPP

#include "utils/types.hpp"

namespace astra_rp
{
    namespace infer
    {
        struct GenerationConfig
        {
            bool add_special = true;
            bool parse_special = true;

            int32_t max_tokens = -1;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_GENERATION_CONFIG_HPP