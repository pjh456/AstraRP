#ifndef INCLUDE_ASTRA_RP_INFERENCE_BACKEND_HPP
#define INCLUDE_ASTRA_RP_INFERENCE_BACKEND_HPP

#include "utils/types.hpp"

namespace astra_rp
{
    namespace core
    {
        class Model;
        class SamplerChain;
        struct ContextParams;
    }

    namespace infer
    {
        class Session;

        class InferenceBackend
        {
        public:
            virtual ~InferenceBackend() = default;

            virtual ResultV<MulPtr<Session>>
            create_session(
                MulPtr<core::Model> model,
                const core::ContextParams &context_params,
                core::SamplerChain &&sampler,
                int32_t seq_id) = 0;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_INFERENCE_BACKEND_HPP
