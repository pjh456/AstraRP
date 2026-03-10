#ifndef INCLUDE_ASTRA_RP_GLOBAL_CONFIG_HPP
#define INCLUDE_ASTRA_RP_GLOBAL_CONFIG_HPP

#include <mutex>

#include "core/model_params.hpp"
#include "core/context_params.hpp"
#include "utils/types.hpp"

namespace astra_rp
{
    namespace core
    {
        struct GlobalConfigData
        {
            Str model_dir = "./models";
            ModelParams model_params =
                ModelParamsBuilder("default").build();

            ContextParams context_params =
                ContextParamsBuilder().build();

            int32_t max_concurrency = 4; // Scheduler 线程池大小
        };

        class GlobalConfigManager
        {
        private:
            GlobalConfigData m_data;

            std::mutex m_mtx;
            bool m_loaded = false;

        public:
            static GlobalConfigManager &instance()
            {
                static GlobalConfigManager manager;
                return manager;
            }

        private:
            GlobalConfigManager() = default;

        public:
            ~GlobalConfigManager() = default;

            GlobalConfigManager(const GlobalConfigManager &) = delete;
            GlobalConfigManager &operator=(const GlobalConfigManager &) = delete;

            GlobalConfigManager(GlobalConfigManager &&) noexcept = default;
            GlobalConfigManager &operator=(GlobalConfigManager &&) noexcept = default;

        public:
            GlobalConfigData current() noexcept
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                return m_data;
            }

            void update(const GlobalConfigData &data)
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_data = data;
            }

            bool loaded() noexcept
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                return m_loaded;
            }

        public:
            ResultV<void> load_from_file(const Str &path);
            ResultV<void> save_to_file(const Str &path);
        };
    }
}
#endif // INCLUDE_ASTRA_RP_GLOBAL_CONFIG_HPP