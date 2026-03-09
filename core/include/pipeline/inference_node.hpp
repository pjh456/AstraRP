#ifndef INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP
#define INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP

#include "utils/types.hpp"
#include "pipeline/base_node.hpp"
#include "infer/session.hpp"
#include "core/lora.hpp"
#include "core/tokenizer.hpp"
#include "infer/engine.hpp"
#include "infer/tokenize_task.hpp"
#include "infer/decode_task.hpp"
#include "infer/sample_task.hpp"
#include "infer/generation_config.hpp"
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

            infer::GenerationConfig m_config;

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

            void set_config(const infer::GenerationConfig &config)
            {
                m_config = config;
            }

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

                infer::TokenizeTask tok_task(m_session->model(), prompt);
                TRY(tok_task.execute());

                Vec<Token> pending_tokens = tok_task.tokens(); // 初始的 Prompt tokens
                int32_t generated_count = 0;
                auto vocab = llama_model_get_vocab(m_session->model()->raw());

                // === 2. 自回归生成循环 ===
                while (true)
                {
                    // 结束条件检查：达到了用户的 max_tokens 上限
                    if (m_config.max_tokens > 0 && generated_count >= m_config.max_tokens)
                        break;

                    // 结束条件检查：Context 满了
                    if (m_session->n_past() + pending_tokens.size() > m_session->context()->capacity())
                    {
                        ASTRA_LOG_WARN("Context capacity reached. Stopping generation.");
                        break;
                    }

                    // A. 执行 Decode (现在它是异常安全的了)
                    infer::DecodeTask dec_task(m_session, pending_tokens);
                    TRY(dec_task.execute());

                    // Decode 成功后，pending_tokens 已被消化，清空
                    pending_tokens.clear();

                    // B. 执行 Sample
                    infer::SampleTask samp_task(m_session);
                    TRY(samp_task.execute());
                    Token next_token = samp_task.token();

                    // C. 后处理与输出 (这里不写成Task是因为它纯粹是字符串映射)
                    auto str_res = core::Tokenizer::detokenize(m_session->model(), {next_token});
                    if (str_res.is_ok())
                    {
                        Str text = str_res.unwrap();
                        m_output.output += text;
                        if (m_bus)
                            m_bus->publish_token(m_id, text); // 实时推流给前端
                    }

                    // D. 检查 EOS (结束符)
                    if (llama_vocab_is_eog(vocab, next_token))
                        break;

                    // E. 将生成的 Token 作为下一轮的输入
                    pending_tokens.push_back(next_token);
                    generated_count++;
                }

                if (m_lora)
                    m_session->disable_lora();

                update_state(NodeState::FINISHED);
                return ResultV<void>::Ok();
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_INFERENCE_NODE_HPP