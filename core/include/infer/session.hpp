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

            MulPtr<astra_rp::core::LoRA> m_lora = nullptr;
            bool m_lora_enabled = false;

            astra_rp::core::Sampler m_sampler;

            int32_t m_seq_id;
            int32_t m_n_past;
            bool m_is_finished;

            Vec<Token> m_history_tokens;

        public:
            Session(
                MulPtr<astra_rp::core::Model> model,
                astra_rp::core::ContextParams ctx_params,
                astra_rp::core::Sampler &&sampler_params,
                int32_t seq_id = 0);

            ~Session();

        public:
            MulPtr<astra_rp::core::Model> model() const noexcept { return m_model; }
            MulPtr<astra_rp::core::Context> context() const noexcept { return m_ctx; }
            MulPtr<astra_rp::core::LoRA> lora() const noexcept { return m_lora; }

            astra_rp::core::Sampler &sampler() noexcept { return m_sampler; }
            const astra_rp::core::Sampler &sampler() const noexcept { return m_sampler; }

        public:
            void clear();

            void enable_lora(
                MulPtr<astra_rp::core::LoRA> lora,
                float scale = 1.0f);
            void enable_lora(float scale = 1.0f);

            void disable_lora();

        public:
            Vec<uint8_t> export_state() const;
            bool import_state(const Vec<uint8_t> &data);

            bool is_finished() const noexcept { return m_is_finished; }
            int32_t get_n_past() const { return m_n_past; }

        public:
            bool feed_prompt(const Str &prompt);
            bool feed_tokens(const Vec<Token> &tokens);

            Token generate_next();

            Str generate(int32_t max_tokens = -1);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SESSION_HPP