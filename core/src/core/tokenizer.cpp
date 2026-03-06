#include "core/tokenizer.hpp"

#include <stdexcept>

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        Vec<Token> Tokenizer::tokenize(
            MulPtr<Model> model,
            const Str &text,
            bool add_special,
            bool parse_special)
        {
            auto vocab = llama_model_get_vocab(model->raw());
            auto token_count = -llama_tokenize(
                vocab,
                text.c_str(), text.size(),
                nullptr, 0,
                add_special,
                parse_special);

            Vec<Token> result(token_count);
            auto status =
                llama_tokenize(
                    vocab,
                    text.c_str(), text.size(),
                    result.data(), result.size(),
                    add_special, parse_special);
            if (status < 0)
                throw std::runtime_error("Failed to tokenize the text.");

            return result;
        }

        Str Tokenizer::detokenize(
            MulPtr<Model> model,
            const Vec<Token> &tokens,
            bool remove_special,
            bool unparse_special)
        {
            if (tokens.empty())
                return "";

            auto vocab = llama_model_get_vocab(model->raw());

            auto str_count = -llama_detokenize(
                vocab,
                tokens.data(), tokens.size(),
                nullptr, 0,
                remove_special,
                unparse_special);

            if (str_count <= 0)
                throw std::runtime_error("detokenize failed!");

            Str result;
            result.resize(str_count);

            auto written = llama_detokenize(
                vocab,
                tokens.data(), tokens.size(),
                result.data(), result.size(),
                remove_special,
                unparse_special);

            if (written < 0)
                throw std::runtime_error("detokenize failed!");

            if (written != str_count)
                result.resize(written);

            return result;
        }
    }
}