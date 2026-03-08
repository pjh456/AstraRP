#include "infer/engine.hpp"

#include "llama.h"

#include "core/model.hpp"
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

        ResultV<void>
        Engine::decode(
            MulPtr<astra_rp::core::Context> ctx,
            MulPtr<astra_rp::core::Batch> batch)
        {
            auto res = llama_decode(ctx->raw(), batch->raw());
            if (res != 0)
            {
                if (res == 1)
                {
                    ASTRA_LOG_ERROR(
                        "Engine decode failed: Batch capacity exceeded. Context model: " +
                        ctx->model().name());
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .engine_decode_failed()
                            .message("Batch capacity exceeded. Consider increasing the batch size or reducing the number of tokens.")
                            .context_id(ctx->model().name())
                            .build());
                }
                else if (res == 2)
                {
                    ASTRA_LOG_ERROR(
                        "Engine decode failed: Context state invalid. Context model: " +
                        ctx->model().name());
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .engine_decode_failed()
                            .message("Context state invalid. This may be caused by exceeding the context capacity or corrupted state. Consider clearing the context or reducing the number of tokens.")
                            .context_id(ctx->model().name())
                            .build());
                }
                else
                {
                    ASTRA_LOG_ERROR(
                        "Engine decode failed with error code: " +
                        std::to_string(res) +
                        ". Context model: " +
                        ctx->model().name());
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .engine_decode_failed()
                            .message("Engine decode failed with error code: " + std::to_string(res))
                            .context_id(ctx->model().name())
                            .build());
                }
            }
            return ResultV<void>::Ok();
        }
    }
}