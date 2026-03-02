#ifndef INCLUDE_ASTRA_RP_BATCH_MANAGER_HPP
#define INCLUDE_ASTRA_RP_BATCH_MANAGER_HPP

#include "utils/types.hpp"

#include "core/init_manager.hpp"

#include <mutex>
#include <list>

namespace astra_rp
{
    namespace core
    {
        class Batch;

        class BatchManager
        {
        private:
            std::mutex m_mtx;
            std::list<UniPtr<Batch>> m_pool;

        private:
            InitManager &m_init_manager;

        public:
            static BatchManager &instance();

        private:
            BatchManager();
            ~BatchManager();

        public:
            BatchManager(const BatchManager &) = delete;
            BatchManager &operator=(const BatchManager &) = delete;

            BatchManager(BatchManager &&) noexcept = default;
            BatchManager &operator=(BatchManager &&) noexcept = default;

        public:
            MulPtr<Batch> acquire(int32_t required_tokens);

        private:
            void release(Batch *batch);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_BATCH_MANAGER_HPP