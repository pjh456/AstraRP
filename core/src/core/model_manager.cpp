#include "core/model_manager.hpp"

#include "llama.h"

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
            : m_init(InitManager::instance()) {}

        ModelManager::~ModelManager() {}

        MulPtr<Model> ModelManager::load(
            const Str &path, ModelParams params)
        {
            std::lock_guard<std::mutex> lock(m_mtx);

            auto &name = params.m_name;
            if (m_table.count(name))
                return nullptr;

            auto data = llama_model_load_from_file(
                path.c_str(), params.m_params);
            if (!data)
                return nullptr;

            auto model = MulPtr<Model>(new Model(name, data));
            m_table.insert({name, model});
            return model;
        }

        void ModelManager::unload(const Str &name)
        {
            std::lock_guard<std::mutex> lock(m_mtx);

            if (!m_table.count(name))
                return;

            m_table.erase(name);
        }
    }
}