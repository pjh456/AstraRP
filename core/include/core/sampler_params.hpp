#ifndef INCLUDE_ASTRA_RP_SAMPLER_PARAMS_HPP
#define INCLUDE_ASTRA_RP_SAMPLER_PARAMS_HPP

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        class SamplerParamsBuilder;

        class SamplerParams
        {
            friend class SamplerParamsBuilder;

        private:
            llama_sampler_chain_params m_params;

        public:
            SamplerParams(llama_sampler_chain_params params)
                : m_params(params) {}

            SamplerParams()
                : SamplerParams(
                      llama_sampler_chain_default_params()) {}

            ~SamplerParams() = default;

            SamplerParams(const SamplerParams &) = default;
            SamplerParams &operator=(const SamplerParams &) = default;

            SamplerParams(SamplerParams &&) noexcept = default;
            SamplerParams &operator=(SamplerParams &&) noexcept = default;

        public:
            llama_sampler_chain_params raw() const noexcept { return m_params; }
        };

        class SamplerParamsBuilder
        {
        private:
            SamplerParams m_params;

        public:
            SamplerParamsBuilder() = default;

            SamplerParams build() const noexcept { return m_params; }

        public:
            SamplerParamsBuilder &perf(bool flag)
            {
                m_params.m_params.no_perf = !flag;
                return *this;
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SAMPLER_PARAMS_HPP