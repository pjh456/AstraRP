#include "core/model_params.hpp"

namespace astra_rp
{
    namespace core
    {
        ModelParams::ModelParams(
            const Str &name, llama_model_params params)
            : m_name(name), m_params(params) {}

        ModelParams::ModelParams(const Str &name)
            : ModelParams(name, llama_model_default_params()) {}
    }
}