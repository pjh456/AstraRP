#ifndef INCLUDE_ASTRA_RP_DECODE_TASK_HPP
#define INCLUDE_ASTRA_RP_DECODE_TASK_HPP

#include "utils/types.hpp"
#include "infer/i_task.hpp"
#include "infer/engine.hpp"

namespace astra_rp
{
    namespace infer
    {
        class Session;

        class DecodeTask : public ITask
        {
        private:
            MulPtr<Session> m_session;
            DecodeParams m_params;

            Vec<Token> m_tokens;

        public:
            DecodeTask(
                MulPtr<Session> session,
                Vec<Token> tokens,
                DecodeParams params = {})
                : m_session(session),
                  m_tokens(std::move(tokens)),
                  m_params(params) {}

        public:
            Str name() const noexcept override { return "DecodeTask"; }

            ResultV<void> execute() override;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_DECODE_TASK_HPP