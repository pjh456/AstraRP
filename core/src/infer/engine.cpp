#include "infer/engine.hpp"

#include "llama.h"

#include "core/model.hpp"
#include "core/batch.hpp"
#include "core/context.hpp"
#include "infer/session.hpp"
#include "core/tokenizer.hpp"
#include "infer/context_transaction.hpp"
#include "core/batch_manager.hpp"
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

        ResultV<Token>
        Engine::char2token(
            MulPtr<core::Model> model,
            char ch)
        {
            return str2token(
                       model,
                       Str(1, ch))
                .map([](auto vec)
                     { return vec[0]; });
        }

        ResultV<Vec<Token>>
        Engine::str2token(
            MulPtr<core::Model> model,
            const Str &str)
        {
            using TokenRes = ResultV<Vec<Token>>;

            if (str.empty())
            {
                return TokenRes::Ok(Vec<Token>{});
            }

            ASSIGN_OR_RETURN(
                tokens,
                core::Tokenizer::tokenize(model, str)
                    .map_err(
                        [&model](auto err)
                        {
                            return utils::ErrorBuilder()
                                .infer()
                                .tokenize_failed()
                                .message("Tokenize failed!")
                                .context_id(model->name())
                                .wrap(std::move(err))
                                .build();
                        }));

            return TokenRes::Ok(std::move(tokens));
        }

        ResultV<std::pair<MulPtr<core::Batch>, size_t>>
        Engine::tok2batch(
            MulPtr<infer::Session> session,
            const Vec<Token> &tokens)
        {
            using BatchRes = ResultV<std::pair<MulPtr<core::Batch>, size_t>>;

            auto ctx = session->context();

            auto max_batch_size =
                llama_n_batch(ctx->raw());
            auto tok_size = tokens.size();
            size_t chunk_size =
                std::min(
                    (size_t)max_batch_size,
                    tok_size);

            if (max_batch_size == 0)
            {
                return BatchRes::Err(
                    utils::ErrorBuilder()
                        .pipeline()
                        .tokenize_failed()
                        .message("No tokens to be processed or batch size == 0!")
                        .build());
            }

            ASSIGN_OR_RETURN(
                batch,
                core::BatchManager::instance()
                    .acquire(chunk_size)
                    .map_err(
                        [](auto err)
                        {
                            return utils::ErrorBuilder()
                                .pipeline()
                                .batch_acquire_failed()
                                .message("Batch acquire failed!")
                                .wrap(std::move(err))
                                .build();
                        }));

            for (size_t i = 0; i < chunk_size; ++i)
            {
                bool require_logits = (i == tok_size - 1);
                auto &tok = tokens[i];

                TRY(batch->add(
                             tok,
                             session->n_past(),
                             {session->seq_id()},
                             require_logits)
                        .map_err(
                            [](auto err)
                            {
                                return utils::ErrorBuilder()
                                    .pipeline()
                                    .batch_add_failed()
                                    .message("Batch add failed!")
                                    .wrap(std::move(err))
                                    .build();
                            }));
                session->advance_past(1);
                session->add_history(tok);
            }

            return BatchRes::Ok(
                std::make_pair(
                    std::move(batch),
                    chunk_size));
        }

        ResultV<Vec<MulPtr<core::Batch>>>
        Engine::all_tok2batch(
            MulPtr<infer::Session> session,
            const Vec<Token> &tokens)
        {
            using BatchRes = ResultV<Vec<MulPtr<core::Batch>>>;

            ContextTransaction tx(session);

            auto ctx = session->context();

            auto max_batch_size =
                llama_n_batch(ctx->raw());
            auto tok_size = tokens.size();
            size_t chunk_size =
                std::min(
                    (size_t)max_batch_size,
                    tok_size);

            if (max_batch_size == 0)
            {
                return BatchRes::Err(
                    utils::ErrorBuilder()
                        .pipeline()
                        .tokenize_failed()
                        .message("No tokens to be processed or batch size == 0!")
                        .build());
            }

            Vec<MulPtr<core::Batch>> batches;
            batches.reserve(tok_size / chunk_size + 1);

            ASSIGN_OR_RETURN(
                batch,
                core::BatchManager::instance()
                    .acquire(chunk_size, 1)
                    .map_err(
                        [&](auto err)
                        {
                            return utils::ErrorBuilder()
                                .pipeline()
                                .batch_acquire_failed()
                                .message("Batch acquire failed!")
                                .wrap(std::move(err))
                                .build();
                        }));

            for (size_t i = 0; i < tok_size; i += chunk_size)
            {
                batch->clear();
                for (size_t j = 0; j < chunk_size; ++j)
                {
                    auto idx = i + j;
                    bool require_logits = (idx == tok_size - 1);
                    auto &tok = tokens[idx];

                    TRY(batch->add(
                                 tok,
                                 session->n_past(),
                                 {session->seq_id()},
                                 require_logits)
                            .map_err(
                                [](auto err)
                                {
                                    return utils::ErrorBuilder()
                                        .pipeline()
                                        .batch_add_failed()
                                        .message("Batch add failed!")
                                        .wrap(std::move(err))
                                        .build();
                                }));
                    session->advance_past(1);
                    session->add_history(tok);
                }

                batches.push_back(std::move(batch));
            }

            tx.commit();
            return BatchRes::Ok(std::move(batches));
        }

        ResultV<void>
        Engine::decode(
            MulPtr<Session> session,
            MulPtr<core::Batch> batch)
        {
            auto ctx = session->context();
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

        ResultV<void>
        Engine::decode(
            MulPtr<Session> session,
            Vec<MulPtr<core::Batch>> batches)
        {
            ContextTransaction tx(session);

            for (auto &batch : batches)
            {
                TRY(decode(session, batch)
                        .map_err(
                            [&session](auto err)
                            {
                                return utils::ErrorBuilder()
                                    .infer()
                                    .engine_decode_failed()
                                    .message("Engine decode Failed!")
                                    .context_id(
                                        session
                                            ->context()
                                            ->model()
                                            .name())
                                    .wrap(std::move(err))
                                    .build();
                            }));
            }

            return ResultV<void>::Ok();
        }

        ResultV<Token>
        Engine::sample(
            MulPtr<core::Context> ctx,
            const core::SamplerChain &sampler)
        {
            return ResultV<Token>::Ok(
                sampler.sample(ctx, -1));
        }
    }
}