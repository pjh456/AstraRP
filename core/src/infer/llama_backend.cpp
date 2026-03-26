#include "infer/llama_backend.hpp"

#include "infer/session.hpp"

namespace astra_rp
{
    namespace infer
    {
        ResultV<MulPtr<Session>>
        LlamaBackend::create_session(
            MulPtr<core::Model> model,
            const core::ContextParams &context_params,
            core::SamplerChain &&sampler,
            int32_t seq_id)
        {
            return Session::create(
                model,
                context_params,
                std::move(sampler),
                seq_id);
        }
    }
}
