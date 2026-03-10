#include "core/context_manager.hpp"

#include "core/global_config.hpp"

namespace astra_rp
{
    namespace core
    {
        ContextManager &ContextManager::instance()
        {
            static ContextManager manager;
            return manager;
        }

        ContextManager::ContextManager()
            : m_init(InitManager::instance()) {}

        ResultV<MulPtr<Context>>
        ContextManager::acquire(
            MulPtr<Model> model,
            ContextParams params)
        {
            using CtxRes = ResultV<MulPtr<Context>>;

            std::lock_guard<std::mutex> lock(m_mtx);

            auto conf =
                GlobalConfigManager::instance()
                    .current();

            if (GlobalConfigManager::instance().loaded() &&
                params.metadata().context_size >
                    conf.context_params.metadata().context_size)
            {
                return CtxRes::Err(
                    utils::ErrorBuilder()
                        .system()
                        .out_of_memory()
                        .message("Requested context size exceeds global limits!")
                        .build());
            }

            auto name = model->name();
            auto &pool_list = m_pool[name];
            auto deleter =
                [this, name](Context *c)
            { this->release(name, c); };

            for (auto it = pool_list.begin(); it != pool_list.end(); ++it)
            {

                if ((params.raw().n_ctx != 0) &&
                    ((*it)->capacity() < params.raw().n_ctx))
                    continue;

                auto ctx_ptr = std::move(*it);
                pool_list.erase(it);

                ctx_ptr->clear(true);

                return CtxRes::Ok(
                    MulPtr<Context>(
                        ctx_ptr.release(), deleter));
            }

            auto raw_ctx = llama_init_from_model(model->raw(), params.raw());
            if (!raw_ctx)
                return CtxRes::Err(
                    utils::ErrorBuilder()
                        .core()
                        .context_init_failed()
                        .message("Failed to allocate KV cache / init context (possibly out of memory)")
                        .context_id(model->name())
                        .build());

            auto *new_ctx = new Context(model, raw_ctx);

            return CtxRes::Ok(
                MulPtr<Context>(
                    new_ctx, deleter));
        }

        void ContextManager::release(const Str &name, Context *ctx)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_pool[name].push_back(UniPtr<Context>(ctx));
        }
    }
}