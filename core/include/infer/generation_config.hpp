#ifndef INCLUDE_ASTRA_RP_GENERATION_CONFIG_HPP
#define INCLUDE_ASTRA_RP_GENERATION_CONFIG_HPP

#include "utils/types.hpp"

namespace astra_rp
{
    namespace infer
    {
        struct GenerationConfig
        {
            int32_t max_tokens = -1;
            Str grammar_rules = "";
        };
    }
}

#endif // INCLUDE_ASTRA_RP_GENERATION_CONFIG_HPP