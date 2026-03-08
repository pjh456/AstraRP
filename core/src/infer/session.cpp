#include "infer/session.hpp"

#include <stdexcept>

#include "llama.h"

#include "core/context_manager.hpp"
#include "core/batch_manager.hpp"
#include "core/tokenizer.hpp"
#include "infer/engine.hpp"

#include "utils/logger.hpp"

namespace astra_rp
{
    namespace infer
    {
        Session::Session(
            MulPtr<astra_rp::core::Model> model,
            astra_rp::core::Sampler &&sampler,
            int32_t seq_id)
            : m_model(model),
              m_ctx(nullptr),
              m_single_batch(nullptr),
              m_sampler(std::move(sampler)),
              m_seq_id(seq_id),
              m_n_past(0),
              m_is_finished(false),
              m_history_tokens() {}

        ResultV<MulPtr<Session>>
        Session::create(
            MulPtr<astra_rp::core::Model> model,
            astra_rp::core::ContextParams ctx_params,
            astra_rp::core::Sampler &&sampler,
            int32_t seq_id)
        {
            using namespace astra_rp::core;

            ASTRA_LOG_INFO(
                "Creating Inference Session. Model: [" +
                model->name() +
                "], SeqID: " +
                std::to_string(seq_id));

            ASTRA_LOG_DEBUG(
                "Context Params: "
                "n_ctx=" +
                std::to_string(ctx_params.raw().n_ctx) +
                ", "
                "n_batch=" +
                std::to_string(ctx_params.raw().n_batch) +
                ", "
                "n_threads=" +
                std::to_string(ctx_params.raw().n_threads) +
                ", "
                "n_threads_batch=" +
                std::to_string(ctx_params.raw().n_threads_batch) +
                ", "
                "flash_attn=" +
                (ctx_params.raw().flash_attn_type == 1
                     ? "Enabled"
                     : "Disabled"));

            ASSIGN_OR_RETURN(
                ctx,
                ContextManager::instance()
                    .acquire(model, ctx_params));

            ASTRA_LOG_INFO(
                "Context acquired. Actual capacity: " +
                std::to_string(ctx->capacity()));

            ASSIGN_OR_RETURN(
                batch,
                BatchManager::instance()
                    .acquire(1, 1));

            auto ptr = new Session(model, std::move(sampler), seq_id);
            auto session = MulPtr<Session>(ptr);

            session->m_ctx = ctx;
            session->m_single_batch = batch;

            ASTRA_LOG_INFO("Session created successfully.");

            return ResultV<MulPtr<Session>>::Ok(session);
        }

        Session::~Session() = default;

        void Session::clear()
        {
            ASTRA_LOG_INFO(
                "Clearing Session state. Resetting n_past from " +
                std::to_string(m_n_past) +
                " to 0.");

            m_n_past = 0;
            m_is_finished = false;

            m_history_tokens.clear();
            m_ctx->clear(true);

            ASTRA_LOG_DEBUG("KV Cache cleared.");
        }

        void Session::enable_lora(
            MulPtr<astra_rp::core::LoRA> lora,
            float scale)
        {
            if (!lora)
            {
                ASTRA_LOG_WARN(
                    "Attempted to enable LoRA, but no LoRA adapter is attached to this session.");

                return;
            }

            m_lora = lora;
            enable_lora(scale);
        }

        void Session::enable_lora(float scale)
        {
            if (!m_lora)
            {
                ASTRA_LOG_WARN(
                    "Attempted to enable LoRA, but no LoRA adapter is attached to this session.");

                return;
            }

            if (m_lora_enabled)
                llama_rm_adapter_lora(m_ctx->raw(), m_lora->raw());

            ASTRA_LOG_INFO(
                "Enabling LoRA with scale: " +
                std::to_string(scale));

            llama_set_adapter_lora(m_ctx->raw(), m_lora->raw(), scale);

            m_lora_enabled = true;
        }

        void Session::disable_lora()
        {
            if (m_lora_enabled && m_lora)
            {
                ASTRA_LOG_INFO("Disabling LoRA adapter.");

                llama_rm_adapter_lora(
                    m_ctx->raw(),
                    m_lora->raw());
            }
            m_lora_enabled = false;
        }

