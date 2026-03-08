#ifndef INCLUDE_ASTRA_RP_SESSION_HPP
#define INCLUDE_ASTRA_RP_SESSION_HPP

#include "utils/types.hpp"
#include "core/model.hpp"
#include "core/context_params.hpp"
#include "core/lora.hpp"
#include "core/context.hpp"
#include "core/sampler.hpp"
#include "core/batch_manager.hpp"

namespace astra_rp
{
    namespace infer
    {
        class Session
        {
        private:
            MulPtr<astra_rp::core::Model> m_model;
            MulPtr<astra_rp::core::Context> m_ctx;
            astra_rp::core::Sampler m_sampler;
            MulPtr<astra_rp::core::LoRA> m_lora = nullptr;

            int32_t m_seq_id;
            int32_t m_n_past;
            bool m_lora_enabled = false;
            bool m_is_finished;

            Vec<Token> m_history_tokens;

        private:
            Session(
                MulPtr<astra_rp::core::Model> model,
                astra_rp::core::Sampler &&sampler,
                int32_t seq_id);

        public:
            static ResultV<MulPtr<Session>>
            create(
                MulPtr<astra_rp::core::Model> model,
                astra_rp::core::ContextParams ctx_params,
                astra_rp::core::Sampler &&sampler,
                int32_t seq_id = 0);

            ~Session();

        public:
            MulPtr<astra_rp::core::Model> model() const noexcept { return m_model; }
            MulPtr<astra_rp::core::Context> context() const noexcept { return m_ctx; }
            MulPtr<astra_rp::core::LoRA> lora() const noexcept { return m_lora; }
            astra_rp::core::Sampler &sampler() noexcept { return m_sampler; }

            int32_t seq_id() const noexcept { return m_seq_id; }
            int32_t n_past() const noexcept { return m_n_past; }
            bool is_finished() const noexcept { return m_is_finished; }
            const Vec<Token> &history() const noexcept { return m_history_tokens; }

            void advance_past(int32_t step) { m_n_past += step; }
            void add_history(Token t) { m_history_tokens.push_back(t); }
            void set_finished(bool flag) { m_is_finished = flag; }

        public:
            void clear();

            void enable_lora(
                MulPtr<astra_rp::core::LoRA> lora,
                float scale = 1.0f);
            void enable_lora(float scale = 1.0f);
            void disable_lora();

            Vec<uint8_t> export_state() const;
            bool import_state(const Vec<uint8_t> &data);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SESSION_HPP