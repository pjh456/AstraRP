#include "core/lora.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        LoRA::LoRA(
            MulPtr<Model> model,
            llama_adapter_lora *adapter)
            : m_model(model),
              m_adapter(adapter) {}

        LoRA::~LoRA()
        {
            if (m_adapter)
            {
                // actually auto free by model.
                // llama_adapter_lora_free(m_adapter);
                m_adapter = nullptr;
            }
        }
    }
}