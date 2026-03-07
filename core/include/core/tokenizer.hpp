#ifndef INCLUDE_ASTRA_RP_TOKENIZER_HPP
#define INCLUDE_ASTRA_RP_TOKENIZER_HPP

#include "utils/types.hpp"
#include "core/model.hpp"

namespace astra_rp
{
    namespace core
    {
        class Tokenizer
        {
        public:
            static ResultV<Vec<Token>> tokenize(
                MulPtr<Model> model,
                const Str &text,
                bool add_special = true,
                bool parse_special = true);

            static ResultV<Str> detokenize(
                MulPtr<Model> model,
                const Vec<Token> &tokens,
                bool remove_special = false,
                bool unparse_special = true);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_TOKENIZER_HPP