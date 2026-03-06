#ifndef INCLUDE_ASTRA_RP_CONTEXT_PARAMS_HPP
#define INCLUDE_ASTRA_RP_CONTEXT_PARAMS_HPP

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        class ContextManager;
        class ContextParamsBuilder;

        class ContextParams
        {
            friend class ContextManager;
            friend class ContextParamsBuilder;

        private:
            llama_context_params m_params;

        public:
            ContextParams(llama_context_params params)
                : m_params(params) {}

            ContextParams()
                : m_params(llama_context_default_params())
            {
                m_params.n_ctx = 0; // follow model
            }

            ~ContextParams() = default;

            ContextParams(const ContextParams &) = default;
            ContextParams &operator=(const ContextParams &) = default;

            ContextParams(ContextParams &&) noexcept = default;
            ContextParams &operator=(ContextParams &&) noexcept = default;

        public:
            llama_context_params raw() const noexcept { return m_params; }
        };

        class ContextParamsBuilder
        {
        private:
            ContextParams m_params;

        public:
            ContextParamsBuilder() = default;

            ContextParams build() const noexcept { return m_params; }

        public:
            ContextParamsBuilder &context_size(uint32_t size)
            {
                m_params.m_params.n_ctx = size;
                return *this;
            }

            ContextParamsBuilder &batch_size(uint32_t size)
            {
                m_params.m_params.n_batch = size;
                return *this;
            }

            ContextParamsBuilder &threads(uint32_t count)
            {
                m_params.m_params.n_threads = count;
                return *this;
            }

            ContextParamsBuilder &threads_batch(uint32_t count)
            {
                m_params.m_params.n_threads_batch = count;
                return *this;
            }

            ContextParamsBuilder &flash_attention(bool flag)
            {
                m_params.m_params.flash_attn_type =
                    flag
                        ? llama_flash_attn_type::
                              LLAMA_FLASH_ATTN_TYPE_ENABLED
                        : llama_flash_attn_type::
                              LLAMA_FLASH_ATTN_TYPE_DISABLED;
                return *this;
            }

            ContextParamsBuilder &kv_cache_type(ggml_type k, ggml_type v)
            {
                m_params.m_params.type_k = k;
                m_params.m_params.type_v = v;
                return *this;
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_CONTEXT_PARAMS_HPP