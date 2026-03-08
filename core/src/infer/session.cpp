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
            astra_rp::core::ContextParams ctx_params,
            astra_rp::core::Sampler &&sampler,
            int32_t seq_id)
            : m_model(model),
              m_ctx(astra_rp::core::ContextManager::
                        instance()
                            .acquire(model, ctx_params)
                            .unwrap()),
              m_sampler(std::move(sampler)),
              m_seq_id(seq_id),
              m_n_past(0),
              m_is_finished(false),
              m_history_tokens()
        {
            auto batch_res = astra_rp::core::BatchManager::instance().acquire(1, 1);
            if (!batch_res.is_ok())
            {
                ASTRA_LOG_ERROR(
                    "Failed to acquire batch for Session initialization: " +
                    batch_res.unwrap_err().message());
                throw std::runtime_error("Failed to acquire batch for Session initialization");
            }
            m_single_batch = batch_res.unwrap();
        }

        Session::~Session() = default;

        void Session::clear()
        {
            m_n_past = 0;
            m_is_finished = false;

            m_history_tokens.clear();
            m_ctx->clear(true);
        }

        void Session::enable_lora(
            MulPtr<astra_rp::core::LoRA> lora,
            float scale)
        {
            if (!lora)
                return;
            m_lora = lora;
            enable_lora(scale);
        }

        void Session::enable_lora(float scale)
        {
            if (!m_lora)
                return;

            if (m_lora_enabled)
                llama_rm_adapter_lora(m_ctx->raw(), m_lora->raw());

            llama_set_adapter_lora(m_ctx->raw(), m_lora->raw(), scale);

            m_lora_enabled = true;
        }

        void Session::disable_lora()
        {
            if (m_lora_enabled && m_lora)
                llama_rm_adapter_lora(m_ctx->raw(), m_lora->raw());
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

            return Tokenizer::tokenize(m_model, prompt)
                .and_then([this](const Vec<Token> &tokens)
                          { return feed_tokens(tokens); });
        }

        ResultV<void> Session::feed_tokens(const Vec<Token> &tokens)
        {
            using namespace astra_rp::core;

            m_history_tokens.reserve(m_history_tokens.size() + tokens.size());

            uint32_t max_batch_size = llama_n_batch(m_ctx->raw());

            ASSIGN_OR_RETURN(batch, BatchManager::instance().acquire(tokens.size(), 1));

            for (size_t i = 0; i < tokens.size(); i += max_batch_size)
            {
                size_t chunk_size =
                    std::min(
                        (size_t)max_batch_size,
                        tokens.size() - i);

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
                                [](auto err)
                                {
                                    ASTRA_LOG_ERROR("Failed to add token to batch: " + err.message());
                                    return err;
                                }));

                    m_n_past++;
                    m_history_tokens.push_back(tokens[token_idx]);
                }

                TRY(Engine::instance()
                        .decode(m_ctx, batch)
                        .map_err(
                            [&](auto err)
                            {
                                ASTRA_LOG_ERROR("Engine decode failed during feed_tokens: " + err.message());
                                return utils::ErrorBuilder()
                                    .infer()
                                    .engine_decode_failed()
                                    .message("Engine decode failed during feed_tokens")
                                    .context_id("SeqID_" + std::to_string(m_seq_id))
                                    .build();
                            }));
            }

            return ResultV<void>::Ok();
        }

        ResultV<Token>
        Session::generate_next()
        {
            using namespace astra_rp::core;

            auto vocab = llama_model_get_vocab(m_model->raw());

            if (m_is_finished)
                return ResultV<Token>::Ok(llama_vocab_eos(vocab));

            auto new_token = m_sampler.sample(*m_ctx, -1);

            if (llama_vocab_is_eog(vocab, new_token))
            {
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
                            ASTRA_LOG_ERROR("Failed to add token to single batch: " + err.message());
                            return err;
                        }));

            TRY(Engine::instance()
                    .decode(m_ctx, m_single_batch)
                    .map_err(
                        [&](auto err)
                        {
                            ASTRA_LOG_ERROR("Engine decode failed during token generation: " + err.message());
                            return utils::ErrorBuilder()
                                .infer()
                                .engine_decode_failed()
                                .message("Engine decode failed during token generation")
                                .context_id("SeqID_" + std::to_string(m_seq_id))
                                .build();
                        }));

            m_n_past++;
            m_history_tokens.push_back(new_token);

            return ResultV<Token>::Ok(new_token);
        }

        ResultV<Str>
        Session::generate(int32_t max_tokens)
        {
            using namespace astra_rp::core;

            int32_t count = 0;
            Vec<Token> newly_generated;

            while (!m_is_finished)
            {
                if (max_tokens > 0 && count >= max_tokens)
                    break;

                ASSIGN_OR_RETURN(
                    token,
                    generate_next()
                        .map_err(
                            [](auto err)
                            {
                                ASTRA_LOG_ERROR("Failed to generate next token: " + err.message());
                                return err;
                            }));

                if (m_is_finished)
                    break;

                newly_generated.push_back(token);
                count++;
            }

            return Tokenizer::detokenize(
                       m_model,
                       newly_generated,
                       false,
                       false)
                .map_err(
                    [](auto err)
                    {
                        ASTRA_LOG_ERROR("Failed to detokenize generated tokens: " + err.message());
                        return err;
                    });
        }
    }
}