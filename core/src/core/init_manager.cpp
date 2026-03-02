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
            llama_backend_init();
        }

        InitManager::~InitManager()
        {
            llama_backend_free();
        }
    }
}