#ifndef INCLUDE_ASTRA_RP_CONTEXT_TRANSACTION_HPP
#define INCLUDE_ASTRA_RP_CONTEXT_TRANSACTION_HPP

#include "utils/types.hpp"

namespace astra_rp
{
    namespace infer
    {
        class Session;

        class ContextTransaction
        {
        private:
            MulPtr<Session> m_session;
            int32_t m_n_past_start;
            bool m_committed = false;

        public:
            explicit ContextTransaction(MulPtr<Session> session);

            ~ContextTransaction()
            {
                if (!m_committed)
                    rollback();
            }

        public:
            void commit() { m_committed = true; }
            void rollback();
        };
    }
}

#endif // INCLUDE_ASTRA_RP_CONTEXT_TRANSACTION_HPP