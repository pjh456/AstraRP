#ifndef INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP
#define INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP

#include "utils/types.hpp"
#include "pipeline/base_node.hpp"
#include "infer/session.hpp"
#include "core/lora.hpp"
#include "core/tokenizer.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class InferenceNode : public BaseNode
        {
        private:
            MulPtr<astra_rp::infer::Session> m_session;
            MulPtr<astra_rp::core::LoRA> m_lora;
            float m_lora_scale;

            // 在回调前灵活拼接 prompts
            std::function<Str(const HashMap<Str, NodePayload> &)> m_prompt_builder;

        public:
            InferenceNode(
                const Str &id,
                MulPtr<EventBus> bus,
                MulPtr<astra_rp::infer::Session> session)
                : BaseNode(id, bus), m_session(session) {}

            void set_lora(
                MulPtr<astra_rp::core::LoRA> lora,
                float scale = 1.0f)
            {
                m_lora = lora;
                m_lora_scale = scale;
            }

            void set_prompt_builder(auto builder) { m_prompt_builder = builder; }

        public:
            bool execute() override
            {
                update_state(NodeState::RUNNING);

                try
                {
                    if (m_lora)
                        m_session->enable_lora(m_lora, m_lora_scale);

                    Str prompt = m_prompt_builder ? m_prompt_builder(m_inputs) : "";
                    m_session->feed_prompt(prompt);

                    Str full_text;
                    while (!m_session->is_finished())
                    {
                        auto token = m_session->generate_next();
                        Str token_str =
                            astra_rp::core::Tokenizer::
                                detokenize(m_session->model(), {token});

                        full_text += token_str;
                        if (m_bus)
                            m_bus->publish_token(m_id, token_str);
                    }

                    m_output.output = full_text;

                    m_session->disable_lora();
                    m_session->clear();

                    update_state(NodeState::FINISHED);
                    return true;
                }
                catch (const std::exception &e)
                {
                    if (m_bus)
                        m_bus->publish_error(m_id, e.what());
                    update_state(NodeState::FAILED);
                    return false;
                }
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP