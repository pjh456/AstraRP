#ifndef INCLUDE_ASTRA_RP_MODEL_MANAGER_HPP
#define INCLUDE_ASTRA_RP_MODEL_MANAGER_HPP

#include <mutex>

#include "utils/types.hpp"
#include "core/init_manager.hpp"

namespace astra_rp
{
    namespace core
    {
        class Model;
        class ModelParams;

        class ModelManager
        {
        private:
            HashMap<Str, MulPtr<Model>> m_table;
            std::mutex m_mtx;

        private:
            InitManager &m_init_manager;

        public:
            static ModelManager &instance();

        private:
            ModelManager();
            ~ModelManager();

        public:
            ModelManager(const ModelManager &) = delete;
            ModelManager &operator=(const ModelManager &) = delete;

            ModelManager(ModelManager &&) noexcept = default;
            ModelManager &operator=(ModelManager &&) noexcept = default;

        public:
            MulPtr<Model> load(const Str &path, ModelParams params);
            void unload(const Str &name);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_MODEL_MANAGER_HPP