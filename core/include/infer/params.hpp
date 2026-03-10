#ifndef INCLUDE_ASTRA_RP_PARAMS_HPP
#define INCLUDE_ASTRA_RP_PARAMS_HPP

#include <cstdint>

#include "utils/types.hpp"

namespace astra_rp
{
    namespace infer
    {
        struct TokenizeParams
        {
            bool add_special = true;
            bool parse_special = true;
        };

        struct DecodeParams
        {
            int32_t max_tokens = -1;
        };

        struct SampleParams
        {
            float temperature = -1.0;
            int32_t top_k = -1;
            std::pair<float, size_t> top_p = {-1.0, 0};
            Str grammar = "";
            int32_t seed = -1;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_PARAMS_HPP