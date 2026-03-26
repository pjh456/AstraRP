#ifndef INCLUDE_ASTRA_RP_LLAMA_BACKEND_HPP
#define INCLUDE_ASTRA_RP_LLAMA_BACKEND_HPP

#include "infer/inference_backend.hpp"

namespace astra_rp
{
    namespace infer
    {
        class LlamaBackend final : public InferenceBackend
        {
        public:
            ResultV<MulPtr<Session>>
            create_session(
                MulPtr<core::Model> model,
                const core::ContextParams &context_params,
                core::SamplerChain &&sampler,
                int32_t seq_id) override;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_LLAMA_BACKEND_HPP
