#ifndef INCLUDE_ASTRA_RP_LORA_MANAGER_HPP
#define INCLUDE_ASTRA_RP_LORA_MANAGER_HPP

#include <mutex>

#include "utils/types.hpp"
#include "core/lora.hpp"

namespace astra_rp
{
    namespace core
    {
        class LoRAManager
        {
        private:
            HashMap<Str, MulPtr<LoRA>> m_table;

            std::mutex m_mtx;

        public:
            static LoRAManager &instance();

        private:
            LoRAManager() = default;

        public:
            ~LoRAManager() = default;

            LoRAManager(const LoRAManager &) = delete;
            LoRAManager &operator=(const LoRAManager &) = delete;

            LoRAManager(LoRAManager &&) noexcept = default;
            LoRAManager &operator=(LoRAManager &&) noexcept = default;

        public:
            ResultV<MulPtr<LoRA>>
            load(
                MulPtr<Model> model,
                const Str &path,
                const Str &name);
            void unload(const Str &name);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_LORA_MANAGER_HPP