#include "core/init_manager.hpp"

#include <string>

#include "llama.h"

#include "utils/logger.hpp"

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
            // 过滤掉 llama.cpp 日志中的换行符
            std::string msg(text);
            if (!msg.empty() && msg.back() == '\n')
                msg.pop_back();

            switch (level)
            {
            case GGML_LOG_LEVEL_ERROR:
                ASTRA_LOG_ERROR("LLAMA: " + msg);
                break;
            case GGML_LOG_LEVEL_WARN:
                ASTRA_LOG_WARN("LLAMA: " + msg);
                break;
            default:
                break; // 忽略其他 INFO/DEBUG
            }
        }
    }
}