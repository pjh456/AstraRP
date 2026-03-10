#include "core/model_manager.hpp"

#include "llama.h"

#include "core/global_config.hpp"
#include "utils/logger.hpp"

namespace astra_rp
{
    namespace core
    {
        ModelManager &ModelManager::instance()
        {
            static ModelManager manager;
            return manager;
        }

        ModelManager::ModelManager()
            : m_init(InitManager::instance())
        {
            ASTRA_LOG_DEBUG("ModelManager initialized.");
        }

        ModelManager::~ModelManager()
        {
            ASTRA_LOG_DEBUG("ModelManager destroying. Cleaning up all models...");

            std::lock_guard<std::mutex> lock(m_mtx);
            m_table.clear();

            ASTRA_LOG_DEBUG("ModelManager destroyed.");
        }

        ResultV<MulPtr<Model>> ModelManager::load(
            const Str &path, ModelParams params)
        {
            ASTRA_LOG_INFO(
                "Request to load model. Name: [" +
                params.m_name +
                "], Path: [" + path + "]");

            std::lock_guard<std::mutex> lock(m_mtx);

            auto &name = params.m_name;
            if (m_table.count(name))
            {
                ASTRA_LOG_WARN(
                    "Model cache hit. Name: [" +
                    name +
                    "] is already loaded. Returning existing instance.");
                return ResultV<MulPtr<Model>>::Ok(m_table[name]);
            }

            ASTRA_LOG_DEBUG(
                "Loading parameters for [" + name + "]: " +
                "n_gpu_layers=" + std::to_string(params.raw().n_gpu_layers) + ", " +
                "use_mmap=" + (params.raw().use_mmap ? "true" : "false") + ", " +
                "use_mlock=" + (params.raw().use_mlock ? "true" : "false") + ", " +
                "vocab_only=" + (params.raw().vocab_only ? "true" : "false"));

            ASTRA_LOG_INFO("Invoking llama_model_load_from_file... This may take a while.");

            auto data =
                llama_model_load_from_file(
                    path.c_str(),
                    params.m_params);
            if (!data)
            {
                ASTRA_LOG_ERROR(
                    "Failed to load model from path: [" +
                    path +
                    "]. "
                    "Possible causes: File not found, corrupted file, invalid format, or Out of Memory.");

                return ResultV<MulPtr<Model>>::Err(
                    utils::ErrorBuilder()
                        .core()
                        .model_load_failed()
                        .message("llama_model_load_from_file failed for path: " + path)
                        .context_id(name)
                        .build());
            }

            auto n_vocab = llama_vocab_n_tokens(llama_model_get_vocab(data));
            auto n_ctx_train = llama_model_n_ctx_train(data);
            auto n_embd = llama_model_n_embd(data);

            ASTRA_LOG_DEBUG(
                "Model metadata [" + name + "]: " +
                "Vocab Size=" + std::to_string(n_vocab) + ", " +
                "Train Context=" + std::to_string(n_ctx_train) + ", " +
                "Embedding Dim=" + std::to_string(n_embd));

            auto model = MulPtr<Model>(new Model(name, data));
            m_table.insert({name, model});

            ASTRA_LOG_INFO(
                "Model [" +
                name +
                "] loaded successfully and registered in manager.");

            return ResultV<MulPtr<Model>>::Ok(model);
        }

        void ModelManager::unload(const Str &name)
        {
            ASTRA_LOG_INFO(
                "Request to unload model: [" +
                name +
                "]");

            std::lock_guard<std::mutex> lock(m_mtx);

            if (!m_table.count(name))
            {
                ASTRA_LOG_WARN(
                    "Unload failed: Model [" +
                    name +
                    "] not found in registry.");
            }

            auto use_count = m_table[name].use_count();
            if (use_count > 1)
            {
                ASTRA_LOG_WARN(
                    "Model [" +
                    name +
                    "] is being removed from manager, but is still referenced by " +
                    std::to_string(use_count - 1) +
                    " other components (e.g., active Sessions). "
                    "Memory will not be freed until they are released.");
            }
            else
            {
                ASTRA_LOG_DEBUG(
                    "Model [" +
                    name +
                    "] has no other references. Memory will be freed immediately.");
            }

            m_table.erase(name);

            ASTRA_LOG_INFO(
                "Model [" +
                name +
                "] unregistered from manager.");
        }

        ResultV<MulPtr<Model>>
        ModelManager::load_config_model()
        {
            if (!GlobalConfigManager::instance().loaded())
            {
                return ResultV<MulPtr<Model>>::Err(
                    utils::ErrorBuilder()
                        .core()
                        .config_load_failed()
                        .message("config file has not loaded yet.")
                        .build());
            }

            const auto &conf =
                GlobalConfigManager::instance()
                    .current();

            return load(
                conf.model_dir,
                std::move(conf.model_params));
        }
    }

}