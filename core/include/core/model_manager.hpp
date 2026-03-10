#ifndef INCLUDE_ASTRA_RP_MODEL_MANAGER_HPP
#define INCLUDE_ASTRA_RP_MODEL_MANAGER_HPP

#include <mutex>

#include "utils/types.hpp"
#include "core/model.hpp"
#include "core/model_params.hpp"
#include "core/init_manager.hpp"

namespace astra_rp
{
    namespace core
    {
        class ModelManager
        {
        private:
            HashMap<Str, MulPtr<Model>> m_table;
            std::mutex m_mtx;

        private:
            InitManager &m_init;

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
            ResultV<MulPtr<Model>> load(const Str &path, ModelParams params);
            void unload(const Str &name);

        public:
            ResultV<MulPtr<Model>> load_config_model(ModelParams params);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_MODEL_MANAGER_HPP