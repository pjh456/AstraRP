#include "core/batch_manager.hpp"

namespace astra_rp
{
    namespace core
    {
        BatchManager &BatchManager::instance()
        {
            static BatchManager manager;
            return manager;
        }

        BatchManager::BatchManager() : m_init_manager(InitManager::instance()) {}

        BatchManager::~BatchManager() = default;

        MulPtr<Batch> BatchManager::acquire(
            int32_t required_tokens, int32_t required_seqs)
        {
            std::lock_guard<std::mutex> lock(m_mtx);

            for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
            {
                if ((*it)->capacity() < required_tokens)
                    continue;
                if ((*it)->max_seqs() < required_seqs)
                    continue;

                auto batch_ptr = std::move(*it);
                m_pool.erase(it);
                batch_ptr->clear();

                return std::shared_ptr<Batch>(
                    batch_ptr.release(),
                    [this](Batch *b)
                    { this->release(b); });
            }

            // 向上取整复用
            int32_t alloc_size = 512;
            while (alloc_size < required_tokens)
                alloc_size *= 2;

            auto *new_batch = new Batch(alloc_size, required_seqs);

            return std::shared_ptr<Batch>(
                new_batch,
                [this](Batch *b)
                { this->release(b); });
        }

        void BatchManager::release(Batch *batch)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_pool.push_back(UniPtr<Batch>(batch));
        }
    }
}