#ifndef INCLUDE_ASTRA_RP_ENGINE_HPP
#define INCLUDE_ASTRA_RP_ENGINE_HPP

#include <mutex>
#include <condition_variable>
#include <thread>
#include <utility>

#include "utils/types.hpp"
#include "core/sampler_chain.hpp"

namespace astra_rp
{
    namespace core
    {
        class Batch;
        class Context;
    }

    namespace infer
    {
        class Session;

        struct TokenizeParams
        {
            bool add_special = true;
            bool parse_special = true;
        };

        class Engine
        {
        public:
            static Engine &instance();

        private:
            Engine() = default;

        public:
            ~Engine() = default;

            Engine(const Engine &) = delete;
            Engine &operator=(const Engine &) = delete;

            Engine(Engine &&) noexcept = default;
            Engine &operator=(Engine &&) noexcept = default;

        public:
            ResultV<Token>
            char2token(
                MulPtr<core::Model> model,
                char ch,
                TokenizeParams params = {});

            ResultV<Vec<Token>>
            str2token(
                MulPtr<core::Model> model,
                const Str &str,
                TokenizeParams params = {});

            ResultV<std::pair<MulPtr<core::Batch>, size_t>>
            tok2batch(
                MulPtr<Session> session,
                const Vec<Token> &tokens);

            ResultV<Vec<MulPtr<core::Batch>>>
            all_tok2batch(
                MulPtr<Session> session,
                const Vec<Token> &tokens);

            ResultV<void> decode(
                MulPtr<Session> session,
                MulPtr<core::Batch> batch);

            ResultV<void> decode(
                MulPtr<Session> session,
                Vec<MulPtr<core::Batch>> batch);

            ResultV<Token> sample(
                MulPtr<core::Context> ctx,
                const core::SamplerChain &sampler);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_ENGINE_HPP