        Vec<uint8_t> Session::export_state() const
        {
            return m_ctx->get_state(m_seq_id);
        }

        bool Session::import_state(const Vec<uint8_t> &data)
        {
            size_t written = m_ctx->set_state(data, m_seq_id);
            return written > 0;
        }

        ResultV<void> Session::feed_prompt(const Str &prompt)
        {
            using namespace astra_rp::core;

            ASTRA_LOG_INFO(
                "Feeding prompt. Length: " +
                std::to_string(prompt.size()) +
                " chars.");

            return Tokenizer::tokenize(m_model, prompt)
                .and_then(
                    [this](const Vec<Token> &tokens)
                    {
                        ASTRA_LOG_DEBUG(
                            "Prompt tokenized into " +
                            std::to_string(tokens.size()) +
                            " tokens.");
                        return feed_tokens(tokens);
                    })
                .map_err(
                    [&](auto err)
                    {
                        ASTRA_LOG_ERROR(
                            "Failed to tokenize prompt: " +
                            err.message());
                        return utils::ErrorBuilder()
                            .infer()
                            .tokenize_failed()
                            .message("Failed to tokenize prompt")
                            .context_id("SeqID_" + std::to_string(m_seq_id))
                            .wrap(std::move(err))
                            .build();
                    });
        }

        ResultV<void> Session::feed_tokens(const Vec<Token> &tokens)
        {
            using namespace astra_rp::core;

            if (tokens.empty())
            {
                ASTRA_LOG_WARN(
                    "feed_tokens called with empty token vector.");
                return ResultV<void>::Ok();
            }

            m_history_tokens.reserve(m_history_tokens.size() + tokens.size());

            auto max_batch_size = llama_n_batch(m_ctx->raw());

            ASTRA_LOG_DEBUG(
                "Processing " +
                std::to_string(tokens.size()) +
                " tokens. Max batch size: " +
                std::to_string(max_batch_size));

            ASSIGN_OR_RETURN(
                batch,
                BatchManager::instance()
                    .acquire(tokens.size(), 1)
                    .map_err(
                        [&](auto err)
                        {
                            ASTRA_LOG_ERROR(
                                "Failed to acquire batch for feeding tokens: " +
                                err.message());
                            return utils::ErrorBuilder()
                                .infer()
                                .batch_capacity_exceeded()
                                .message(
                                    "Failed to acquire batch for feeding tokens. Required capacity: " +
                                    std::to_string(tokens.size()))
                                .context_id("SeqID_" + std::to_string(m_seq_id))
                                .wrap(std::move(err))
                                .build();
                        }));

            int chunks = 0;
            for (size_t i = 0; i < tokens.size(); i += max_batch_size)
            {
                chunks++;

                size_t chunk_size =
                    std::min(
                        (size_t)max_batch_size,
                        tokens.size() - i);

                ASTRA_LOG_DEBUG(
                    "Preparing batch chunk " +
                    std::to_string(chunks) +
                    ", size: " +
                    std::to_string(chunk_size));

                for (size_t j = 0; j < chunk_size; ++j)
                {
                    size_t token_idx = i + j;

                    // last token needs to calculate logits for sampling.
                    bool require_logits = (token_idx == tokens.size() - 1);

                    TRY(batch->add(
                                 tokens[token_idx],
                                 m_n_past,
                                 {m_seq_id},
                                 require_logits)
                            .map_err(
                                [&](auto err)
                                {
                                    ASTRA_LOG_ERROR(
                                        "Batch add token failed at index " +
                                        std::to_string(token_idx));
                                    return utils::ErrorBuilder()
                                        .infer()
                                        .batch_capacity_exceeded()
                                        .message("Failed to add token to batch")
                                        .wrap(std::move(err))
                                        .build();
                                }));

                    m_n_past++;
                    m_history_tokens.push_back(tokens[token_idx]);
                }

                TRY(Engine::instance()
                        .decode(m_ctx, batch)
                        .map_err(
                            [&](auto err)
                            {
                                ASTRA_LOG_ERROR(
                                    "Engine decode failed during feed_tokens: " +
                                    err.message());
                                return utils::ErrorBuilder()
                                    .infer()
                                    .engine_decode_failed()
                                    .message("Engine decode failed during feed_tokens")
                                    .context_id("SeqID_" + std::to_string(m_seq_id))
                                    .wrap(std::move(err))
                                    .build();
                            }));
            }

            ASTRA_LOG_INFO(
                "Feed tokens complete. Processed in " +
                std::to_string(chunks) +
                " chunks. New n_past: " +
                std::to_string(m_n_past));

            return ResultV<void>::Ok();
        }

