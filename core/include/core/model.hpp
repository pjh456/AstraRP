#ifndef INCLUDE_ASTRA_RP_MODEL_HPP
#define INCLUDE_ASTRA_RP_MODEL_HPP

#include "utils/types.hpp"

struct llama_model;

namespace astra_rp
{
    namespace core
    {
        class ModelManager;

        class Model
        {
            friend class ModelManager;

        private:
            llama_model *m_model = nullptr;
            Str m_name;

        private:
            Model(const Str &name, llama_model *model = nullptr);

        public:
            ~Model();

            Model(const Model &) = delete;
            Model &operator=(const Model &) = delete;

            Model(Model &&) noexcept = default;
            Model &operator=(Model &&) noexcept = default;

        public:
            llama_model *raw() const noexcept { return m_model; }
            Str name() const noexcept { return m_name; }

        public:
            Vec<Token> tokenize(
                const Str &text,
                bool add_special = true,
                bool parse_special = true) const;

            Str detokenize(
                const Vec<Token> &tokens,
                bool remove_special = false,
                bool unparse_special = true) const;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_MODEL_HPP