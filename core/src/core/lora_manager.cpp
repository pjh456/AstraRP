#include "core/lora_manager.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        LoRAManager &LoRAManager::instance()
        {
            static LoRAManager manager;
            return manager;
        }

        MulPtr<LoRA> LoRAManager::load(
            MulPtr<Model> model,
            const Str &path,
            const Str &name)
        {
            std::lock_guard<std::mutex> lock(m_mtx);

            Str key = model->name() + "::" + name;
            if (m_table.count(key))
                return m_table[key];

            auto raw_adapter =
                llama_adapter_lora_init(model->raw(), path.c_str());
            if (!raw_adapter)
                return nullptr;

            auto lora_ptr = new LoRA(model, raw_adapter);
            auto lora = MulPtr<LoRA>(lora_ptr);

            m_table.insert({key, lora});
            return lora;
        }

        void LoRAManager::unload(const Str &name)
        {
            std::lock_guard<std::mutex> lock(m_mtx);

            if (!m_table.count(name))
                return;

            m_table.erase(name);
        }
    }
}