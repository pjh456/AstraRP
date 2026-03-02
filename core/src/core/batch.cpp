#include "core/batch.hpp"

#include <stdexcept>

namespace astra_rp
{
    namespace core
    {
        Batch::Batch(int32_t max_tokens, int32_t max_seqs)
            : m_max_tokens(max_tokens), m_max_seqs(max_seqs)
        {
            m_batch = {0};
            m_batch = llama_batch_init(max_tokens, 0, max_seqs);
        }

        Batch::~Batch()
        {
            if (m_batch.token)
                llama_batch_free(m_batch);
        }

        void Batch::add(
            Token id,
            int32_t pos,
            const Vec<int32_t> &seq_ids,
            bool logits)
        {
            auto i = m_batch.n_tokens;
            if (i >= m_max_tokens)
                throw std::runtime_error("Batch token capacity exceeded");

            auto seq_size = seq_ids.size();
            if ((int32_t)seq_size > m_max_seqs)
                throw std::runtime_error("Batch sequence capacity exceeded (seq_ids size > max_seqs)");

            m_batch.token[i] = id;
            m_batch.pos[i] = pos;
            m_batch.logits[i] = logits; // 是否要结果

            // 处理序列 ID
            m_batch.n_seq_id[i] = (int32_t)seq_size;
            for (size_t j = 0; j < seq_size; ++j)
                m_batch.seq_id[i][j] = seq_ids[j];

            m_batch.n_tokens++;
        }
    }
}