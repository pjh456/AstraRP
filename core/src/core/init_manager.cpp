#include "core/init_manager.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        InitManager &InitManager::instance()
        {
            static InitManager manager;
            return manager;
        }

        InitManager::InitManager()
        {
            llama_log_set(llama_empty_log_callback, nullptr);

            llama_backend_init();
        }

        InitManager::~InitManager()
        {
            llama_backend_free();
        }

        void InitManager::
            llama_empty_log_callback(
                ggml_log_level level,
                const char *text,
                void *user_data)
        {
            // 直接忽略所有日志内容
            (void)level;
            (void)text;
            (void)user_data;
        }
    }
}