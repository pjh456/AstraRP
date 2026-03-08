#ifndef INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP
#define INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP

#include "utils/types.hpp"
#include "pipeline/base_node.hpp"
#include "infer/session.hpp"
#include "core/lora.hpp"
#include "core/tokenizer.hpp"
#include "infer/task.hpp"
#include "infer/engine.hpp"
#include "utils/logger.hpp"

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
            ResultV<void> execute() override
            {
                update_state(NodeState::RUNNING);

                if (m_lora)
                    m_session->enable_lora(m_lora, m_lora_scale);

                Str prompt =
                    m_prompt_builder
                        ? m_prompt_builder(m_inputs)
                        : "";

                ASSIGN_OR_RETURN(
                    task,
                    infer::TaskBuilder(m_session)
                        .prompt(prompt)
                        // .max_tokens(100) // 默认智能推导
                        .on_text_generated(
                            [this](const Str &text)
                            {
                                m_output.output += text;
                                if (m_bus)
                                    m_bus->publish_token(m_id, text);
                                return true;
                            })
                        .on_error(
                            [this](const utils::Error &err)
                            {
                                ASTRA_LOG_ERROR(
                                    "Node Execution failed: " +
                                    err.to_string());
                                if (m_bus)
                                    m_bus->publish_error(m_id, err);
                            })
                        .on_finish(
                            [this]()
                            {
                                if (m_lora)
                                    m_session->disable_lora();
                                update_state(NodeState::FINISHED);
                            })
                        .build()
                        .map_err(
                            [this](auto err)
                            {
                                update_state(NodeState::FAILED);
                                return utils::Failure(
                                    utils::ErrorBuilder()
                                        .infer()
                                        .node_execution_failed()
                                        .wrap(err)
                                        .build());
                            }));

                infer::Engine::instance().submit(task);
                task->wait();

                update_state(NodeState::FINISHED);

                return ResultV<void>::Ok();
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP