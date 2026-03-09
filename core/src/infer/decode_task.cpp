#include "infer/decode_task.hpp"

#include "infer/session.hpp"
#include "infer/engine.hpp"

namespace astra_rp
{
    namespace infer
    {
        ResultV<void>
        DecodeTask::execute()
        {
            ASSIGN_OR_RETURN(
                batches,
                Engine::instance()
                    .all_tok2batch(
                        m_session,
                        m_tokens));

            TRY(Engine::instance()
                    .decode(
                        m_session,
                        batches)
                    .map_err(
                        [this](auto err)
                        {
                            return utils::ErrorBuilder()
                                .infer()
                                .engine_decode_failed()
                                .message("Batch decode failed!")
                                .context_id(
                                    m_session
                                        ->context()
                                        ->model()
                                        .name())
                                .wrap(std::move(err))
                                .build();
                        }));

            return ResultV<void>::Ok();
        }
    }
}