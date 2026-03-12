#include "pipeline/inference_node.hpp"

#include "llama.h"

#include "core/model.hpp"
#include "core/model_manager.hpp"
#include "core/sampler_chain.hpp"
#include "core/tokenizer.hpp"
#include "core/global_config.hpp"
#include "infer/engine.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        ResultV<InferenceNode>
        InferenceNode::create(
            const Str &id,
            MulPtr<core::Model> model,
            core::ContextParams cp,
            infer::TokenizeParams tp,
            infer::DecodeParams dp,
            infer::SampleParams sp,
            MulPtr<EventBus> bus)
        {
            auto builder =
                core::SamplerChainBuilder();
            if (!sp.grammar.empty())
                builder.grammar(
                    model,
                    sp.grammar,
                    "");
            if (sp.temperature >= 0)
                builder.temperature(sp.temperature);
            if (sp.top_k >= 0)
                builder.top_k(sp.top_k);
            if (sp.top_p.first >= 0)
                builder.top_p(
                    sp.top_p.first,
                    sp.top_p.second);
            builder.seed(sp.seed);
            ASSIGN_OR_RETURN(
                sampler,
                builder.build());

            ASSIGN_OR_RETURN(
                session,
                infer::Session::create(
                    model, cp,
                    std::move(sampler)));

            infer::GenerationConfig conf;
            conf.add_special = tp.add_special;
            conf.parse_special = tp.parse_special;
            conf.max_tokens = dp.max_tokens;

            InferenceNode node(id, bus, session);
            node.set_config(conf);

            return ResultV<InferenceNode>::Ok(node);
        }

        ResultV<InferenceNode>
        InferenceNode::default_create(
            const Str &id,
            infer::TokenizeParams tp,
            infer::DecodeParams dp,
            infer::SampleParams sp,
            MulPtr<EventBus> bus)
        {
            using InferRes =
                ResultV<InferenceNode>;

            if (!core::GlobalConfigManager::instance().loaded())
                return InferRes::Err(
                    utils::ErrorBuilder()
                        .pipeline()
                        .config_load_failed()
                        .message("Config has not loaded yet.")
                        .build());

            const auto &conf =
                core::GlobalConfigManager::instance()
                    .current();

            auto &cp = conf.context_params;
            ASSIGN_OR_RETURN(
                model,
                core::ModelManager::instance()
                    .load_config_model());

            return create(id, model, cp, tp, dp, sp, bus);
        }

        ResultV<void>
        InferenceNode::execute()
        {
            update_state(NodeState::RUNNING);

            if (m_lora)
                m_session->enable_lora(m_lora, m_lora_scale);

            Str prompt =
                m_prompt_builder
                    ? m_prompt_builder(m_inputs)
                    : "";

            infer::TokenizeTask tok_task(
                m_session->model(),
                prompt,
                {
                    m_config.add_special,
                    m_config.parse_special,
                });
            TRY(tok_task.execute());

            Vec<Token> pending_tokens = tok_task.tokens(); // 初始的 Prompt tokens
            int32_t generated_count = 0;
            auto vocab = llama_model_get_vocab(m_session->model()->raw());

            // === 2. 自回归生成循环 ===
            while (true)
            {
                if (m_abort_flag && m_abort_flag->load())
                {
                    ASTRA_LOG_WARN("Inference aborted by user.");
                    break;
                }

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
    }
}