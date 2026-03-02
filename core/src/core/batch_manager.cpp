#include "core/batch_manager.hpp"

#include "core/batch.hpp"

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

        MulPtr<Batch> BatchManager::acquire(int32_t required_tokens)
        {
            std::lock_guard<std::mutex> lock(m_mtx);

            for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
            {
                if ((*it)->capacity() < required_tokens)
                    continue;

                auto batch_ptr = std::move(*it);
                m_pool.erase(it);
                batch_ptr->clear();

                return std::shared_ptr<Batch>(
                    batch_ptr.release(),
                    [this](Batch *b)
                    { this->release(b); });
            }

            int32_t alloc_size = (required_tokens <= 512) ? 512 : required_tokens;
            auto *new_batch = new Batch(alloc_size, 1); // TODO: 把这个 1 改掉

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