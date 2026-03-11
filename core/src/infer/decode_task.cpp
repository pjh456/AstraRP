#include "infer/decode_task.hpp"

#include "infer/session.hpp"
#include "infer/engine.hpp"
#include "infer/context_transaction.hpp"

namespace astra_rp
{
    namespace infer
    {
        ResultV<void>
        DecodeTask::execute()
        {
            ContextTransaction tx(m_session);

            ASSIGN_OR_RETURN(
                batches,
                Engine::instance()
                    .all_tok2batch(
                        m_session,
                        m_tokens,
                        m_params.max_tokens));

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

            tx.commit();

            return ResultV<void>::Ok();
        }
    }
}