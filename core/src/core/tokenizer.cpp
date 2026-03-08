#include "core/tokenizer.hpp"

#include <stdexcept>

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        ResultV<Vec<Token>>
        Tokenizer::tokenize(
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
                return ResultV<Vec<Token>>::Err(
                    utils::ErrorBuilder()
                        .core()
                        .tokenize_failed()
                        .message("llama_tokenize returned negative status (Buffer too small or invalid utf-8)")
                        .context_id(model->name())
                        .build());

            return ResultV<Vec<Token>>::Ok(std::move(result));
        }

        ResultV<Str>
        Tokenizer::detokenize(
            MulPtr<Model> model,
            const Vec<Token> &tokens,
            bool remove_special,
            bool unparse_special)
        {
            if (tokens.empty())
                return ResultV<Str>::Ok("");

            auto vocab = llama_model_get_vocab(model->raw());

            auto str_count = -llama_detokenize(
                vocab,
                tokens.data(), tokens.size(),
                nullptr, 0,
                remove_special,
                unparse_special);

            if (str_count <= 0)
                return ResultV<Str>::Err(
                    utils::ErrorBuilder()
                        .core()
                        .detokenize_failed()
                        .message("llama_detokenize returned non-positive status (Buffer too small or invalid tokens)")
                        .context_id(model->name())
                        .build());

            Str result;
            result.resize(str_count);

            auto written = llama_detokenize(
                vocab,
                tokens.data(), tokens.size(),
                result.data(), result.size(),
                remove_special,
                unparse_special);

            if (written < 0)
                return ResultV<Str>::Err(
                    utils::ErrorBuilder()
                        .core()
                        .detokenize_failed()
                        .message("llama_detokenize failed mapping tokens to string")
                        .context_id(model->name())
                        .build());

            if (written != str_count)
                result.resize(written);

            return ResultV<Str>::Ok(std::move(result));
        }
    }
}