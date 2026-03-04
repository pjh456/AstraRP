#ifndef INCLUDE_ASTRA_RP_CONTEXT_MANAGER_HPP
#define INCLUDE_ASTRA_RP_CONTEXT_MANAGER_HPP

#include <mutex>

#include "utils/types.hpp"
#include "core/context.hpp"
#include "core/context_params.hpp"
#include "core/model.hpp"
#include "core/init_manager.hpp"

namespace astra_rp
{
    namespace core
    {

        class ContextManager
        {
        private:
            Map<Str, List<UniPtr<Context>>> m_pool;

            std::mutex m_mtx;
            InitManager &m_init;

        public:
            static ContextManager &instance();

        private:
            ContextManager();

        public:
            ~ContextManager() = default;

            ContextManager(const ContextManager &) = delete;
            ContextManager &operator=(const ContextManager &) = delete;

            ContextManager(ContextManager &&) noexcept = default;
            ContextManager &operator=(ContextManager &&) noexcept = default;

        public:
            MulPtr<Context> acquire(MulPtr<Model> model, ContextParams params);

        private:
            void release(const Str &name, Context *ctx);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_CONTEXT_MANAGER_HPP