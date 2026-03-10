#ifndef INCLUDE_ASTRA_RP_TOKENIZE_TASK_HPP
#define INCLUDE_ASTRA_RP_TOKENIZE_TASK_HPP

#include "utils/types.hpp"
#include "infer/i_task.hpp"
#include "infer/params.hpp"

namespace astra_rp
{
    namespace core
    {
        class Model;
    }

    namespace infer
    {
        class TokenizeTask : public ITask
        {
        private:
            MulPtr<core::Model> m_model;
            TokenizeParams m_params;

            Str m_prompt;
            Vec<Token> m_tokens;

        public:
            TokenizeTask(
                MulPtr<core::Model> model,
                Str prompt,
                TokenizeParams params = {})
                : m_model(model),
                  m_prompt(std::move(prompt)),
                  m_params(params) {}

        public:
            Str name() const noexcept override { return "TokenizeTask"; }
            const Vec<Token> &tokens() const { return m_tokens; }
            ResultV<void> execute() override;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_TOKENIZE_TASK_HPP