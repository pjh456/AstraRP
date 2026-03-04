#ifndef INCLUDE_ASTRA_RP_INIT_MANAGER_HPP
#define INCLUDE_ASTRA_RP_INIT_MANAGER_HPP

#include "ggml.h"

namespace astra_rp
{
    namespace core
    {
        class InitManager
        {
        public:
            static InitManager &instance();

        private:
            InitManager();
            ~InitManager();

        public:
            InitManager(const InitManager &) = delete;
            InitManager &operator=(const InitManager &) = delete;

            InitManager(InitManager &&) noexcept = default;
            InitManager &operator=(InitManager &&) noexcept = default;

        private:
            static void llama_empty_log_callback(
                ggml_log_level level,
                const char *text,
                void *user_data);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_INIT_MANAGER_HPP