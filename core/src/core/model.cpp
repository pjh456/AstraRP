#include "core/model.hpp"

#include "llama.h"

#include <stdexcept>

namespace astra_rp
{
    namespace core
    {
        Model::Model(const Str &name, llama_model *model)
            : m_name(name), m_model(model) {}

        Model::~Model()
        {
            if (m_model)
                llama_model_free(m_model);
        }
    }
}