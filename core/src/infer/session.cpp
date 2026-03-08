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

            auto ptr = new Session(model, std::move(sampler), seq_id);
            auto session = MulPtr<Session>(ptr);

            session->m_ctx = ctx;

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
    }
}