#include "core/context.hpp"
#include "core/model.hpp"

#include "llama.h"

namespace astra_rp
{
    namespace core
    {
        Context::Context(
            MulPtr<Model> model,
            llama_context *ctx)
            : m_model(model),
              m_ctx(ctx
                        ? ctx
                        : llama_new_context_with_model(
                              model->raw(),
                              llama_context_default_params())) {}

        Context::~Context()
        {
            if (m_ctx)
            {
                llama_free(m_ctx);
                m_ctx = nullptr;
            }
        }

        uint32_t Context::capacity() const
        {
            return llama_n_ctx(m_ctx);
        }

        void Context::clear(bool only_meta)
        {
            if (m_ctx)
            {
                auto mem = llama_get_memory(m_ctx);
                llama_memory_clear(mem, !only_meta);
            }
        }

        size_t Context::state_size(int32_t seq_id) const
        {
            return seq_id < 0
                       ? llama_state_get_size(m_ctx)
                       : llama_state_seq_get_size(m_ctx, seq_id);
        }

        Vec<uint8_t> Context::get_state(int32_t seq_id) const
        {
            auto size = state_size(seq_id);

            Vec<uint8_t> vec;
            vec.resize(size);
            if (seq_id < 0)
                llama_state_get_data(
                    m_ctx, vec.data(), size);
            else
                llama_state_seq_get_data(
                    m_ctx, vec.data(), size, seq_id);

            return vec;
        }

        size_t Context::set_state(const Vec<uint8_t> &data, int32_t seq_id)
        {
            return seq_id < 0
                       ? llama_state_set_data(
                             m_ctx, data.data(), data.size())
                       : llama_state_seq_set_data(
                             m_ctx, data.data(), data.size(), seq_id);
        }
    }
}