#include "infer/sample_task.hpp"

#include "infer/session.hpp"
#include "infer/engine.hpp"

namespace astra_rp
{
    namespace infer
    {
        ResultV<void>
        SampleTask::execute()
        {
            ASSIGN_OR_RETURN(
                tok,
                Engine::instance()
                    .sample(
                        m_session->context(),
                        m_session->sampler()));
            m_token = tok;
            return ResultV<void>::Ok();
        }
    }
}