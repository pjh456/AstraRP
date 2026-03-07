#include "infer/engine.hpp"

#include "llama.h"

#include "utils/logger.hpp"

namespace astra_rp
{
    namespace infer
    {
        Engine &Engine::instance()
        {
            static Engine manager;
            return manager;
        }

        int32_t Engine::decode(
            MulPtr<astra_rp::core::Context> ctx,
            MulPtr<astra_rp::core::Batch> batch)
        {
            return llama_decode(
                ctx->raw(),
                batch->raw());
        }
    }
}