#include "infer/session.hpp"

#include <stdexcept>

#include "llama.h"

#include "core/context_manager.hpp"
#include "core/batch_manager.hpp"
#include "core/tokenizer.hpp"
#include "infer/engine.hpp"

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
                            .acquire(model, ctx_params)),
              m_sampler(std::move(sampler)),
              m_seq_id(seq_id),
              m_n_past(0),
              m_is_finished(false),
              m_history_tokens() {}

        Session::~Session() = default;

        void Session::clear()
        {
            m_n_past = 0;
            m_is_finished = false;

            m_history_tokens.clear();
            m_ctx->clear(true);
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

        bool Session::feed_prompt(const Str &prompt)
        {
            using namespace astra_rp::core;

            auto tokens = Tokenizer::tokenize(m_model, prompt);

            return feed_tokens(tokens);
        }

        bool Session::feed_tokens(const Vec<Token> &tokens)
        {
            using namespace astra_rp::core;

            auto batch = BatchManager::instance().acquire(tokens.size(), 1);

            for (size_t i = 0; i < tokens.size(); ++i)
            {
                // last token needs to calculate logits for sampling.
                bool require_logits = (i == tokens.size() - 1);

                batch->add(tokens[i], m_n_past, {m_seq_id}, require_logits);

                m_n_past++;
                m_history_tokens.push_back(tokens[i]);
            }

            auto res = Engine::instance().decode(m_ctx, batch);

            if (res != 0)
            {
                throw std::runtime_error(
                    "Engine decode failed in feed_tokens! Code: " +
                    std::to_string(res));
            }

            return res == 0;
        }

        Token Session::generate_next()
        {
            using namespace astra_rp::core;

            auto vocab = llama_model_get_vocab(m_model->raw());

            if (m_is_finished)
                return llama_vocab_eos(vocab);

            auto new_token = m_sampler.sample(*m_ctx, -1);

            if (llama_vocab_is_eog(vocab, new_token))
            {
                m_is_finished = true;
                return new_token;
            }

            auto batch = BatchManager::instance().acquire(1, 1);
            batch->add(new_token, m_n_past, {m_seq_id}, true);

            auto res = Engine::instance().decode(m_ctx, batch);

            if (res != 0)
            {
                throw std::runtime_error(
                    "Engine decode failed in generate_next! Code: " +
                    std::to_string(res));
            }

            m_n_past++;
            m_history_tokens.push_back(new_token);

            return new_token;
        }

        Str Session::generate(int32_t max_tokens)
        {
            using namespace astra_rp::core;

            int32_t count = 0;
            Vec<Token> newly_generated;

            while (!m_is_finished)
            {
                if (max_tokens > 0 && count >= max_tokens)
                    break;

                Token token = generate_next();

                if (m_is_finished)
                    break;

                newly_generated.push_back(token);
                count++;
            }

            return Tokenizer::detokenize(m_model, newly_generated);
        }
    }
}