        ResultV<Token>
        Session::generate_next()
        {
            using namespace astra_rp::core;

            auto vocab = llama_model_get_vocab(m_model->raw());

            if (m_is_finished)
            {
                ASTRA_LOG_WARN("generate_next called on a finished session.");
                return ResultV<Token>::Ok(llama_vocab_eos(vocab));
            }

            ASTRA_LOG_DEBUG("Sampling next token...");
            auto new_token = m_sampler.sample(*m_ctx, -1);

            if (llama_vocab_is_eog(vocab, new_token))
            {
                ASTRA_LOG_INFO(
                    "End of generation detected (EOS/EOG). Token ID: " +
                    std::to_string(new_token));

                m_is_finished = true;
                return ResultV<Token>::Ok(new_token);
            }

            m_single_batch->clear();
            TRY(m_single_batch
                    ->add(
                        new_token, m_n_past,
                        {m_seq_id},
                        true)
                    .map_err(
                        [](auto err)
                        {
                            ASTRA_LOG_ERROR(
                                "Failed to add token to single batch: " +
                                err.message());
                            return utils::ErrorBuilder()
                                .infer()
                                .batch_capacity_exceeded()
                                .message("Failed to add token to single batch")
                                .wrap(std::move(err))
                                .build();
                        }));

            TRY(Engine::instance()
                    .decode(m_ctx, m_single_batch)
                    .map_err(
                        [&](auto err)
                        {
                            ASTRA_LOG_ERROR(
                                "Engine decode failed during token generation: " +
                                err.message());
                            return utils::ErrorBuilder()
                                .infer()
                                .engine_decode_failed()
                                .message("Engine decode failed during token generation")
                                .context_id("SeqID_" + std::to_string(m_seq_id))
                                .wrap(std::move(err))
                                .build();
                        }));

            m_n_past++;
            m_history_tokens.push_back(new_token);

            ASTRA_LOG_INFO("Token Generation finished.");

            return ResultV<Token>::Ok(new_token);
        }

        ResultV<Str>
        Session::generate(int32_t max_tokens)
        {
            using namespace astra_rp::core;

            ASTRA_LOG_INFO(
                "Starting generation loop. Max tokens: " +
                std::to_string(max_tokens));

            int32_t count = 0;
            Vec<Token> newly_generated;

            while (!m_is_finished)
            {
                if (max_tokens > 0 && count >= max_tokens)
                {
                    ASTRA_LOG_INFO(
                        "Max token limit reached (" +
                        std::to_string(max_tokens) +
                        "). Stopping.");
                    break;
                }

                ASSIGN_OR_RETURN(
                    token,
                    generate_next()
                        .map_err(
                            [&](auto err)
                            {
                                ASTRA_LOG_ERROR(
                                    "Failed to generate next token: " +
                                    err.message());
                                return utils::ErrorBuilder()
                                    .infer()
                                    .engine_decode_failed()
                                    .message("Failed to generate next token")
                                    .context_id("SeqID_" + std::to_string(m_seq_id))
                                    .wrap(std::move(err))
                                    .build();
                            }));

                if (m_is_finished)
                    break;

                newly_generated.push_back(token);
                count++;
            }

            ASTRA_LOG_INFO(
                "Generation finished. Tokens generated: " +
                std::to_string(count));

            return Tokenizer::detokenize(
                       m_model,
                       newly_generated,
                       false,
                       false)
                .map_err(
                    [&](auto err)
                    {
                        ASTRA_LOG_ERROR(
                            "Failed to detokenize generated tokens: " +
                            err.message());
                        return utils::ErrorBuilder()
                            .infer()
                            .detokenize_failed()
                            .message("Failed to detokenize generated tokens")
                            .context_id("SeqID_" + std::to_string(m_seq_id))
                            .wrap(std::move(err))
                            .build();
                    });
        }
    }
}