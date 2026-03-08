#ifndef INCLUDE_ASTRA_RP_BATCH_HPP
#define INCLUDE_ASTRA_RP_BATCH_HPP

#include "utils/types.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        class BatchManager;

        class Batch
        {
            friend class BatchManager;

        private:
            llama_batch m_batch;
            int32_t m_max_tokens;
            int32_t m_max_seqs;

        private:
            Batch(int32_t max_tokens, int32_t max_seqs);

        public:
            ~Batch();

            Batch(const Batch &) = delete;
            Batch operator=(const Batch &) = delete;

            Batch(Batch &&) noexcept;
            Batch &operator=(Batch &&) noexcept;

        public:
            ResultV<void>
            add(
                Token id,
                int32_t pos,
                const Vec<int32_t> &seq_ids,
                bool logits);

            void clear();

        public:
            llama_batch raw() const noexcept { return m_batch; }
            int32_t max_seqs() const noexcept { return m_max_seqs; }
            int32_t size() const noexcept { return m_batch.n_tokens; }
            int32_t capacity() const noexcept { return m_max_tokens; }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_BATCH_HPP