#ifndef INCLUDE_ASTRA_RP_LORA_HPP
#define INCLUDE_ASTRA_RP_LORA_HPP

#include "utils/types.hpp"
#include "core/model.hpp"

struct llama_adapter_lora;

namespace astra_rp
{
    namespace core
    {
        class LoRAManager;

        class LoRA
        {
            friend class LoRAManager;

        private:
            llama_adapter_lora *m_adapter;
            MulPtr<Model> m_model;

        private:
            LoRA(MulPtr<Model> model, llama_adapter_lora *adapter = nullptr);

        public:
            ~LoRA();

            LoRA(const LoRA &) = delete;
            LoRA &operator=(const LoRA &) = delete;

            LoRA(LoRA &&) noexcept = default;
            LoRA &operator=(LoRA &&) noexcept = default;

        public:
            llama_adapter_lora *raw() const noexcept { return m_adapter; }
            MulPtr<Model> model() const noexcept { return m_model; }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_LORA_HPP