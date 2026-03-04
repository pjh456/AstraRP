#include "core/context_params.hpp"

namespace astra_rp
{
    namespace core
    {

        ContextParams::ContextParams(llama_context_params params)
            : m_params(params) {}

        ContextParams::ContextParams()
            : m_params(llama_context_default_params())
        {
            m_params.n_ctx = 0; // follow model
        }

    }
}