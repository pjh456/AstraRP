#ifndef INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP
#define INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP

#include "utils/types.hpp"
#include "pipeline/base_node.hpp"
#include "infer/inference_backend.hpp"
#include "infer/session.hpp"
#include "core/lora.hpp"
#include "infer/tokenize_task.hpp"
#include "infer/decode_task.hpp"
#include "infer/sample_task.hpp"
#include "infer/generation_config.hpp"
#include "infer/params.hpp"
#include "utils/logger.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class InferenceNode : public BaseNode
        {
        private:
            MulPtr<infer::Session> m_session;
            MulPtr<core::LoRA> m_lora;
            float m_lora_scale;

            infer::GenerationConfig m_config;

            // 在回调前灵活拼接 prompts
            std::function<Str(const HashMap<Str, NodePayload> &)> m_prompt_builder;
            MulPtr<infer::InferenceBackend> m_backend;

        public:
            InferenceNode(
                const Str &id,
                MulPtr<EventBus> bus,
                MulPtr<infer::Session> session,
                MulPtr<infer::InferenceBackend> backend = nullptr)
                : BaseNode(id, bus),
                  m_session(session),
                  m_backend(backend) {}

            void set_lora(
                MulPtr<core::LoRA> lora,
                float scale = 1.0f)
            {
                m_lora = lora;
                m_lora_scale = scale;
            }

            void set_prompt_builder(auto builder) { m_prompt_builder = builder; }

            void set_config(const infer::GenerationConfig &config)
            {
                m_config = config;
            }

            void set_backend(MulPtr<infer::InferenceBackend> backend)
            {
                m_backend = backend;
            }

        public:
            static ResultV<InferenceNode>
            create(
                const Str &id,
                MulPtr<core::Model> model,
                core::ContextParams cp,
                infer::TokenizeParams tp,
                infer::DecodeParams dp,
                infer::SampleParams sp,
                MulPtr<EventBus> bus = nullptr,
                MulPtr<infer::InferenceBackend> backend = nullptr);

            static ResultV<InferenceNode>
            default_create(
                const Str &id,
                infer::TokenizeParams tp,
                infer::DecodeParams dp,
                infer::SampleParams sp,
                MulPtr<EventBus> bus = nullptr,
                MulPtr<infer::InferenceBackend> backend = nullptr);

        public:
            ResultV<void> execute() override;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP
