#ifndef INCLUDE_ASTRA_RP_SAMPLER_CHAIN_HPP
#define INCLUDE_ASTRA_RP_SAMPLER_CHAIN_HPP

#include <stdexcept>
#include <memory>

#include "utils/types.hpp"
#include "core/model.hpp"
#include "core/context.hpp"

#include "core/sampler_params.hpp"

struct llama_sampler;

namespace astra_rp
{
    namespace core
    {
        class Model;
        class SamplerFactory;

        struct SamplerDeleter
        {
            void operator()(llama_sampler *s) const;
        };

        using UniSampler = std::unique_ptr<llama_sampler, SamplerDeleter>;

        struct SamplerView
        {
            Str name;
            llama_sampler *raw;
        };

        class SamplerFactory
        {
        public:
            static UniSampler greedy();
            static UniSampler seed(uint32_t seed);

        public:
            static UniSampler top_k(int32_t k);
            static UniSampler top_p(float p, size_t min_keep);
            static UniSampler min_p(float p, size_t min_keep);
            static UniSampler adaptive_p(float target, float decay, uint32_t seed);
            static UniSampler temperature(float t);
            static UniSampler grammar(
                MulPtr<Model> model,
                const Str &str,
                const Str &root);
        };

        class SamplerChain
        {
        private:
            llama_sampler *m_chain;
            Vec<SamplerView> m_views;

        public:
            SamplerChain(SamplerParams params);

            SamplerChain();

            ~SamplerChain();

            SamplerChain(const SamplerChain &);
            SamplerChain &operator=(const SamplerChain &);

            SamplerChain(SamplerChain &&) noexcept;
            SamplerChain &operator=(SamplerChain &&) noexcept;

        public:
            llama_sampler *raw() const { return m_chain; }
            const Vec<SamplerView> &views() const noexcept { return m_views; }

        public:
            void push_back(UniSampler sampler, const Str &name);

            Token sample(const Context &ctx, int32_t idx);

        private:
            void free_resource();

            void rebind_views();
        };

        class SamplerChainBuilder
        {
        private:
            SamplerChain m_chain;
            bool m_has_selector = false;

        public:
            SamplerChainBuilder(SamplerChain &&base)
                : m_chain(std::move(base)) {}

            SamplerChainBuilder(SamplerParams params)
                : m_chain(params) {}

            SamplerChainBuilder()
                : m_chain() {}

            ResultV<SamplerChain> build() noexcept
            {
                if (!m_has_selector)
                    return ResultV<SamplerChain>::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .sampler_configuration_error()
                            .message("Sampler chain must end with a selector like greedy() or seed()")
                            .build());
                return ResultV<SamplerChain>::Ok(std::move(m_chain));
            }

        public:
            SamplerChainBuilder &greedy()
            {
                m_chain.push_back(
                    SamplerFactory::greedy(),
                    "greedy");
                m_has_selector = true;
                return *this;
            }

            SamplerChainBuilder &seed(uint32_t seed)
            {
                m_chain.push_back(
                    SamplerFactory::seed(seed),
                    "seed");
                m_has_selector = true;
                return *this;
            }

        public:
            SamplerChainBuilder &top_k(int32_t k)
            {
                m_chain.push_back(
                    SamplerFactory::top_k(k),
                    "top_k");
                return *this;
            }

            SamplerChainBuilder &top_p(float p, size_t min_keep)
            {
                m_chain.push_back(
                    SamplerFactory::top_p(p, min_keep),
                    "top_p");
                return *this;
            }

            SamplerChainBuilder &min_p(float p, size_t min_keep)
            {
                m_chain.push_back(
                    SamplerFactory::min_p(p, min_keep),
                    "min_p");
                return *this;
            }

            SamplerChainBuilder &adaptive_p(
                float target,
                float decay,
                uint32_t seed)
            {
                m_chain.push_back(
                    SamplerFactory::adaptive_p(target, decay, seed),
                    "adaptive_p");
                return *this;
            }

            SamplerChainBuilder &temperature(float t)
            {
                m_chain.push_back(
                    SamplerFactory::temperature(t),
                    "temperature");
                return *this;
            }

            SamplerChainBuilder &grammar(
                MulPtr<Model> model,
                const Str &str,
                const Str &root)
            {
                m_chain.push_back(
                    SamplerFactory::grammar(model, str, root),
                    "grammar");
                return *this;
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SAMPLER_CHAIN_HPP