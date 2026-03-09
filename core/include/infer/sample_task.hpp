#ifndef INCLUDE_ASTRA_RP_SAMPLE_TASK_HPP
#define INCLUDE_ASTRA_RP_SAMPLE_TASK_HPP

#include "utils/types.hpp"
#include "infer/i_task.hpp"
#include "core/sampler_chain.hpp"

namespace astra_rp
{
    namespace core
    {
        class Context;
    }

    namespace infer
    {
        class Session;

        class SampleTask : public ITask
        {
        private:
            MulPtr<Session> m_session;
            Token m_token = -1;

        public:
            explicit SampleTask(MulPtr<Session> session)
                : m_session(session) {}

        public:
            Str name() const noexcept override { return "SampleTask"; }
            Token token() const noexcept { return m_token; }
            ResultV<void> execute() override;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SAMPLE_TASK_HPP