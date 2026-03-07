#ifndef INCLUDE_ASTRA_RP_SAMPLER_HPP
#define INCLUDE_ASTRA_RP_SAMPLER_HPP

#include <stdexcept>

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
        class SamplerBuilder;

        class Sampler
        {
            friend class SamplerBuilder;

        private:
            llama_sampler *m_sampler;

        private:
            Sampler(llama_sampler *sampler = nullptr);

        public:
            ~Sampler();

            Sampler(const Sampler &);
            Sampler &operator=(const Sampler &);

            Sampler(Sampler &&) noexcept;
            Sampler &operator=(Sampler &&) noexcept;

        public:
            Token sample(const Context &ctx, int32_t idx);
        };

        class SamplerBuilder
        {
        private:
            Sampler m_sampler;
            bool m_has_selector = false;

        public:
            SamplerBuilder(SamplerParams params);
            SamplerBuilder() = default;

            Sampler build() const
            {
                if (!m_has_selector)
                    throw std::logic_error("Sampler chain must end with a selector like greedy() or seed()");
                return m_sampler;
            }

        public:
            SamplerBuilder &greedy();
            SamplerBuilder &seed(uint32_t seed);

        public:
            SamplerBuilder &top_k(int32_t k);
            SamplerBuilder &top_p(float p, size_t min_keep);
            SamplerBuilder &min_p(float p, size_t min_keep);
            SamplerBuilder &adaptive_p(float target, float decay, uint32_t seed);
            SamplerBuilder &temperature(float t);
            SamplerBuilder &grammar(
                MulPtr<Model> model,
                const Str &str,
                const Str &root);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SAMPLER_HPP