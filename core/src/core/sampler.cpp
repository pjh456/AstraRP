#include "core/sampler.hpp"
#include "core/model.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        Sampler::Sampler(llama_sampler *sampler)
            : m_sampler(
                  sampler
                      ? sampler
                      : llama_sampler_chain_init(
                            llama_sampler_chain_default_params())) {}

        Sampler::~Sampler()
        {
            if (m_sampler)
            {
                llama_sampler_free(m_sampler);
                m_sampler = nullptr;
            }
        }

        Sampler::Sampler(const Sampler &other)
            : m_sampler(llama_sampler_clone(other.m_sampler)) {}

        Sampler &Sampler::operator=(const Sampler &other)
        {
            if (this == &other)
                return *this;

            if (m_sampler)
                llama_sampler_free(m_sampler);

            m_sampler = llama_sampler_clone(other.m_sampler);

            return *this;
        }

        Sampler::Sampler(Sampler &&other) noexcept
            : m_sampler(other.m_sampler)
        {
            other.m_sampler = nullptr;
        }

        Sampler &Sampler::operator=(Sampler &&other) noexcept
        {
            if (this == &other)
                return *this;

            if (m_sampler)
                llama_sampler_free(m_sampler);

            m_sampler = other.m_sampler;
            other.m_sampler = nullptr;

            return *this;
        }

        Token Sampler::sample(const Context &ctx, int32_t idx)
        {
            return llama_sampler_sample(m_sampler, ctx.raw(), idx);
        }

        SamplerBuilder &SamplerBuilder::greedy()
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_greedy());
            return *this;
        }

        SamplerBuilder &SamplerBuilder::top_k(int32_t k)
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_top_k(k));
            return *this;
        }

        SamplerBuilder &SamplerBuilder::top_p(float p, size_t min_keep)
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_top_p(p, min_keep));
            return *this;
        }

        SamplerBuilder &SamplerBuilder::min_p(float p, size_t min_keep)
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_min_p(p, min_keep));
            return *this;
        }

        SamplerBuilder &SamplerBuilder::adaptive_p(float target, float decay, uint32_t seed)
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_adaptive_p(target, decay, seed));
            return *this;
        }

        SamplerBuilder &SamplerBuilder::temperature(float t)
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_temp(t));
            return *this;
        }

        SamplerBuilder &SamplerBuilder::seed(uint32_t seed)
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_dist(seed));
            return *this;
        }

        SamplerBuilder &SamplerBuilder::grammar(
            MulPtr<Model> model,
            const Str &str,
            const Str &root)
        {
            llama_sampler_chain_add(
                m_sampler.m_sampler,
                llama_sampler_init_grammar(
                    llama_model_get_vocab(model->raw()),
                    str.c_str(),
                    root.c_str()));
            return *this;
        }
    }
}