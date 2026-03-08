#ifndef INCLUDE_ASTRA_RP_ENGINE_HPP
#define INCLUDE_ASTRA_RP_ENGINE_HPP

#include "utils/types.hpp"
#include "core/context.hpp"
#include "core/batch.hpp"

namespace astra_rp
{
    namespace infer
    {
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
            ResultV<void>
            decode(
                MulPtr<astra_rp::core::Context> ctx,
                MulPtr<astra_rp::core::Batch> batch);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_ENGINE_HPP