#include "core/sampler_chain.hpp"
#include "core/model.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        void SamplerDeleter::operator()(llama_sampler *s) const
        {
            if (s)
            {
                llama_sampler_free(s);
            }
        }

        UniSampler SamplerFactory::greedy()
        {
            return UniSampler(llama_sampler_init_greedy());
        }

        UniSampler SamplerFactory::seed(uint32_t seed)
        {
            return UniSampler(llama_sampler_init_dist(seed));
        }

        UniSampler SamplerFactory::top_k(int32_t k)
        {
            return UniSampler(llama_sampler_init_top_k(k));
        }

        UniSampler SamplerFactory::top_p(float p, size_t min_keep)
        {
            return UniSampler(llama_sampler_init_top_p(p, min_keep));
        }

        UniSampler SamplerFactory::min_p(float p, size_t min_keep)
        {
            return UniSampler(llama_sampler_init_min_p(p, min_keep));
        }

        UniSampler SamplerFactory::adaptive_p(float target, float decay, uint32_t seed)
        {
            return UniSampler(llama_sampler_init_adaptive_p(target, decay, seed));
        }

        UniSampler SamplerFactory::temperature(float t)
        {
            return UniSampler(llama_sampler_init_temp(t));
        }

        UniSampler SamplerFactory::grammar(
            MulPtr<Model> model,
            const Str &str,
            const Str &root)
        {
            return UniSampler(
                llama_sampler_init_grammar(
                    llama_model_get_vocab(model->raw()),
                    str.c_str(),
                    root.c_str()));
        }

        SamplerChain::SamplerChain(SamplerParams params)
            : m_chain(llama_sampler_chain_init(params.raw()))
        {
        }

        SamplerChain::SamplerChain() : SamplerChain(SamplerParams()) {}

        SamplerChain::~SamplerChain()
        {
            free_resource();
        }

        SamplerChain::SamplerChain(const SamplerChain &other)
        {
            if (!other.m_chain)
                return;

            m_chain = llama_sampler_clone(other.m_chain);
            if (!m_chain)
                throw std::runtime_error("Failed to clone llama_sampler_chain");
            m_views = other.m_views;
            rebind_views();
        }

        SamplerChain &SamplerChain::operator=(const SamplerChain &other)
        {
            if (this == &other)
                return *this;

            free_resource();

            if (other.m_chain)
            {
                m_chain = llama_sampler_clone(other.m_chain);
                if (!m_chain)
                    throw std::runtime_error("Failed to clone llama_sampler_chain");
                m_views = other.m_views;
                rebind_views();
            }
            return *this;
        }

        SamplerChain::SamplerChain(SamplerChain &&other) noexcept
            : m_chain(other.m_chain),
              m_views(std::move(other.m_views))
        {
            other.m_chain = nullptr;
        }

        SamplerChain &SamplerChain::operator=(SamplerChain &&other) noexcept
        {
            if (this == &other)
                return *this;

            m_chain = other.m_chain;
            m_views = std::move(other.m_views);

            other.m_chain = nullptr;

            return *this;
        }

        void SamplerChain::push_back(
            UniSampler sampler,
            const Str &name)
        {
            if (!sampler)
                return;

            llama_sampler *raw_sampler = sampler.release();
            llama_sampler_chain_add(m_chain, raw_sampler);
            m_views.push_back({name, raw_sampler});
        }

        Token SamplerChain::sample(
            const Context &ctx,
            int32_t idx)
        {
            return llama_sampler_sample(m_chain, ctx.raw(), idx);
        }

        void SamplerChain::free_resource()
        {
            if (m_chain)
            {
                llama_sampler_free(m_chain);
                m_chain = nullptr;
            }
            m_views.clear();
        }

        void SamplerChain::rebind_views()
        {
            if (!m_chain)
                return;

            for (size_t i = 0; i < m_views.size(); ++i)
            {
                llama_sampler *new_sub_ptr =
                    llama_sampler_chain_get(m_chain, (int32_t)i);

                if (new_sub_ptr)
                {
                    m_views[i].raw = new_sub_ptr;
                }
                else
                {
                    // unreachable?
                    m_views[i].raw = nullptr;
                }
            }
        }
    }
}