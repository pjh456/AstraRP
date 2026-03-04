#include "core/model.hpp"

#include "llama.h"

#include <stdexcept>

namespace astra_rp
{
    namespace core
    {
        Model::Model(const Str &name, llama_model *model)
            : m_name(name), m_model(model) {}

        Model::~Model()
        {
            if (m_model)
                llama_model_free(m_model);
        }

        Vec<Token> Model::tokenize(
            const Str &text,
            bool add_special,
            bool parse_special) const
        {
            auto vocab = llama_model_get_vocab(m_model);
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

        Str Model::detokenize(
            const Vec<Token> &tokens,
            bool remove_special,
            bool unparse_special) const
        {
            if (tokens.empty())
                return "";

            auto vocab = llama_model_get_vocab(m_model);

            auto str_count = -llama_detokenize(
                vocab,
                tokens.data(), tokens.size(),
                nullptr, 0,
                remove_special,
                unparse_special);

            // TODO: 更明显的错误处理
            if (str_count <= 0)
                return "";

            Str result;
            result.resize(str_count);

            auto written = llama_detokenize(
                vocab,
                tokens.data(), tokens.size(),
                result.data(), result.size(),
                remove_special,
                unparse_special);

            // TODO: 更明显的错误处理
            if (written < 0)
                return "";

            if (written != str_count)
                result.resize(written);

            return result;
        }
    }
}