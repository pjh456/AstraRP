#include "infer/context_transaction.hpp"

#include "infer/session.hpp"

#include "utils/logger.hpp"

namespace astra_rp
{
    namespace infer
    {
        ContextTransaction::ContextTransaction(
            MulPtr<Session> session)
            : m_session(session),
              m_n_past_start(session->n_past()) {}

        void ContextTransaction::rollback()
        {
            auto ctx = m_session->context();
            auto mem = llama_get_memory(ctx->raw());

            llama_memory_seq_rm(
                mem,
                m_session->seq_id(),
                m_n_past_start,
                -1);

            m_session->set_n_past(m_n_past_start);

            ASTRA_LOG_WARN("ContextTransaction: Rollback triggered, KV Cache reverted.");
        }
    }
}